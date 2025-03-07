#ifndef ERROR_CHECK_GL_H
#define ERROR_CHECK_GL_H

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

bool loadGlad();

void initGLFW(unsigned int versionMajor, unsigned int versionMinor);

void framebufferSizeCallback(GLFWwindow* window, int width, int height);

bool createWindow(GLFWwindow*& window, const char* title, unsigned int width, unsigned int height, GLFWframebuffersizefun = framebufferSizeCallback);

int checkGLError();

#define CHECK_GL_ERROR() { \
    int err = checkGLError(); \
    if (err != GL_NO_ERROR) { \
        std::cerr << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    } \
} 

#endif
