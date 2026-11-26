#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define LEN(a) (sizeof (a) / sizeof (a)[0])


inline static void die(const char *fmt, ...) {
    fprintf(stderr, "fatal: ");

    va_list ap;
    va_start(ap, fmt);

    vfprintf(stderr, fmt, ap);

    va_end(ap);

    fprintf(stderr, ".\n");

    exit(1);
}

static void *xmalloc(size_t n) {
    return malloc(n) ?: (die("out of memory"), NULL);
}

static char *concat(const char *s0, ...) {
    size_t needed = 0;

    va_list ap;

    va_start(ap, s0);
    for(const char *s = s0; s; s = va_arg(ap, const char *))
        needed += strlen(s);
    va_end(ap);

    char *ret = xmalloc(needed + 1);
    ret[0] = '\0';

    va_start(ap, s0);
    for(const char *s = s0; s; s = va_arg(ap, const char *))
        strcat(ret, s);
    va_end(ap);

    return ret;
}

static char *slurp(const char *pathname) {
    FILE *stream = fopen(pathname, "r");

    if(!stream)
        die("could not open %s: %s", pathname, strerror(errno));

    struct stat st;
    if(fstat(fileno(stream), &st) != 0)
        die("could not read stats for %s: %s", pathname, strerror(errno));

    size_t len = st.st_size;
    char *buf = xmalloc(len + 1);

    size_t nread = fread(buf, 1, len, stream);
    if(nread != len) {
        if(ferror(stream)) {
            die("could not read file %s: %s", pathname, strerror(errno));
        } else {
            die("could not read file %s: expected %lu bytes, got %lu",
                    pathname, len, nread);
        }
    }

    buf[len] = '\0';

    fclose(stream);

    return buf;
}

static struct {
    enum {
        SCORE_STOP,
        SCORE_PLAYING,
    } state;

    GLfloat current_tick;
} score;

static void reset_score() {
    score.state        = SCORE_STOP;
    score.current_tick = 0.0f;
}

static void play_score() {
    score.state        = SCORE_PLAYING;
    score.current_tick = 0.0f;

    glfwSetTime(0.0);
}

static void error_callback(int code, const char *msg) {
    die("(glfw code %d) %s", code, msg);
}

static void framebuffer_size_callback(GLFWwindow *, int w, int h) {
    glViewport(0, 0, w, h);
}

static void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if(action == GLFW_RELEASE)
        return;

    switch(key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_LEFT:
            if(score.state != SCORE_PLAYING)
                score.current_tick -= 1.0f;
            break;
        case GLFW_KEY_RIGHT:
            if(score.state != SCORE_PLAYING)
                score.current_tick += 1.0f;
            break;
        case GLFW_KEY_P:
            if(score.state != SCORE_PLAYING)
                play_score();
            break;
        case GLFW_KEY_R:
            reset_score();
            break;
    }
}

static GLuint make_shader(const char *pathname, GLuint shader_type) {
    GLint         lens[1];
    GLchar const* sources[1];

    sources[0] = slurp(pathname);
    lens[0]    = strlen(sources[0]);

    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, sources, lens);
    glCompileShader(shader);

    GLint ret;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
    if(ret != GL_TRUE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        char *log = xmalloc(length);
        glGetShaderInfoLog(shader, length, &length, log);

        die("could not load gl shader %s:\n%s", pathname, log);
    }

    free((void *)sources[0]);

    return shader;
}

#define CORNER_LOCATION       0
#define NOTE_LOCATION         1
#define TICK_START_LOCATION   2
#define TICK_END_LOCATION     3

