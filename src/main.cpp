#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/VertexArrayObject.h"
#include "graphics/Shader.h"
#include "input/DetectInput.h"

// Cube vertices with normals (Position x, y, z | Normal nx, ny, nz)
GLfloat cubeVertices[] = {
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

GLuint cubeIndices[] = {
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
    if (!createWindow(window, "TerraLink", 900, 700)) {
        glfwTerminate();
        return -1;
    }

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");

    VertexArrayObject VAO;
    VAO.addVertexBuffer(cubeVertices, sizeof(cubeVertices));
    VAO.addElementBuffer(cubeIndices, sizeof(cubeIndices));
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame
    
    // Main program loop
    while (!glfwWindowShouldClose(window)) {

        deltaTime = glfwGetTime() - lastFrame;
        lastFrame += deltaTime;

        processInput(window, &camera, deltaTime);

        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.use();

        camera.updateCameraMatrix(0.1f, 100.0f, window);
        shaderProgram.setUniform4("cameraMatrix", camera.cameraMatrix);

        VAO.bind();
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        VAO.unbind();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
