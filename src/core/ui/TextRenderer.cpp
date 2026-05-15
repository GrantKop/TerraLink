#include "core/ui/TextRenderer.h"

#include <cstdint>
#include <vector>

#include "core/ui/Font8x8.h"

namespace {

// Glyph atlas layout: 16 columns x 8 rows = 128 cells, each cell GLYPH_W x
// GLYPH_H.  Texture dims = 128 x 64 R8 texels.
constexpr int ATLAS_COLS = 16;
constexpr int ATLAS_ROWS = 8;

constexpr int ATLAS_W = ATLAS_COLS * terralink::font::GLYPH_W; // 128
constexpr int ATLAS_H = ATLAS_ROWS * terralink::font::GLYPH_H; //  64

struct GlyphVertex {
    float x, y;
    float u, v;
};

} // namespace

TextRenderer::TextRenderer(const std::string& basePath) {
    shader = std::make_unique<Shader>(
        basePath + "/shaders/text.vert",
        basePath + "/shaders/text.frag");

    buildFontTexture();
    buildGLBuffers();

    shader->use();
    shader->setInt("uFont", 0);
}

TextRenderer::~TextRenderer() {
    release();
}

void TextRenderer::release() {
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (fontTex) { glDeleteTextures(1, &fontTex); fontTex = 0; }
    if (shader) {
        shader->deleteShader();
        shader.reset();
    }
}

void TextRenderer::setProjection(const glm::mat4& proj) {
    projection = proj;
}

void TextRenderer::buildFontTexture() {
    // Build the per-glyph row bitmaps, then unpack to a single R8 atlas
    // image.  Each on bit becomes 0xFF; off becomes 0x00.
    std::uint8_t glyphBits[terralink::font::GLYPH_COUNT][terralink::font::GLYPH_H];
    terralink::font::buildAsciiBitmap(glyphBits);

    std::vector<std::uint8_t> atlasPixels(static_cast<std::size_t>(ATLAS_W) * ATLAS_H, 0);

    for (int g = 0; g < terralink::font::GLYPH_COUNT; ++g) {
        int gx = (g % ATLAS_COLS) * terralink::font::GLYPH_W;
        int gy = (g / ATLAS_COLS) * terralink::font::GLYPH_H;

        for (int r = 0; r < terralink::font::GLYPH_H; ++r) {
            std::uint8_t rowBits = glyphBits[g][r];
            for (int c = 0; c < terralink::font::GLYPH_W; ++c) {
                bool on = (rowBits >> (7 - c)) & 1u;
                int px = gx + c;
                int py = gy + r;
                atlasPixels[py * ATLAS_W + px] = on ? 0xFF : 0x00;
            }
        }
    }

    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_W, ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasPixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::buildGLBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphVertex),
                          reinterpret_cast<void*>(offsetof(GlyphVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphVertex),
                          reinterpret_cast<void*>(offsetof(GlyphVertex, u)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

float TextRenderer::measureWidth(const std::string& text, float cellPx) const {
    return static_cast<float>(text.size()) * cellPx;
}

void TextRenderer::drawString(const std::string& text,
                              float xPx,
                              float yPx,
                              float cellPx,
                              const glm::vec4& color) {
    if (text.empty() || color.a <= 0.0f) return;

    std::vector<GlyphVertex> verts;
    verts.reserve(text.size() * 6);

    const float uStep = 1.0f / static_cast<float>(ATLAS_COLS);
    const float vStep = 1.0f / static_cast<float>(ATLAS_ROWS);

    float penX = xPx;
    for (char ch : text) {
        unsigned int idx = static_cast<unsigned char>(ch);
        if (idx >= static_cast<unsigned int>(terralink::font::GLYPH_COUNT)) idx = '?';

        float u0 = (idx % ATLAS_COLS) * uStep;
        float v0 = (idx / ATLAS_COLS) * vStep;
        float u1 = u0 + uStep;
        float v1 = v0 + vStep;

        float x0 = penX;
        float y0 = yPx;
        float x1 = penX + cellPx;
        float y1 = yPx + cellPx;

        // Two triangles per glyph quad.  Texture origin is top-left in our
        // ortho projection, which is also how the atlas was uploaded, so UVs
        // are passed straight through.
        verts.push_back({x0, y0, u0, v0});
        verts.push_back({x1, y0, u1, v0});
        verts.push_back({x1, y1, u1, v1});

        verts.push_back({x0, y0, u0, v0});
        verts.push_back({x1, y1, u1, v1});
        verts.push_back({x0, y1, u0, v1});

        penX += cellPx;
    }

    shader->use();
    shader->setMat4("uProjection", projection);
    shader->setVec4("uColor", color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(GlyphVertex)),
                 verts.data(),
                 GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