static GLuint make_program(const char *shader_name) {
    char *vshader_name = concat("shaders/", shader_name, ".v.glsl", NULL);
    char *fshader_name = concat("shaders/", shader_name, ".f.glsl", NULL);

    GLuint vshader = make_shader(vshader_name, GL_VERTEX_SHADER);
    GLuint fshader = make_shader(fshader_name, GL_FRAGMENT_SHADER);

    free(vshader_name);
    free(fshader_name);

    GLuint program = glCreateProgram();

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    glBindAttribLocation(program, CORNER_LOCATION,     "corner");
    glBindAttribLocation(program, NOTE_LOCATION,       "note");
    glBindAttribLocation(program, TICK_START_LOCATION, "tick_start");
    glBindAttribLocation(program, TICK_END_LOCATION,   "tick_end");

    glLinkProgram(program);

    GLint ret;
    glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if(ret != GL_TRUE) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        char *log = xmalloc(len);
        glGetProgramInfoLog(program, len, &len, log);

        die("could not link program %s:\n%s", shader_name, log);
    }

    glDetachShader(program, vshader);
    glDetachShader(program, fshader);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return program;
}

struct note {
    GLfloat note;
    GLfloat start;
    GLfloat end;
};

static void configure_notes_vao(GLuint vao, GLuint notes_binding_index) {
    glVertexArrayBindingDivisor(vao, notes_binding_index, 1);

    glEnableVertexArrayAttrib(vao, NOTE_LOCATION);
    glVertexArrayAttribFormat(vao, NOTE_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, note));
    glVertexArrayAttribBinding(vao, NOTE_LOCATION, notes_binding_index);

    glEnableVertexArrayAttrib(vao, TICK_START_LOCATION);
    glVertexArrayAttribFormat(vao, TICK_START_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, start));
    glVertexArrayAttribBinding(vao, TICK_START_LOCATION, notes_binding_index);

    glEnableVertexArrayAttrib(vao, TICK_END_LOCATION);
    glVertexArrayAttribFormat(vao, TICK_END_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, end));
    glVertexArrayAttribBinding(vao, TICK_END_LOCATION, notes_binding_index);
}

struct voice_gl {
    GLuint  vao;
    GLuint  vbos[2];
    GLuint  program;

    GLuint  note_count;
    GLuint  vertex_count;
};

static void make_rectangles_voice_gl(struct voice_gl *dst, struct note *notes,
                                    size_t note_count) {
    static const GLfloat vertices[][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
    };

#   define VERTICES_BINDING_INDEX 0
#   define NOTES_BINDING_INDEX    1

    memset(dst, 0, sizeof *dst);

    dst->program = make_program("rectangles");

    glCreateBuffers(2, dst->vbos);

    glNamedBufferData(dst->vbos[VERTICES_BINDING_INDEX],
            sizeof vertices, vertices, GL_STATIC_DRAW);
    glNamedBufferData(dst->vbos[NOTES_BINDING_INDEX],
            sizeof notes[0] * note_count, notes, GL_STATIC_DRAW);

    glCreateVertexArrays(1, &dst->vao);
    glVertexArrayVertexBuffer(dst->vao, VERTICES_BINDING_INDEX,
            dst->vbos[VERTICES_BINDING_INDEX], 0, sizeof vertices[0]);
    glVertexArrayVertexBuffer(dst->vao, NOTES_BINDING_INDEX,
            dst->vbos[NOTES_BINDING_INDEX], 0, sizeof notes[0]);

    glEnableVertexArrayAttrib(dst->vao, CORNER_LOCATION);
    glVertexArrayAttribFormat(dst->vao, CORNER_LOCATION, 2, GL_FLOAT,
            GL_FALSE, 0);
    glVertexArrayAttribBinding(dst->vao, CORNER_LOCATION,
            VERTICES_BINDING_INDEX);

    configure_notes_vao(dst->vao, NOTES_BINDING_INDEX);

    dst->note_count = note_count;
    dst->vertex_count = LEN(vertices);
}

static void render_voice_gl(struct voice_gl *voice, float tick) {
    glUseProgram(voice->program);
    glBindVertexArray(voice->vao);

    glUniform1f(glGetUniformLocation(voice->program, "t"), tick);

    glDrawArraysInstanced(GL_TRIANGLES, 0, voice->vertex_count,
            voice->note_count);
}

static void loop(GLFWwindow *window) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#   define C   72.0f
#   define Cis 73.0f
#   define D   74.0f
#   define Es  75.0f
#   define E   76.0f
#   define F   77.0f
#   define Fis 78.0f
#   define G   79.0f
#   define Gis 80.0f
#   define A   81.0f
#   define B   82.0f
#   define H   83.0f

