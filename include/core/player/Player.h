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

    void update(float deltaTime, glm::vec3 *lightpos);

    int selectedBlockID = 1;

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

    void handleInput(float deltaTime, glm::vec3 *lightpos);
    std::optional<glm::ivec3> highlightedBlock;

    static Player* s_instance;
};

#endif
