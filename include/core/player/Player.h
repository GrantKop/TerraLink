#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>

#include "core/camera/Camera.h"
#include "core/world/FlatWorld.h"

class Player {
public:
    static void setInstance(Player* instance);
    static Player& instance();

    Player(GLFWwindow* window);

    void update(float deltaTime, GLFWwindow* window);

    void setPosition(float x, float y, float z);
    void setPosition(const glm::vec3& pos);

    glm::vec3 getPosition() const;
    glm::ivec3 getChunkPosition() const;

    Camera& getCamera();

    int VIEW_DISTANCE = 3;

private:
    Camera camera;
    void handleInput(GLFWwindow* window, float deltaTime);

    static Player* s_instance;
};

#endif
