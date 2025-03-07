#ifndef DETECTINPUT_H
#define DETECTINPUT_H

#include "../core/camera/Camera.h"
#include <GLFW/glfw3.h>

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void processInput(GLFWwindow* window, Camera* camera, float deltaTime);

#endif
