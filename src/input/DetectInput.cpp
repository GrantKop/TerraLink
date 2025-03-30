#include "input/DetectInput.h"

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    std::cout << "Cursor position: (" << xpos << ", " << ypos << ")" << std::endl;
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        std::cout << "Left mouse button pressed" << std::endl;
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    std::cout << "Scroll offset: (" << xoffset << ", " << yoffset << ")" << std::endl;
}

void processInput(GLFWwindow* window, glm::vec3 *lightpos) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        lightpos->x += 0.12f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        lightpos->x -= 0.12f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        lightpos->z -= 0.12f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        lightpos->z += 0.12f;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
        lightpos->y += 0.12f;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
        lightpos->y -= 0.12f;
    }
}
