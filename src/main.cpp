#include <iostream>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "core/registers/AtlasRegister.h"
#include "core/world/World.h"
#include "core/player/Player.h"
#include "core/game/Game.h"

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

    return std::string(("TerraLink " + Game::instance().getGameVersion()).c_str()) + "  //  " + std::to_string(fps) + " fps";
}

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "TerraLink", 800, 600)) {
        glfwTerminate();
        return -1;
    }

    Game game;
    Game::setInstance(&game);

    Player player(window);
    Player::setInstance(&player);

    BlockRegister blockRegister;
    Atlas blockAtlas((Game::instance().getBasePath() + "/assets/textures/blocks/").c_str());
    blockAtlas.linkBlocksToAtlas(&blockRegister);
    BlockRegister::setInstance(&blockRegister);

    game.init();
    game.getWorld().setSaveDirectory("myWorld");
    game.getWorld().createSaveDirectory();

    World::instance().loadPlayerData(Player::instance(), "placeholder");

    Shader shaderProgram((Game::instance().getBasePath() + "/shaders/block.vert").c_str(), (Game::instance().getBasePath() + "/shaders/block.frag").c_str());
    Shader lightShader((Game::instance().getBasePath() + "/shaders/light.vert").c_str(), (Game::instance().getBasePath() + "/shaders/light.frag").c_str());

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

    Texture atlas((Game::instance().getBasePath() + "/assets/textures/blocks/block_atlas.png").c_str(), GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    atlas.setUniform(shaderProgram, "tex0", 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    while (!glfwWindowShouldClose(window)) {
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame += deltaTime;

        glClearColor(0.38f, 0.66f, 0.77f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        game.getWorld().uploadChunkMeshes(15);
        game.getWorld().unloadDistantChunks();
        game.getWorld().uploadChunksToMap();

        player.update(deltaTime, &lightPos);

        shaderProgram.setUniform4("cameraMatrix", player.getCamera().cameraMatrix);
        shaderProgram.setUniform3("camPos", player.getCamera().position);
        shaderProgram.setUniform3("lightPos", lightPos);
        atlas.bind();

        for (auto& [pos, chunk] : game.getWorld().chunks) {
            if (!chunk->mesh.isUploaded || chunk->mesh.vertices.empty() || chunk->mesh.indices.empty()) continue;

            chunk->mesh.VAO.bind();
            glDrawElements(GL_TRIANGLES, chunk->mesh.indices.size(), GL_UNSIGNED_INT, 0);
        }       
        
        glUseProgram(lightShader.ID);
        lightShader.setUniform4("cameraMatrix", player.getCamera().cameraMatrix);
        
        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightShader.setMat4("model", lightModel);
        lightVAO.bind();
        glDrawElements(GL_TRIANGLES, lightIndices.size(), GL_UNSIGNED_INT, 0);

        // game.getWorld().chunkReset();
        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount().c_str());

        // CHECK_GL_ERROR();
    }

    game.getWorld().shutdown();

    lightVAO.deleteBuffers();
    atlas.deleteTexture();
    shaderProgram.deleteShader();
    lightShader.deleteShader();
    glfwTerminate();

    return 0;
}
