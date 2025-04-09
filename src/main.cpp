#include <iostream>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "core/registers/AtlasRegister.h"
#include "core/world/World.h"
#include "core/player/Player.h"

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

std::string programName = "TerraLink";
std::string programVersion = "v0.1.8";
std::string windowTitle = programName + " " + programVersion;

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

    return std::string(windowTitle.c_str()) + "  //  " + std::to_string(fps) + " fps";
}

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, windowTitle.c_str(), 800, 600)) {
        glfwTerminate();
        return -1;
    }

    BlockRegister blockRegister;
    Atlas blockAtlas("../../assets/textures/blocks/");
    blockAtlas.linkBlocksToAtlas(&blockRegister);
    BlockRegister::setInstance(&blockRegister);

    Player player(window);
    Player::setInstance(&player);

    World world;
    world.init();

    Shader shaderProgram("../../shaders/block.vert", "../../shaders/block.frag");
    Shader lightShader("../../shaders/light.vert", "../../shaders/light.frag");

    VertexArrayObject lightVAO;
    lightVAO.init();
    lightVAO.bind();
    lightVAO.addVertexBuffer(lightVertices);
    lightVAO.addElementBuffer(lightIndices);
    lightVAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(8.f, 200.f, 8.f);
	glm::mat4 lightModel = glm::mat4(5.0f);

    lightShader.setUniform4("lightColor", lightColor);
    shaderProgram.setUniform4("lightColor", lightColor);

    Texture atlas("../../assets/textures/blocks/block_atlas.png", GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    atlas.setUniform(shaderProgram, "tex0", 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    try {
    // Main program loop
    while (!glfwWindowShouldClose(window)) {

        deltaTime = glfwGetTime() - lastFrame;
        lastFrame += deltaTime;

        glClearColor(0.38f, 0.66f, 0.77f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        player.update(deltaTime, &lightPos);

        world.uploadChunkMeshes(15);
        world.unloadDistantChunks();
        world.uploadChunksToMap();

        shaderProgram.setUniform4("cameraMatrix", player.getCamera().cameraMatrix);
        shaderProgram.setUniform3("camPos", player.getCamera().position);
        shaderProgram.setUniform3("lightPos", lightPos);
        atlas.bind();

        // First: draw all opaque geometry
        for (auto& [pos, chunk] : world.chunks) {
            if (!chunk->mesh.isUploaded || chunk->mesh.opaqueIndices.empty()) continue;
            chunk->mesh.opaqueVAO.bind();
            glDrawElements(GL_TRIANGLES, chunk->mesh.opaqueIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // Then sort & draw transparent geometry
        static glm::vec3 lastCamPos = player.getCamera().position;
        static std::vector<std::pair<float, Chunk*>> sortedTransparentChunks;
        static float lastSortTime = 0;

        float distMoved = glm::length(player.getCamera().position - lastCamPos);
        if (distMoved > 1.0f || glfwGetTime() - lastSortTime > 0.25f) {
            lastSortTime = glfwGetTime();
            lastCamPos = player.getCamera().position;

            sortedTransparentChunks.clear();
            for (auto& [pos, chunk] : world.chunks) {
                if (!chunk->mesh.transparentVertices.empty()) {
                    glm::vec3 worldPos = glm::vec3(pos.x, pos.y, pos.z) * static_cast<float>(CHUNK_SIZE);
                    float dist = world.distanceToCamera(worldPos, player.getCamera().position);
                    sortedTransparentChunks.emplace_back(dist, chunk.get());
                }
            }

            std::sort(sortedTransparentChunks.begin(), sortedTransparentChunks.end(),
                                [](auto& a, auto& b) { return a.first > b.first; });
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (auto& [dist, chunk] : sortedTransparentChunks) {
            chunk->mesh.transparentVAO.bind();
            glDrawElements(GL_TRIANGLES, chunk->mesh.transparentIndices.size(), GL_UNSIGNED_INT, 0);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glUseProgram(lightShader.ID);
        lightShader.setUniform4("cameraMatrix", player.getCamera().cameraMatrix);
        
        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightShader.setMat4("model", lightModel);
        lightVAO.bind();
        glDrawElements(GL_TRIANGLES, lightIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount().c_str());

        // CHECK_GL_ERROR();
    }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred." << std::endl;
    }

    lightVAO.deleteBuffers();
    atlas.deleteTexture();
    shaderProgram.deleteShader();
    lightShader.deleteShader();
    glfwTerminate();

    return 0;
}
