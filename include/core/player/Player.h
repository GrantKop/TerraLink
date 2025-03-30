#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>

#include "core/camera/Camera.h"
#include "core/world/FlatWorld.h"

class Player {
public:
    Player(GLFWwindow* window);

    void update(float deltaTime, GLFWwindow* window);

    void setPosition(float x, float y, float z);
    void setPosition(const glm::vec3& pos);

    glm::vec3 getPosition() const;
    glm::ivec3 getChunkPosition() const;

    Camera& getCamera();

private:
    Camera camera;
    void handleInput(GLFWwindow* window, float deltaTime);
};

#endif
