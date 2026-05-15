#include "core/ui/HeldBlockHUD.h"

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/registers/BlockRegister.h"
#include "graphics/Vertex.h"

HeldBlockHUD::HeldBlockHUD() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

HeldBlockHUD::~HeldBlockHUD() {
    release();
}

void HeldBlockHUD::release() {
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    indexCount = 0;
}

void HeldBlockHUD::update(int selectedBlockID) {
    if (selectedBlockID != lastBuiltID) {
        rebuildFor(selectedBlockID);
        lastBuiltID = selectedBlockID;
    }
}

void HeldBlockHUD::rebuildFor(int blockID) {
    const Block block = BlockRegister::instance().getBlockByIndex(blockID);
    const std::vector<Vertex>& verts = block.vertices;

    // Air and any malformed geometry: skip with an empty index buffer so
    // draw() short-circuits.
    if (verts.empty() || (verts.size() % 4) != 0) {
        indexCount = 0;
        return;
    }

    // Block geometry stored on each Block is a flat list of CCW quads (4
    // vertices per face).  Same triangulation the chunk mesher uses:
    // (0,1,2) and (0,2,3) preserves winding so back-face culling keeps the
    // outward-facing triangles.
    std::vector<GLuint> indices;
    indices.reserve((verts.size() / 4) * 6);
    for (GLuint base = 0; base < static_cast<GLuint>(verts.size()); base += 4) {
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, texCoords)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    indexCount = static_cast<GLsizei>(indices.size());
}

void HeldBlockHUD::draw(Shader& blockShader, int xPx, int yPx, int sizePx) {
    if (indexCount == 0) return;

    // Save the world viewport so we don't disturb anything that comes after.
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Square viewport in the framebuffer corner. (xPx, yPx) is the bottom-left
    // pixel because OpenGL's viewport origin is bottom-left.
    glViewport(xPx, yPx, sizePx, sizePx);

    // The world pass has already finished; clearing the depth buffer here is
    // free of side effects on subsequent draws (the next frame starts with
    // its own glClear).  Re-enabling depth test gives correct face sorting on
    // the cube even though Game::renderUI() turns it off for 2D UI.
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Block geometry occupies [0,1]^3.  Camera at the +X +Y +Z corner so the
    // top (+Y), right (+X) and front (+Z) faces are all visible, matching
    // Minecraft's held-item corner perspective.  Aspect = 1.0 because the
    // viewport is square.
    const glm::vec3 eye(2.2f, 1.7f, 2.2f);
    const glm::vec3 target(0.5f, 0.5f, 0.5f);
    const glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj = glm::perspective(glm::radians(28.0f), 1.0f, 0.1f, 20.0f);
    const glm::mat4 cameraMatrix = proj * view;
    const glm::mat4 model(1.0f);

    blockShader.use();
    blockShader.setMat4("cameraMatrix", cameraMatrix);
    blockShader.setMat4("model", model);
    blockShader.setUniform3("camPos", eye);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Restore the previous viewport and 2D-UI depth state.  The next world
    // frame re-enables depth test in Game::render's pipeline anyway, but we
    // leave things exactly as we found them.
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDisable(GL_DEPTH_TEST);
}
