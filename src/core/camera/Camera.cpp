#include "core/camera/Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : position(position), front(glm::vec3(0.0f, 0.0f, -1.0f)), 
    worldUp(up), yaw(yaw), pitch(pitch), 
    movementSpeed(1.25f), sensitivity(120.0f), fov(45.0f) {
    updateCameraVectors();
}

void Camera::matrix(Shader& shaderProgram, const char* uniformName) {
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, uniformName), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}

void Camera::updateCameraMatrix(float nearPlane, float farPlane, GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    perspectiveProjection = glm::perspective(glm::radians(fov), (float)width / (float)height, nearPlane, farPlane);
    orthoProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height, nearPlane, farPlane);
    cameraMatrix = perspectiveProjection * glm::lookAt(position, position + front, up);
}

void Camera::updatePosition(CameraMovement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    if (direction == FORWARD) {
        position += front * velocity;
    }
    if (direction == BACKWARD) {
        position -= front * velocity;
    }
    if (direction == LEFT) {
        position -= right * velocity;
    }
    if (direction == RIGHT) {
        position += right * velocity;
    }
    if (direction == UP) {
        position += up * velocity;
    } 
    if (direction == DOWN) {
        position -= up * velocity;
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::processMouseMovement(GLFWwindow* window, bool constrainPitch) {

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    if (firstClick) {
        glfwSetCursorPos(window, (width / 2), (height / 2));
        firstClick = false;
    }

    double mouseX;
    double mouseY;

    glfwGetCursorPos(window, &mouseX, &mouseY);
    float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
    float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;

    glm::vec3 newFront = glm::rotate(front, glm::radians(-rotX), glm::normalize(glm::cross(front, up)));

    if (!(glm::angle(newFront, up) <= glm::radians(5.0f)) || (glm::angle(newFront, -up) <= glm::radians(5.0f))) {
        front = newFront;
    }

    front = glm::rotate(front, glm::radians(-rotY), up);
    right = glm::normalize(glm::cross(front, up));

    glfwSetCursorPos(window, (width / 2), (height / 2));
}

void Camera::processMouseScroll(float yoffset) {
    if (fov >= 1.0f && fov <= 45.0f) {
        fov -= yoffset;
    }
    if (fov <= 1.0f) {
        fov = 1.0f;
    }
    if (fov >= 45.0f) {
        fov = 45.0f;
    }
}
