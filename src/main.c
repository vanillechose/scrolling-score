#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

static void loop(GLFWwindow *window) {
    while(!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int, char **) {
    if(!glfwInit())
        die("could not initialize openGL");

    glfwSetErrorCallback(error_callback);

    atexit(glfwTerminate);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
