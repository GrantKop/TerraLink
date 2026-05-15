#ifndef HELD_BLOCK_HUD_H
#define HELD_BLOCK_HUD_H

#include <glad/glad.h>

#include "graphics/Shader.h"

// 3D "held block" preview in the corner of the screen, Minecraft-style.
//
// Reuses the existing block shader and block atlas: the per-block geometry
// (positions + normals + atlas-mapped UVs) lives on every Block, so we just
// upload that into our own VAO/EBO and draw it through the same world shader
// with a tiny corner camera in its own viewport.
//
// The GL buffers are only rebuilt when the selected block ID changes; static
// frames pay one glDrawElements call and a few uniform writes.
class HeldBlockHUD {
public:
    HeldBlockHUD();
    ~HeldBlockHUD();

    HeldBlockHUD(const HeldBlockHUD&) = delete;
    HeldBlockHUD& operator=(const HeldBlockHUD&) = delete;

    // Rebuilds the GL buffers from BlockRegister if the ID has changed since
    // the previous call.  Safe to call every frame.
    void update(int selectedBlockID);

    // Draws the held block into a square viewport whose bottom-left corner is
    // (xPx, yPx) in framebuffer pixels.  Saves and restores the previous
    // viewport; enables depth test for the cube and disables it on exit; the
    // depth buffer is cleared before drawing so the cube sits on top of the
    // world.
    //
    // blockShader must accept "cameraMatrix", "model", and "camPos" uniforms
    // (the same uniforms the chunk pass uses).  The atlas texture must be
    // bound to the slot the shader's tex0 sampler points at *before* calling
    // this; the caller already has that texture handy in Game.
    void draw(Shader& blockShader, int xPx, int yPx, int sizePx);

    void release();

private:
    void rebuildFor(int blockID);

    int     lastBuiltID = -2; // -2 forces the first update() to rebuild
    GLuint  VAO = 0;
    GLuint  VBO = 0;
    GLuint  EBO = 0;
    GLsizei indexCount = 0;
};

#endif
