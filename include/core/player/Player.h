#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>

#include "core/player/Camera.h"
#include "core/world/World.h"

class Player {
public:
    static void setInstance(Player* instance);
    static Player& instance();

    Player(GLFWwindow* window);

    void update(float deltaTime, GLFWwindow* window, glm::vec3 *lightpos);

    void setPosition(float x, float y, float z);
    void setPosition(const glm::vec3& pos);

    glm::vec3 getPosition() const;
    glm::ivec3 getChunkPosition() const;

    Camera& getCamera();

    int VIEW_DISTANCE = 24;

private:
    Camera camera;
    void handleInput(GLFWwindow* window, float deltaTime, glm::vec3 *lightpos);

    static Player* s_instance;
};

#endif
