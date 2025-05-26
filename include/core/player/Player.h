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
    Player();

    std::optional<glm::ivec3> getHighlightedBlock() const;

    void update(float deltaTime);

    int gameMode = 0;
    int selectedBlockID = 1;
    float verticalVelocity = 0.0f;
    glm::vec3 currentVelocity = glm::vec3(0.0f);
    const float acceleration = 40.0f;
    bool onGround = false;

    const float jumpSpeed = 7.0f;
    float jumpBufferTime = 0.0f;
    float jumpCooldown = 0.0f;
    const float jumpBufferMax = 0.2f;
    const float jumpDelay = .5f;
    bool jumpReleased = true;

    glm::vec3 playerSize = glm::vec3(0.6f, 1.8f, 0.6f);
    glm::vec3 playerPosition;
    const float eyeOffset = .7f;

    float distanceSinceLastStep = 0.0f;

    void moveWithCollision(glm::vec3 velocity, float deltaTime);

    void setPosition(float x, float y, float z);
    void setPosition(const glm::vec3& pos);

    glm::vec3 getPosition() const;
    glm::ivec3 getChunkPosition() const;

    GLFWwindow* getWindow(); 

    Camera& getCamera();

    void setPlayerName(const std::string& name);

    std::string getPlayerName();
    void syncViewDistance(int viewDistance);
    int getViewDistance() const { return VIEW_DISTANCE; }
    int getRenderDistance() const { return GL_RENDER_DISTANCE; }
    int getFarFogDistance() const { return FAR_FOG_DISTANCE; }
    int getNearFogDistance() const { return NEAR_FOG_DISTANCE; }
    int getBottomFogDistance() const { return BOTTOM_FOG_DISTANCE; }

    int blockCount = 18;

private:
    Camera camera;
    GLFWwindow* window = nullptr;

    std::string playerName = "player";

    void handleInput(float deltaTime);
    std::optional<glm::ivec3> highlightedBlock;

    static Player* s_instance;

    int VIEW_DISTANCE = 2;
    int NEAR_FOG_DISTANCE = 26;
    int FAR_FOG_DISTANCE = 32;
    int BOTTOM_FOG_DISTANCE = 32;
    float GL_RENDER_DISTANCE = 40.0f;
};

#endif
