#ifndef CAMERA_H
#define CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL

#include "../../graphics/Shader.h"
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>
#include <GLFW/glfw3.h>


enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
    public:
        glm::vec3 position;
        glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 orthoProjection = glm::mat4(1.0f);
        glm::mat4 perspectiveProjection = glm::mat4(1.0f);
        glm::mat4 cameraMatrix = glm::mat4(1.0f);

        bool firstClick = false;

        float yaw;
        float pitch;

        float movementSpeed;
        float sensitivity;
        float fov;

        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
        float yaw = -90.0f, float pitch = 0.0f);

        void matrix(Shader& shaderProgram, const char* uniformName);
        void updateCameraMatrix(float nearPlane, float farPlane, GLFWwindow* window);
        void updatePosition(CameraMovement direction, float deltaTime);
        void processMouseMovement(GLFWwindow* window, bool constrainPitch = true);
        void processMouseScroll(float yoffset);

    private:
        void updateCameraVectors();
    
};

#endif
