#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <memory>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "graphics/Shader.h"

// Minimal 2D text renderer backed by an embedded 8x8 bitmap font.
//
// Owns a single R8 glyph atlas built from Font8x8 at construction time,
// a dynamic VBO that holds one quad per character, and a small text shader
// that takes an ortho projection and an RGBA color/alpha tint.
//
// Drawing a string is one buffer upload + one draw call.
class TextRenderer {
public:
    // basePath is the same path used by Game::getBasePath(); the shader files
    // are loaded relative to it (shaders/text.vert and shaders/text.frag).
    explicit TextRenderer(const std::string& basePath);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Sets the pixel-space ortho projection.  Call once per frame before
    // drawString.  Matches the projection used by Game::renderUI() so the
    // text shares the crosshair's coordinate system: origin top-left, Y down.
    void setProjection(const glm::mat4& projection);

    // Draws `text` with its top-left corner at (xPx, yPx).  `cellPx` is the
    // glyph cell size in pixels (e.g. 24 = 3x scale); kerning is implicit at
    // one cell per glyph.  `color` is multiplied by the glyph mask, so its
    // alpha drives fades.
    void drawString(const std::string& text,
                    float xPx,
                    float yPx,
                    float cellPx,
                    const glm::vec4& color);

    // Returns the pixel width of `text` at the given cell size.
    float measureWidth(const std::string& text, float cellPx) const;

    // Frees GL resources.  Called automatically by the destructor; Game also
    // calls it explicitly during shutdown for symmetry with other renderers.
    void release();

private:
    void buildFontTexture();
    void buildGLBuffers();

    GLuint fontTex = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;

    std::unique_ptr<Shader> shader;
    glm::mat4 projection{1.0f};
};

#endif
