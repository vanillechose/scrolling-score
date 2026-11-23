#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


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

static void error_callback(int code, const char *msg) {
    die("(glfw code %d) %s", code, msg);
}

static void framebuffer_size_callback(GLFWwindow *, int w, int h) {
    glViewport(0, 0, w, h);
}

static void key_callback(GLFWwindow *window, int key, int, int, int) {
    if(key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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

        die("could not load gl shader:\n%s", log);
    }

    free((void *)sources[0]);

    return shader;
}

static void loop(GLFWwindow *window) {
    GLuint vshader = make_shader("shaders/v.glsl", GL_VERTEX_SHADER);
    GLuint fshader = make_shader("shaders/f.glsl", GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    glLinkProgram(program);

    GLint ret;
    glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if(ret != GL_TRUE) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        char *log = xmalloc(len);
        glGetProgramInfoLog(program, len, &len, log);

        die("could not link program:\n%s", log);
    }

    glDetachShader(program, vshader);
    glDetachShader(program, fshader);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    GLfloat vertices[][2] = {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f },
    };

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferData(vbo, sizeof vertices, vertices, GL_STATIC_DRAW);

    GLuint vao;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof vertices[0]);
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glUseProgram(program);

    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, 6);

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

    loop(window);

    glfwDestroyWindow(window);
}
