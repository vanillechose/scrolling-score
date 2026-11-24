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
                score.current_tick -= 20.0f;
            break;
        case GLFW_KEY_RIGHT:
            if(score.state != SCORE_PLAYING)
                score.current_tick += 20.0f;
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

    glBindAttribLocation(program, NOTE_LOCATION,         "note");
    glBindAttribLocation(program, TICK_START_LOCATION,   "tick_start");
    glBindAttribLocation(program, TICK_END_LOCATION,     "tick_end");

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

static void loop(GLFWwindow *window) {
    GLuint program = make_program("rectangles");

    GLfloat vertices[][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
    };

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

    struct note {
        GLfloat note;
        GLfloat start;
        GLfloat end;
    };

    /* semibreve = 160.0 ticks */
    struct note notes[] = {
        { D,     0.0f,  80.0f },
        { A,    80.0f, 160.0f },
        { F,   160.0f, 240.0f },
        { D,   240.0f, 320.0f },
        { Cis, 320.0f, 400.0f },
        { D,   400.0f, 440.0f },
        { E,   440.0f, 480.0f },
        { F,   480.0f, 560.0f },

        { C - (12.0f * 2), 560.0f, 600.0f },
        { C + (12.0f * 2), 600.0f, 640.0f },
    };

    GLuint quad_vbo, notes_vbo;
    glCreateBuffers(1, &quad_vbo);
    glNamedBufferData(quad_vbo, sizeof vertices, vertices, GL_STATIC_DRAW);

    glCreateBuffers(1, &notes_vbo);
    glNamedBufferData(notes_vbo, sizeof notes, notes, GL_STATIC_DRAW);

    GLuint vao;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, quad_vbo, 0, sizeof vertices[0]);
    glVertexArrayVertexBuffer(vao, 1, notes_vbo, 0, sizeof notes[0]);
    glVertexArrayBindingDivisor(vao, 1, 1);

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, NOTE_LOCATION);
    glVertexArrayAttribFormat(vao, NOTE_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, note));
    glVertexArrayAttribBinding(vao, NOTE_LOCATION, 1);

    glEnableVertexArrayAttrib(vao, TICK_START_LOCATION);
    glVertexArrayAttribFormat(vao, TICK_START_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, start));
    glVertexArrayAttribBinding(vao, TICK_START_LOCATION, 1);

    glEnableVertexArrayAttrib(vao, TICK_END_LOCATION);
    glVertexArrayAttribFormat(vao, TICK_END_LOCATION, 1, GL_FLOAT, GL_FALSE,
            offsetof(struct note, end));
    glVertexArrayAttribBinding(vao, TICK_END_LOCATION, 1);

    glUseProgram(program);

    GLuint uniform_tick = glGetUniformLocation(program, "t");

    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

#       define TPS (160.0 / 3.0) /* BPM 80.0 */

        if(score.state == SCORE_PLAYING) {
            double elapsed = glfwGetTime();
            double current_tick = elapsed * TPS;

            score.current_tick = (float)current_tick;
        }

        glBindVertexArray(vao);

        glUniform1f(uniform_tick, score.current_tick);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, LEN(notes));

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
        glfwCreateWindow(1600, 900, "Scrolling Score", NULL, NULL);

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
