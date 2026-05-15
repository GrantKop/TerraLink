#include "graphics/TextRenderer.h"
#include "graphics/Font8x8.h"

#include <vector>
#include <cstddef>

TextRenderer::TextRenderer() {}

TextRenderer::~TextRenderer() {}

// Packs the 128 embedded glyphs into one single-channel (GL_RED) texture
// laid out as a COLS x ROWS grid. .r == 1.0 means "ink", 0.0 means empty.
void TextRenderer::buildAtlasTexture() {
    atlasW = COLS * GLYPH;
    atlasH = ROWS * GLYPH;

    std::vector<unsigned char> pixels(atlasW * atlasH, 0);

    for (int c = 0; c < 128; ++c) {
        int cellX = (c % COLS) * GLYPH;
        int cellY = (c / COLS) * GLYPH;
        for (int row = 0; row < GLYPH; ++row) {
            unsigned char bits = FONT8X8_BASIC[c][row];
            for (int col = 0; col < GLYPH; ++col) {
                // Bit 0 (LSB) is the leftmost pixel in font8x8.
                if (bits & (1 << col)) {
                    int px = cellX + col;
                    int py = cellY + row; // row 0 = glyph top, stored first
                    pixels[py * atlasW + px] = 255;
                }
            }
        }
    }

    glGenTextures(1, &atlasTex);
    glBindTexture(GL_TEXTURE_2D, atlasTex);

    // 1-byte rows: atlasW (128) is a multiple of 4 here, but be explicit so
    // this stays correct if the grid dimensions ever change.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasW, atlasH, 0,
                 GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::init(const std::string& basePath) {
    // Reuse the existing UI vertex shader; only the fragment stage differs
    // (it multiplies the glyph coverage by a tint/alpha uniform).
    shader = std::make_unique<Shader>(
        basePath + "/shaders/ui.vert",
        basePath + "/shaders/text.frag"
    );

    buildAtlasTexture();

    // Configure the VAO once with a single placeholder quad so the vertex
    // attribute layout is captured. The VBO name never changes afterwards,
    // so re-uploading vertex data each frame keeps these attributes valid.
    std::vector<Vertex> seed = {
        {{0,0,0}, {0,0,1}, {0,0}},
        {{1,0,0}, {0,0,1}, {1,0}},
        {{1,1,0}, {0,0,1}, {1,1}},
        {{0,1,0}, {0,0,1}, {0,1}},
    };
    std::vector<GLuint> seedIdx = {0, 2, 1, 0, 3, 2};

    vao = std::make_unique<VertexArrayObject>();
    vao->init();
    vao->bind();
    vao->addVertexBuffer(seed, GL_DYNAMIC_DRAW);
    vao->addElementBuffer(seedIdx, GL_DYNAMIC_DRAW);
    vao->addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    vao->addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    vao->addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    vao->unbind();

    shader->use();
    shader->setInt("tex0", 0);

    ready = true;
}

float TextRenderer::measureWidth(const std::string& text, float pixelHeight) const {
    return static_cast<float>(text.size()) * pixelHeight; // monospace cells
}

void TextRenderer::drawText(const std::string& text, float x, float y,
                            float pixelHeight, const glm::vec4& color,
                            const glm::mat4& projection) {
    if (!ready || text.empty() || color.a <= 0.0f) return;

    const float invW = 1.0f / static_cast<float>(atlasW);
    const float invH = 1.0f / static_cast<float>(atlasH);

    std::vector<Vertex> verts;
    std::vector<GLuint> indices;
    verts.reserve(text.size() * 4);
    indices.reserve(text.size() * 6);

    float penX = x;
    for (unsigned char ch : text) {
        if (ch >= 128) ch = '?';

        int cellX = (ch % COLS) * GLYPH;
        int cellY = (ch / COLS) * GLYPH;

        // Half-texel inset avoids bleeding from neighbouring glyphs.
        float u0 = (cellX + 0.5f) * invW;
        float u1 = (cellX + GLYPH - 0.5f) * invW;
        float v0 = (cellY + 0.5f) * invH;            // glyph top
        float v1 = (cellY + GLYPH - 0.5f) * invH;    // glyph bottom

        float x0 = penX;
        float x1 = penX + pixelHeight;
        float y0 = y;                 // top edge (smaller screen y)
        float y1 = y + pixelHeight;   // bottom edge

        GLuint base = static_cast<GLuint>(verts.size());

        // Same vertex order / winding as the existing crosshair quad so the
        // global GL_CULL_FACE state does not cull these triangles.
        verts.push_back({{x0, y0, 0.0f}, {0, 0, 1}, {u0, v0}}); // top-left
        verts.push_back({{x1, y0, 0.0f}, {0, 0, 1}, {u1, v0}}); // top-right
        verts.push_back({{x1, y1, 0.0f}, {0, 0, 1}, {u1, v1}}); // bottom-right
        verts.push_back({{x0, y1, 0.0f}, {0, 0, 1}, {u0, v1}}); // bottom-left

        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 0);
        indices.push_back(base + 3);
        indices.push_back(base + 2);

        penX += pixelHeight; // monospace advance
    }

    if (indices.empty()) return;

    shader->use();
    shader->setMat4("uProjection", projection);
    shader->setVec4("uColor", color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTex);

    vao->bind();
    vao->addVertexBuffer(verts, GL_DYNAMIC_DRAW);
    vao->addElementBuffer(indices, GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    vao->unbind();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::shutdown() {
    if (atlasTex) {
        glDeleteTextures(1, &atlasTex);
        atlasTex = 0;
    }
    if (vao) vao->deleteBuffers();
    if (shader) shader->deleteShader();
    ready = false;
}
