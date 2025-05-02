#include "core/player/Player.h"

Player* Player::s_instance = nullptr;

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void Player::setInstance(Player* instance) {
    s_instance = instance;
}

Player& Player::instance() {
    if (!s_instance) {
        s_instance = new Player(nullptr);
    }
    return *s_instance;
}

Player::Player(GLFWwindow* window) : playerPosition(5.0f, 130.0f, 3.0f), camera(glm::vec3(5.0f, 130.0f + eyeOffset, 3.0f)) {
    camera.updateCameraMatrix(0.1f, 500.0f, window);
    this->window = window;
    glfwSetScrollCallback(window, scrollCallback);
}

std::optional<glm::ivec3> Player::getHighlightedBlock() const {
    return highlightedBlock;
}

void Player::moveWithCollision(glm::vec3 velocity, float deltaTime) {
    glm::vec3 proposedPos = playerPosition + velocity * deltaTime;
    if (!World::instance().collidesWithBlockAABB(proposedPos, playerSize)) {
        playerPosition = proposedPos;
    } else {
        // Slide along axis
        glm::vec3 step = glm::vec3(velocity.x, 0.0f, 0.0f) * deltaTime;
        if (!World::instance().collidesWithBlockAABB(playerPosition + step, playerSize)) playerPosition += step;

        step = glm::vec3(0.0f, 0.0f, velocity.z) * deltaTime;
        if (!World::instance().collidesWithBlockAABB(playerPosition + step, playerSize)) playerPosition += step;

        step = glm::vec3(0.0f, velocity.y, 0.0f) * deltaTime;
        if (!World::instance().collidesWithBlockAABB(playerPosition + step, playerSize)) playerPosition += step;
    }
    camera.position = playerPosition + glm::vec3(0.0f, eyeOffset, 0.0f);
}

// Updates the player's position and camera based on input
void Player::update(float deltaTime, glm::vec3 *lightpos) {
    // Update jump timers
    jumpBufferTime -= deltaTime;
    jumpCooldown -= deltaTime;
    jumpBufferTime = std::max(0.0f, jumpBufferTime);
    jumpCooldown = std::max(0.0f, jumpCooldown);

    camera.updateCameraMatrix(0.1f, 800.0f, window);
    if (gameMode == 0) {
        glm::vec3 groundCheck = playerPosition;
        groundCheck.y -= 0.15f;
        onGround = World::instance().collidesWithBlockAABB(groundCheck, playerSize);
        if (onGround && jumpBufferTime > 0.0f && jumpCooldown <= 0.0f) {
            verticalVelocity = jumpSpeed;
            onGround = false;
            jumpBufferTime = 0.0f;
            jumpCooldown = jumpDelay;
        }        
    
        if (onGround && verticalVelocity < 0.0f) {
            verticalVelocity = 0.0f;
        }
    
        if (!onGround && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            const float gravity = -20.f;
            const float terminalVelocity = -75.0f;
            verticalVelocity = std::max(verticalVelocity + gravity * deltaTime, terminalVelocity);
        }

        moveWithCollision(glm::vec3(0.0f, verticalVelocity, 0.0f), deltaTime);
    }
    handleInput(deltaTime, lightpos);
}

// Sets the player's position using x, y, z coordinates
void Player::setPosition(float x, float y, float z) {
    playerPosition = glm::vec3(x, y, z);
    camera.position = playerPosition + glm::vec3(0.0f, eyeOffset, 0.0f);
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

// Returns the GLFW window object associated with the player
GLFWwindow* Player::getWindow() {
    return window;
}

// Returns a reference to the player's camera object
Camera& Player::getCamera() {
    return camera;
}

// Handles input for the player, including movement and camera rotation
void Player::handleInput(float deltaTime, glm::vec3 *lightpos) {
    if (gameMode == 1) {
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

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.updatePosition(CAM_DOWN, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            camera.movementSpeed = 38.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE) {
            camera.movementSpeed = 5.25f;
        }
    } else if (gameMode == 0) {
        glm::vec3 velocity(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            velocity += camera.getDirectionXZ();
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            velocity -= camera.getDirectionXZ();
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            velocity -= glm::normalize(glm::cross(camera.getDirectionXZ(), glm::vec3(0, 1, 0)));
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            velocity += glm::normalize(glm::cross(camera.getDirectionXZ(), glm::vec3(0, 1, 0)));
        }
        bool isSprinting = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        bool isSneaking = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        if (isSneaking) {
            camera.movementSpeed = 3.0f;
            verticalVelocity = 0.0f;
        } else if (isSprinting) {
            camera.movementSpeed = 8.75f;
        } else {
            camera.movementSpeed = 5.25f;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            jumpBufferTime = jumpBufferMax;
        }

        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * camera.movementSpeed;
            moveWithCollision(velocity, deltaTime);
        }
    }

    static bool lastNPress = false;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !lastNPress) {
        gameMode = gameMode == 0 ? 1 : 0;
        std::cout << "Switched to " << (gameMode == 0 ? "Walk" : "Fly") << " mode\n";
        verticalVelocity = 0.0f;
    }
    lastNPress = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    
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
            
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            } else {
                glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, 0);
            }
        
            f11WasPressed = true;
        }
    } else {
        f11WasPressed = false;
    }

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

    static bool lastLeftClick = false;
    bool leftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftNow && !lastLeftClick) {
        if (auto hit = camera.raycastToBlock(World::instance())) {
            World::instance().setBlockAtWorldPosition(hit->block.x, hit->block.y, hit->block.z, 0);
        }
    }    
    lastLeftClick = leftNow;

    static bool lastRightClick = false;
    bool rightNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !lastRightClick) {
        if (auto hit = camera.raycastToBlock(World::instance())) {
            glm::ivec3 placePos = hit->block + hit->normal;
            glm::vec3 blockCenter = glm::vec3(placePos) + 0.5f;
            if (!World::instance().wouldBlockOverlapPlayer(placePos)) {
                World::instance().setBlockAtWorldPosition(placePos.x, placePos.y, placePos.z, selectedBlockID);
            }
        }
    }
    lastRightClick = rightNow;

    if (gameMode == 1) {
        playerPosition = camera.position - glm::vec3(0.0f, eyeOffset, 0.0f);
    }

    // if (auto hit = camera.raycastToBlock(World::instance())) {
    //     highlightedBlock = hit->block;
    // } else {
    //     highlightedBlock.reset();
    // }    
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Player& player = Player::instance();

    player.selectedBlockID += (yoffset > 0) ? 1 : -1;

    if (player.selectedBlockID == 11) player.selectedBlockID += (yoffset > 0) ? 1 : -1;

    if (player.selectedBlockID < 1) player.selectedBlockID = 13;
    if (player.selectedBlockID > 13) player.selectedBlockID = 1;

    std::cout << "Selected Block ID: " << player.selectedBlockID << std::endl;
}

