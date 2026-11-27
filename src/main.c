#include <assert.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "common.h"

/* TODO: provide a way to configure this value */
#define TICK_PER_SEMIBREVE 160.0

static struct note *load_voice_from_file(const char *pathname, size_t *count) {
    char *text = slurp(pathname);

    struct note *ret;
    *count = parse_voice(&ret, text, TICK_PER_SEMIBREVE);

    free(text);

    return ret;
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

static bool should_reload_shaders = false;

static void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if(action == GLFW_RELEASE)
        return;

    switch(key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_LEFT:
            if(score.state != SCORE_PLAYING)
                score.current_tick -= TICK_PER_SEMIBREVE / 16.0f;
            break;
        case GLFW_KEY_RIGHT:
            if(score.state != SCORE_PLAYING)
                score.current_tick += TICK_PER_SEMIBREVE / 16.0f;
            break;
        case GLFW_KEY_P:
            if(score.state != SCORE_PLAYING)
                play_score();
            break;
        case GLFW_KEY_R:
            should_reload_shaders = true;
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

        if(!should_reload_shaders)
            die("could not link program %s:\n%s", shader_name, log);
        return 0;
    }

    glDetachShader(program, vshader);
    glDetachShader(program, fshader);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return program;
}

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

static void reload_voice_shader(struct voice_gl *voice) {
    GLuint new_program = make_program("rectangles");

    if(new_program) {
        glDeleteProgram(voice->program);

        voice->program = new_program;
    }
}

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

static struct voice_gl *voices;
static size_t           voice_count;

static void init(int argc, char **argv) {
    argc -= 1;
    argv += 1;

    voice_count = argc;
    voices      = xmalloc(voice_count * sizeof voices[0]);

    for(size_t i = 0; i < voice_count; i += 1) {
        size_t note_count;
        struct note *notes = load_voice_from_file(argv[i], &note_count);

        make_rectangles_voice_gl(&voices[i], notes, note_count);
    }
}

static void render_voices() {
    for(size_t i = 0; i < voice_count; i += 1) {
        render_voice_gl(&voices[i], score.current_tick);
    }
}

static void loop(GLFWwindow *window) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while(!glfwWindowShouldClose(window)) {
        if(should_reload_shaders)
            goto reload;

        glClear(GL_COLOR_BUFFER_BIT);

#       define TPS (160.0 / 3.0) /* BPM 80.0 */

        if(score.state == SCORE_PLAYING) {
            double elapsed = glfwGetTime();
            double current_tick = elapsed * TPS;

            score.current_tick = (float)current_tick;
        }

        render_voices();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return;
reload:
    for(size_t i = 0; i < voice_count; i += 1) {
        reload_voice_shader(&voices[i]);
    }

    should_reload_shaders = false;

    loop(window);
}

int main(int argc, char **argv) {
    if(!glfwInit())
        die("could not initialize openGL");

    glfwSetErrorCallback(error_callback);

    atexit(glfwTerminate);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(800, 800, "Scrolling Score", NULL, NULL);

    if(!window)
        die("could not create window");

    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK)
        die("could not load extensions");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSwapInterval(1);

    reset_score();

    init(argc, argv);

    loop(window);

    glfwDestroyWindow(window);
}
