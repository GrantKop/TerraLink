#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/VertexBuffer.h"
#include "input/DetectInput.h"

// Cube vertices with normals (Position x, y, z | Normal nx, ny, nz)
float cubeVertices[] = {
    // Back face
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 0 - Back Bottom Left
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 1 - Back Bottom Right
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 2 - Back Top Right
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 3 - Back Top Left

    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 4 - Front Bottom Left
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 5 - Front Bottom Right
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 6 - Front Top Right
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 7 - Front Top Left

    // Left face
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 0 - Left Bottom Back
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 3 - Left Top Back
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 4 - Left Bottom Front
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 7 - Left Top Front

    // Right face
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 1 - Right Bottom Back
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 2 - Right Top Back
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 5 - Right Bottom Front
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 6 - Right Top Front

    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 0 - Bottom Back Left
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 1 - Bottom Back Right
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 4 - Bottom Front Left
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 5 - Bottom Front Right

    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 3 - Top Back Left
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 2 - Top Back Right
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 7 - Top Front Left
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f  // 6 - Top Front Right
};


unsigned int cubeIndices[] = {
    // Front face
    4, 5, 6,  6, 7, 4,
    // Back face
    0, 3, 2,  2, 1, 0,
    // Left face
    4, 7, 3,  3, 0, 4,
    // Right face
    1, 2, 6,  6, 5, 1,
    // Top face
    3, 7, 6,  6, 2, 3,
    // Bottom face
    4, 0, 1,  1, 5, 4
};

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "Fabrivis", 800, 600)) {
        glfwTerminate();
        return -1;
    }

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");

    VertexBuffer vb;
    vb.setData<float>(0, 144, cubeVertices, GL_STATIC_DRAW);
    vb.setAttribPointer<float>(0, 3, GL_FLOAT, 6, 0);
    shaderProgram.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 blockModel = glm::mat4(1.0f);
    blockModel = glm::translate(blockModel, glm::vec3(0.0f, 0.0f, 0.0f));
    shaderProgram.setMat4("Model", blockModel);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame
    
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, &camera, deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.use();

        camera.updateCameraMatrix(0.1f, 100.0f, window);
        shaderProgram.setMat4("cameraMatrix", camera.cameraMatrix);

        vb.draw(GL_TRIANGLES, 36, GL_UNSIGNED_INT, cubeIndices, 1);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
