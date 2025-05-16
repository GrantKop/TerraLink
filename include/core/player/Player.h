#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>

#include "core/player/Camera.h"
#include "core/world/World.h"

class Camera;

class Player {
public:
    static void setInstance(Player* instance);
    static Player& instance();

    Player(GLFWwindow* window);

    std::optional<glm::ivec3> getHighlightedBlock() const;

    void update(float deltaTime);

    int gameMode = 0;
    int selectedBlockID = 1;
    float verticalVelocity = 0.0f;
    bool onGround = false;

    const float jumpSpeed = 7.0f;
    float jumpBufferTime = 0.0f;
    float jumpCooldown = 0.0f;
    const float jumpBufferMax = 0.2f;
    const float jumpDelay = .2f;

    glm::vec3 playerSize = glm::vec3(0.6f, 1.8f, 0.6f);
    glm::vec3 playerPosition;
    const float eyeOffset = .7f;

    void moveWithCollision(glm::vec3 velocity, float deltaTime);

    void setPosition(float x, float y, float z);
    void setPosition(const glm::vec3& pos);

    glm::vec3 getPosition() const;
    glm::ivec3 getChunkPosition() const;

    GLFWwindow* getWindow(); 

    Camera& getCamera();

    int VIEW_DISTANCE = 16;

private:
    Camera camera;
    GLFWwindow* window = nullptr;

    void handleInput(float deltaTime);
    std::optional<glm::ivec3> highlightedBlock;

    static Player* s_instance;
};

#endif
