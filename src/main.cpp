#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "input/DetectInput.h"
#include "core/registers/AtlasRegister.h"
#include "core/registers/BlockRegister.h"

// Cube vertices with normals (Position x, y, z | Normal nx, ny, nz | Texture tx, ty)
// Json reading library reads textures on map in alphabetical order, default blocks must be created with BaBoFLRT order in mind
std::vector<Vertex> cubeVertices = {
    // Back face
    {Vertex{glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f)}},
    {Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f)}},
    {Vertex{glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f)}},
    // Bottom face
    {Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f)}},
    {Vertex{glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f)}},
    // Front face
    {Vertex{glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)}},
    {Vertex{glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)}},
    {Vertex{glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f)}},
    // Left face
    {Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f)}},
    // Right face
    {Vertex{glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}},
    // Top face
    {Vertex{glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f)}},
    {Vertex{glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f)}},
    {Vertex{glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f)}}
};

std::vector<GLuint> cubeIndices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20
};

std::vector<Vertex> lightVertices = {
    {Vertex{glm::vec3(-0.1f, -0.1f,  0.1f)}},
    {Vertex{glm::vec3( 0.1f, -0.1f,  0.1f)}},
    {Vertex{glm::vec3( 0.1f,  0.1f,  0.1f)}},
    {Vertex{glm::vec3(-0.1f,  0.1f,  0.1f)}},
    {Vertex{glm::vec3(-0.1f, -0.1f, -0.1f)}},
    {Vertex{glm::vec3( 0.1f, -0.1f, -0.1f)}},
    {Vertex{glm::vec3( 0.1f,  0.1f, -0.1f)}},
    {Vertex{glm::vec3(-0.1f,  0.1f, -0.1f)}}
};

std::vector<GLuint> lightIndices = {
    0, 1, 2, 2, 3, 0,
    6, 5, 4, 4, 7, 6,
    7, 4, 0, 0, 3, 7,
    1, 5, 6, 6, 2, 1,
    5, 1, 0, 0, 4, 5,
    3, 2, 6, 6, 7, 3
};

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

const char* windowTitle = "TerraLink";

int _fpsCount = 0, fps = 0;
float prevTime = 0.0f;

std::string fpsCount() {

    float curTime = glfwGetTime();
    ++_fpsCount;
    if (curTime - prevTime > 1.0f) {
        prevTime = curTime;
        fps = _fpsCount;
        _fpsCount = 0;
    }

    return std::string(windowTitle).substr(0,9) + "  //  " + std::to_string(fps) + " fps";
}

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, windowTitle, 900, 700)) {
        glfwTerminate();
        return -1;
    }

    BlockRegister blockRegister;
    Atlas blockAtlas("../../assets/textures/blocks/");
    blockAtlas.linkBlocksToAtlas(&blockRegister);

    for (int i = 0; i < cubeVertices.size(); i++) cubeVertices[i].texCoords = blockRegister.blocks[2].vertices[i].texCoords;

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");

    VertexArrayObject VAO;
    VAO.bind();
    VAO.addVertexBuffer(cubeVertices);
    VAO.addElementBuffer(cubeIndices);
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    Shader lightShader("../../shaders/light.vert", "../../shaders/light.frag");

    VertexArrayObject lightVAO;
    lightVAO.bind();
    lightVAO.addVertexBuffer(lightVertices);
    lightVAO.addElementBuffer(lightIndices);
    lightVAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(1.2f, 1.2f, 1.2f);
	glm::mat4 lightModel = glm::mat4(1.0f);

    glm::vec3 cubePos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 cubeModel = glm::mat4(1.0f);
    cubeModel = glm::translate(cubeModel, cubePos);

    lightShader.setUniform4("lightColor", lightColor);

    shaderProgram.setUniform4("model", cubeModel);
    shaderProgram.setUniform4("lightColor", lightColor);

    Texture atlas("../../assets/textures/blocks/block_atlas.png", GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    atlas.setUniform(shaderProgram, "tex0", 0);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Enable alpha values for textures
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame
    
    // Main program loop
    while (!glfwWindowShouldClose(window)) {

        deltaTime = glfwGetTime() - lastFrame;
        lastFrame += deltaTime;

        processInput(window, &camera, deltaTime, &lightPos);

        camera.updateCameraMatrix(0.1f, 100.0f, window);

        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram.ID);
        shaderProgram.setUniform4("cameraMatrix", camera.cameraMatrix);
        shaderProgram.setUniform3("camPos", glm::vec3(camera.position.x, camera.position.y, camera.position.z));
        shaderProgram.setUniform3("lightPos", glm::vec3(lightPos.x, lightPos.y, lightPos.z));
        atlas.bind();
        VAO.bind();
        glDrawElements(GL_TRIANGLES, cubeIndices.size(), GL_UNSIGNED_INT, 0);

        glUseProgram(lightShader.ID);
        lightShader.setUniform4("cameraMatrix", camera.cameraMatrix);
        
        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightShader.setMat4("model", lightModel);
        lightVAO.bind();
        glDrawElements(GL_TRIANGLES, lightIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount().c_str());

        CHECK_GL_ERROR();
    }

    VAO.deleteBuffers();
    lightVAO.deleteBuffers();
    atlas.deleteTexture();
    shaderProgram.deleteShader();
    lightShader.deleteShader();
    glfwTerminate();

    return 0;
}
