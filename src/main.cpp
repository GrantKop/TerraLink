#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "input/DetectInput.h"

// Cube vertices with normals (Position x, y, z | Normal nx, ny, nz | Texture tx, ty)
GLfloat cubeVertices[] = {
    // Back face
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 0 - Back Bottom Left
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // 1 - Back Bottom Right
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // 2 - Back Top Right
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // 3 - Back Top Left

    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 4 - Front Bottom Left
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // 5 - Front Bottom Right
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // 6 - Front Top Right
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // 7 - Front Top Left

    // Left face
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 8 - Front Top Left
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 9 - Back Top Left
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // 10 - Back Bottom Left
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 11 - Front Bottom Left

    // Right face
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 12 - Front Top Right
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 13 - Back Top Right
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // 14 - Back Bottom Right
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 15 - Front Bottom Right

    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // 16 - Back Bottom Left
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 17 - Back Bottom Right
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // 18 - Front Bottom Right
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 19 - Front Bottom Left

    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 20 - Back Top Left
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // 21 - Back Top Right
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // 22 - Front Top Right
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f  // 23 - Front Top Left
};

GLuint cubeIndices[] = {
    // Front face
    4, 5, 6, 6, 7, 4,
    // Back face
    2, 1, 0, 0, 3, 2,
    // Left face
    8, 9, 10, 10, 11, 8,
    // Right face
    14, 13, 12, 12, 15, 14,
    // Bottom face
    16, 17, 18, 18, 19, 16,
    // Top face
    22, 21, 20, 20, 23, 22
};

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

const char* windowTitle = "TerraLink";

int _fpsCount = 0, fps = 0;
float prevTime = 0.0f;

char* fpsCount() {

    float curTime = glfwGetTime();
    ++_fpsCount;
    if (curTime - prevTime > 1.0f) {
        prevTime = curTime;
        fps = _fpsCount;
        _fpsCount = 0;
    }

    std::string temp = std::string(windowTitle).substr(0,9) + "  //  " + std::to_string(fps) + " fps";

    char* result = new char[temp.length() + 1];
    std::strcpy(result, temp.c_str());

    return result;
}

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, windowTitle, 900, 700)) {
        glfwTerminate();
        return -1;
    }

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");

    VertexArrayObject VAO;
    VAO.addVertexBuffer(cubeVertices, sizeof(cubeVertices));
    VAO.addElementBuffer(cubeIndices, sizeof(cubeIndices));
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

    Texture stone("../../assets/textures/blocks/stone_0.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
    stone.setUniform(shaderProgram, "tex0", 0);

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

        stone.bind();

        camera.updateCameraMatrix(0.1f, 100.0f, window);
        shaderProgram.setUniform4("cameraMatrix", camera.cameraMatrix);

        VAO.bind();
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        VAO.unbind();

        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount());
    }

    VAO.deleteBuffers();
    stone.deleteTexture();
    shaderProgram.deleteShader();
    glfwTerminate();

    return 0;
}
