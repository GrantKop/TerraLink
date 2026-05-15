#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <string>
#include <memory>
#include <glm/glm.hpp>

#include "graphics/Shader.h"
#include "graphics/VertexArrayObject.h"

// Minimal bitmap-font text renderer.
//
// Deliberately tiny: the project has no text renderer, so this draws strings
// using the embedded 8x8 font (see Font8x8.h) as a single glyph-atlas texture
// and one quad per character. It reuses the existing Shader / VertexArrayObject
// helpers and the UI vertex shader -- it does not touch the engine or the
// chunk/UI rendering pipeline.
//
// It knows nothing about blocks, the player, or game state: callers pass in a
// finished std::string. Block-name lookup stays in the game layer.
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Builds the glyph atlas + shader. Requires a current GL context.
    // basePath is Game::getBasePath(); shaders are loaded from basePath/shaders.
    void init(const std::string& basePath);

    // Draws `text` with its top-left corner at (x, y), in the supplied
    // orthographic projection (same pixel space the UI uses). `pixelHeight`
    // is the on-screen height of one glyph cell. `color` is RGBA; the alpha
    // channel is what the HUD fade animates.
    void drawText(const std::string& text, float x, float y, float pixelHeight,
                  const glm::vec4& color, const glm::mat4& projection);

    // Pixel width a string occupies at the given pixelHeight (monospace).
    float measureWidth(const std::string& text, float pixelHeight) const;

    void shutdown();

    bool isReady() const { return ready; }

private:
    static const int GLYPH = 8;   // each glyph is 8x8 px
    static const int COLS  = 16;  // atlas is a 16 x 8 grid (128 glyphs)
    static const int ROWS  = 8;

    GLuint atlasTex = 0;
    int atlasW = 0;
    int atlasH = 0;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<VertexArrayObject> vao;
    bool ready = false;

    void buildAtlasTexture();
};

#endif