#   define x(s, d) (40 * s), (40 * s + 40 * d)

    /* bass */
    struct note tenor[] = {
        { D, x(1, 1) },
        { B - 12, x(2.5, 0.5) },

        { Es, x(3, 1) },
        { C, x(4, 1) },
        { A - 12, x(5.5, 0.5) },

        { D, x(6, 1) },
        { B - 12, x(7, 1) },
        { G - 12, x(8.5, 0.5) },

        { C, x(9, 0.5) },
        { C, x(9.5, 0.5) },
        { C, x(10, 0.5) },
        { C, x(10.5, 0.5) },
        { C, x(11, 0.5) },
        { C, x(11.5, 0.5) },
        
        { C, x(12, 0.5) },
        { B - 12, x(12.5, 0.25) },
        { A - 12, x(12.75, 0.25) },
        { B - 12, x(13, 0.25) },
        { G - 12, x(13.25, 0.25) },
        { B - 12, x(13.50, 0.25) },
        { C, x(13.75, 0.25) },
        { D, x(14, 0.25) },
        { Cis, x(14.25, 0.25) },
        { D, x(14.50, 0.50) },

        { D, x(15, 0.5) },
        { G - 12, x(15.5, 0.5) },
        { G - 12, x(16.25, 0.25) },
        { A - 12, x(16.50, 0.25) },
        { B - 12, x(16.75, 0.25) },
        { C, x(17, 0.25) },
        { B - 12, x(17.25, 0.25) },
        { C, x(17.50, 0.50) },

        { C, x(18, 0.5) },
        { F - 12, x(18.5, 0.5) },
        { F - 12, x(19.25, 0.25) },
        { G - 12, x(19.50, 0.25) },
        { A - 12, x(19.75, 0.25) },
        { B - 12, x(20, 0.25) },
        { A - 12, x(20.25, 0.25) },
        { B - 12, x(20.50, 0.75) }, /* ~ */

        { A - 12, x(21.25, 0.25) },
        { B - 12, x(21.5, 0.25) },
        { A - 12, x(21.75, 0.25) },
        { G - 12, x(22, 0.25) },
        { F - 12, x(22.25, 0.25) },
        { G - 12, x(22.50, 0.25) },
        { F - 12, x(22.75, 0.25) },
        { E - 12, x(23, 0.25) },
        { D - 12, x(23.25, 0.25) },
        { E - 12, x(23.50, 0.25) },
        { D - 12, x(23.75, 0.25) },
    };
    struct note alto[] = {
        { G, x(13, 1) },
        { F, x(14.5, 0.5) },

        { B, x(15, 1) },
        { G, x(16, 1) },
        { E, x(17.5, 0.5) },

        { A, x(18, 1) },
        { F, x(19, 1) },
        { D, x(20.5, 0.5) },

        { G, x(21, 0.5) },
        { G, x(21.5, 0.5) },
        { G, x(22, 0.5) },
        { G, x(22.5, 0.5) },
        { G, x(23, 0.5) },
        { G, x(23.5, 0.5) },
    };

    struct voice_gl tenor_gl;
    struct voice_gl alto_gl;

    make_rectangles_voice_gl(&tenor_gl, tenor, LEN(tenor));
    make_rectangles_voice_gl(&alto_gl, alto, LEN(alto));

    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

#       define TPS (160.0 / 3.0) /* BPM 80.0 */

        if(score.state == SCORE_PLAYING) {
            double elapsed = glfwGetTime();
            double current_tick = elapsed * TPS;

            score.current_tick = (float)current_tick;
        }

        render_voice_gl(&tenor_gl, score.current_tick);
        render_voice_gl(&alto_gl, score.current_tick);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int, char **) {
    if(!glfwInit())
        die("could not initialize openGL");

    glfwSetErrorCallback(error_callback);

    atexit(glfwTerminate);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(1000, 1000, "Scrolling Score", NULL, NULL);

    if(!window)
        die("could not create window");

    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK)
        die("could not load extensions");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSwapInterval(1);

    reset_score();

    loop(window);

    glfwDestroyWindow(window);
}
