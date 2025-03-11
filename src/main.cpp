#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "input/DetectInput.h"
#include "core/registers/AtlasRegister.h"

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

GLfloat lightVertices[] =
{ //     COORDINATES     //
	-0.1f, -0.1f,  0.1f,
	-0.1f, -0.1f, -0.1f,
	 0.1f, -0.1f, -0.1f,
	 0.1f, -0.1f,  0.1f,
	-0.1f,  0.1f,  0.1f,
	-0.1f,  0.1f, -0.1f,
	 0.1f,  0.1f, -0.1f,
	 0.1f,  0.1f,  0.1f
};

GLuint lightIndices[] =
{
	0, 1, 2,
	0, 2, 3,
	7, 4, 0,
	3, 7, 0,
	6, 7, 3,
	2, 6, 3,
	5, 6, 2,
	1, 5, 2,
	4, 5, 1,
	0, 4, 1,
	6, 5, 4,
	7, 6, 4
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

    registerAtlas();

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");

    VertexArrayObject VAO;
    VAO.bind();
    VAO.addVertexBuffer(cubeVertices, sizeof(cubeVertices));
    VAO.addElementBuffer(cubeIndices, sizeof(cubeIndices));
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    VAO.unbind();

    Shader lightShader("../../shaders/light.vert", "../../shaders/light.frag");

    VertexArrayObject lightVAO;
    lightVAO.bind();
    lightVAO.addVertexBuffer(lightVertices, sizeof(lightVertices));
    lightVAO.addElementBuffer(lightIndices, sizeof(lightIndices));
    lightVAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    lightVAO.unbind();

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
        glDrawElements(GL_TRIANGLES, sizeof(cubeIndices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);

        glUseProgram(lightShader.ID);
        lightShader.setUniform4("cameraMatrix", camera.cameraMatrix);
        
        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightShader.setMat4("model", lightModel);
        lightVAO.bind();
        glDrawElements(GL_TRIANGLES, sizeof(lightIndices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount().c_str());
    }

    VAO.deleteBuffers();
    lightVAO.deleteBuffers();
    atlas.deleteTexture();
    shaderProgram.deleteShader();
    lightShader.deleteShader();
    glfwTerminate();

    return 0;
}
