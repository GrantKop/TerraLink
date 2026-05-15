# TerraLink

**TerraLink** is a procedural terrain engine written in C++ with OpenGL. It features real-time chunk generation, multithreaded streaming, and multiplayer networking using UDP/TCP. Designed to be scalable and performant, TerraLink is a project I completed for 5 credit hours that explores the combination of procedural graphics, multithreading, and networking.

I have spent around 12 weeks and 350+ hours on this project in total.

If you are interested in the technical side of the systems I built, here is the [wiki](https://github.com/GrantKop/TerraLink/wiki)

---

## Features

- 🌍 Procedural terrain generation using Perlin noise
- 🧵 Multithreaded background chunk creation, meshing, and uploading with thread-safe queues to minimize mutex locking
- 🌐 Multiplayer support with UDP chunk streaming and TCP client tracking
- 💾 Persistent chunk saving and loading using Zstandard compression
- 🎮 OpenGL-based rendering using a handful of shader programs
- 🏔️ Multi-Biome blending and domain-warped noise to prevent jumps in terrain
- 🔊 Ambient music and sound effects using OpenAL
- 🏷️ Selected-block name HUD that fades after 3 seconds
- 📦 3D "held block" preview in the bottom-right corner

---

## Selected-block name HUD

Whenever the player changes their currently selected block, the block's
human-readable name (e.g. "Birch Log", "Cobble Stone") is drawn near the
bottom of the screen and fades out over 3 seconds — 2 seconds solid plus a
1-second linear fade. Picking a different block immediately resets the timer
and swaps the label text.

A small 3D preview of the same block is also drawn in the bottom-right
corner, similar to Minecraft's held-item slot. It always reflects whatever
block would be placed by a right-click, so it stays visible (no fade) and
updates instantly when the selection changes.

**How it's wired together**

- Block selection itself is unchanged: `Player::selectedBlockID` is still
  driven by the scroll wheel (`scrollCallback`) and middle mouse
  (`Player::handleInput`).
- `include/core/registers/BlockNameLookup.h` (`blockNameById`) wraps
  `BlockRegister::getBlockByIndex(id).name`, keeping the name lookup separate
  from any rendering code.
- `include/core/ui/BlockNameHUD.h` owns the timer + last-seen-ID state and
  produces a 0–1 alpha. It contains no GL calls and is straightforward to
  unit-test.
- `include/core/ui/TextRenderer.h` draws the label using an embedded 8×8
  bitmap font (see `include/core/ui/Font8x8.h`). At construction it builds a
  single R8 glyph atlas texture, and `drawString` batches each call into one
  dynamic VBO upload + one `glDrawArrays`. The minimal text shader lives at
  `shaders/text.vert` / `shaders/text.frag`.
- `Game::renderUI()` ticks the HUD with the current `selectedBlockID` and,
  while it is visible, draws the label centered horizontally at roughly 85%
  down the screen. The crosshair draw above is untouched.
- `include/core/ui/HeldBlockHUD.h` owns the held-block preview. It rebuilds a
  small VAO/EBO from `Block::vertices` only when the selected ID changes
  (one upload per scroll/middle-click), then draws into a square viewport
  in the bottom-right using the existing `block` shader and atlas with a
  corner-view perspective camera. Static frames cost one `glDrawElements`
  call and a few uniform writes — no new shader, no new texture.

**Manual test**

1. `make run` (DEV_MODE must be `TRUE` in `src/main.cpp`).
2. After spawn, the label for the default selected block fades in/out once,
   and the held-block preview is visible in the bottom-right corner.
3. Scroll the mouse wheel — the label for each new block appears, stays solid
   for ~2 seconds, then fades out over the next ~1 second. The bottom-right
   preview swaps to the new block on the same frame and stays visible.
4. Aim at any placed block and middle-click — the picked block's name
   replaces the label, the timer resets, and the held-block preview updates.
5. Wait 3 seconds without changing selection; the label fully disappears,
   while the held-block preview remains.
6. Confirm the held-block preview shows three faces (top + right + front)
   shaded by the world's directional light. Non-cube blocks like grass
   plants render as their actual crossed-plane geometry.

---

## Screenshots
![image](https://github.com/user-attachments/assets/04f9812d-8f4a-45aa-b12b-3bd1721c6117)
![image](https://github.com/user-attachments/assets/f72bb1cd-3d2f-462f-9bd3-76ea93c215fa)
![image](https://github.com/user-attachments/assets/1d3ff36a-a5cc-4768-943a-8f588c9961e7)
![image](https://github.com/user-attachments/assets/43e0031b-99d4-4b7e-89e8-b8b0fa2fcd64)

---

## Prerequisites

- [Make](https://gnuwin32.sourceforge.net/packages/make.htm)
- [CMake](https://cmake.org/)
- [Git](https://git-scm.com/)
- [CPack](https://cmake.org/cmake/help/latest/module/CPack.html)

All other dependencies (GLFW, GLAD, GLM, stb, etc.) are installed automatically using **[vcpkg](https://github.com/microsoft/vcpkg)**.

---

## Building the Project

Clone the repository and use the provided `Makefile`:

```bash
# Install dependencies, configure and build
make setup

# Run the executable (inside src/main.cpp, DEV_MODE must be set to TRUE)
make run

# If you want to make the installer, inside src/main.cpp, DEV_MODE must be set to FALSE
make installer
