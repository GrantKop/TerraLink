#include "core/player/Camera.h"

#include "core/world/World.h"
#include "core/player/Player.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : position(position), front(glm::vec3(0.0f, 0.0f, -1.0f)), 
    worldUp(up), yaw(yaw), pitch(pitch), 
    movementSpeed(5.0f), sensitivity(120.0f), fov(60.0f) {
    updateCameraVectors();
}

Camera::~Camera() {}

// Sets the field of view (FOV) for the camera
void Camera::setFOV(float fov, GLFWwindow* window) {
    this->fov = fov;

    if (window) {
        updateCameraMatrix(0.1f, Player::instance().getRenderDistance(), window);
    }
}

// Sets the base FOV for the camera
void Camera::setBaseFOV(float fov) {
    this->baseFOV = fov;
    this->fov = fov;
}

// Returns the base FOV of the camera
float Camera::getBaseFOV() const {
    return baseFOV;
}

// Sets the sensitivity for mouse movement
void Camera::setSensitivity(float sensitivity) {
    if (sensitivity < 10.0f) sensitivity = 10.0f;
    if (sensitivity > 500.0f) sensitivity = 500.0f;
    this->sensitivity = sensitivity;
}

// Sets the camera matrix in the shader program
void Camera::matrix(Shader& shaderProgram, const char* uniformName) {
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, uniformName), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}

// Updates the camera matrix based on the window size and projection type
void Camera::updateCameraMatrix(float nearPlane, float farPlane, GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (width == 0 || height == 0) return;
    perspectiveProjection = glm::perspective(glm::radians(fov), (float)width / (float)height, nearPlane, farPlane);
    orthoProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height, nearPlane, farPlane);
    cameraMatrix = perspectiveProjection * glm::lookAt(position, position + front, up);
}

// Updates the camera position based on the movement direction and delta time
void Camera::updatePosition(CameraMovement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    if (direction == CAM_FORWARD) {
        position += front * velocity;
    }
    if (direction == CAM_BACKWARD) {
        position -= front * velocity;
    }
    if (direction == CAM_LEFT) {
        position -= right * velocity;
    }
    if (direction == CAM_RIGHT) {
        position += right * velocity;
    }
    if (direction == CAM_UP) {
        position += up * velocity;
    } 
    if (direction == CAM_DOWN) {
        position -= up * velocity;
    }
}

// Updates the camera vectors based on the current yaw and pitch angles
void Camera::updateCameraVectors() {
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

// Processes mouse movement to update the camera orientation
void Camera::processMouseMovement(GLFWwindow* window, bool constrainPitch) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    if (firstClick) {
        glfwSetCursorPos(window, width / 2, height / 2);
        firstClick = false;
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    float rotX = sensitivity * static_cast<float>(mouseY - height / 2) / height;
    float rotY = sensitivity * static_cast<float>(mouseX - width / 2) / width;

    pitch -= rotX;
    yaw   += rotY;

    // Clamp pitch to avoid flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();  // Recalculate front, right, up

    glfwSetCursorPos(window, width / 2, height / 2);
}

// Processes mouse scroll input to adjust the field of view (FOV)
// Not in use
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

// Casts a ray from the camera to find the first block it intersects with
std::optional<RaycastHit> Camera::raycastToBlock(const World& world, float maxDistance) const {
    glm::vec3 rayOrigin = position;
    glm::vec3 rayDir = glm::normalize(front);

    const float step = 0.05f;
    float traveled = 0.0f;

    glm::ivec3 lastBlockPos = glm::floor(rayOrigin);

    while (traveled < maxDistance) {
        glm::vec3 sample = rayOrigin + rayDir * traveled;
        glm::ivec3 blockPos = glm::floor(sample);

        if (blockPos != lastBlockPos) {
            int blockID = world.getBlockIDAtWorldPosition(blockPos.x, blockPos.y, blockPos.z);
            if (blockID != 0) {
                glm::ivec3 faceNormal = lastBlockPos - blockPos;
                return RaycastHit{ blockPos, faceNormal };
            }

            lastBlockPos = blockPos;
        }

        traveled += step;
    }

    return std::nullopt;
}
