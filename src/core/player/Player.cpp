#include "core/player/Player.h"

Player* Player::s_instance = nullptr;

void Player::setInstance(Player* instance) {
    s_instance = instance;
}

Player& Player::instance() {
    if (!s_instance) {
        s_instance = new Player(nullptr); // Pass nullptr for window, will be set later
    }
    return *s_instance;
}

Player::Player(GLFWwindow* window) : camera(glm::vec3(5.0f, 20.0f, 3.0f)) {
    camera.updateCameraMatrix(0.1f, 300.0f, window);
}

void Player::update(float deltaTime, GLFWwindow* window) {
    handleInput(window, deltaTime);
    camera.updateCameraMatrix(0.1f, 300.0f, window);
}

// Sets the player's position using x, y, z coordinates
void Player::setPosition(float x, float y, float z) {
    camera.position = glm::vec3(x, y, z);
}

// Sets the player's position using a glm::vec3 object
void Player::setPosition(const glm::vec3& pos) {
    camera.position = pos;
}

// Returns the player's current position as a glm::vec3 object
glm::vec3 Player::getPosition() const {
    return camera.position;
}

// Returns the player's current chunk position as a glm::ivec3 object
glm::ivec3 Player::getChunkPosition() const {
    return glm::ivec3(floor(camera.position.x / CHUNK_SIZE), floor(camera.position.y / CHUNK_SIZE), floor(camera.position.z / CHUNK_SIZE));
}

// Returns a reference to the player's camera object
Camera& Player::getCamera() {
    return camera;
}

// Handles input for the player, including movement and camera rotation
void Player::handleInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.updatePosition(CAM_FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.updatePosition(CAM_BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.updatePosition(CAM_LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.updatePosition(CAM_RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.updatePosition(CAM_UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.updatePosition(CAM_DOWN, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.movementSpeed = 17.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        camera.movementSpeed = 3.5f;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        camera.processMouseMovement(window);
    } 
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        camera.firstClick = true;
    }
    
    static bool fullscreen = false;
    static bool f11WasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!f11WasPressed) {
            fullscreen = !fullscreen;
        
            if (fullscreen) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
                glfwSetWindowMonitor(window, monitor,
                                     0, 0, mode->width, mode->height,
                                     mode->refreshRate);
            } else {
                glfwSetWindowMonitor(window, nullptr,
                                     100, 100, 800, 600,
                                     0);
            }
        
            f11WasPressed = true;
        }
    } else {
        f11WasPressed = false;
    }
}
