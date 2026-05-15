# Selected-block HUD & held block

Two related bits of player feedback for the currently selected (to-be-placed)
block:

1. **Name HUD** — a label, centered on screen, showing the block's
   human-readable name. It holds at full opacity for ~2 seconds, then fades
   out over the following ~1 second (3 seconds total) and stays hidden until a
   different block is selected.
2. **Held block** — the selected block drawn as a small 3D cube in the
   bottom-right corner, so it always reflects what right-click will place
   (similar to a Minecraft held item).

## Name HUD — how it works

The feature is split into three independent pieces so that block-name lookup,
fade/state logic, and rendering stay separate:

1. **Selection (unchanged engine code).** `Player::selectedBlockID` is the
   single source of truth. It is changed only by the existing mouse-scroll
   handler (`scrollCallback`) and the middle-click pick handler in
   `src/core/player/Player.cpp`. The HUD only *reads* this value; selection
   behaviour was not modified.

2. **Lookup + fade state (`Game`).**
   `Game::updateSelectedBlockHUD()` (called once per frame from
   `Game::render()`) detects when `selectedBlockID` changes, resolves the name
   via the existing registry — `BlockRegister::getBlockByIndex(id).name`
   (e.g. `"Cobble Stone"`) — and restarts the fade timer. The timer/alpha math
   lives here, not in the renderer. Constants `Game::HUD_HOLD_TIME` (2.0s) and
   `Game::HUD_FADE_TIME` (3.0s) control the timing.

3. **Rendering (`TextRenderer`).** The project had no text renderer, so a
   minimal one was added: `include/graphics/TextRenderer.h` /
   `src/graphics/TextRenderer.cpp`. At startup it bakes the embedded
   public-domain 8x8 bitmap font (`include/graphics/Font8x8.h`) into a single
   single-channel glyph-atlas texture, then draws one quad per character.
   It reuses the existing `Shader` / `VertexArrayObject` helpers and the
   existing UI vertex shader (`shaders/ui.vert`); the only new shader is
   `shaders/text.frag`, which multiplies glyph coverage by a `uColor` (RGBA)
   uniform so the label can be tinted and faded. `TextRenderer` knows nothing
   about blocks — it only draws `std::string`s.

`Game::renderSelectedBlockHUD()` is invoked from `Game::renderUI()` while
alpha blending is enabled (right after the crosshair), so it does not alter
the engine or chunk-rendering pipeline.

### Files touched

| File | Purpose |
|---|---|
| `include/graphics/Font8x8.h` | Embedded public-domain 8x8 font (new) |
| `include/graphics/TextRenderer.h`, `src/graphics/TextRenderer.cpp` | Minimal bitmap text renderer (new) |
| `shaders/text.frag` | Tint/alpha fragment shader, reuses `ui.vert` (new) |
| `include/core/game/Game.h`, `src/core/game/Game.cpp` | HUD state, lookup, per-frame update, draw call |
| `docs/selected-block-hud.md` | This note (new) |

## Held block — how it works

The held block reuses the **existing** world block pipeline rather than adding
a new one:

- Every `Block` already carries a render-ready vertex list
  (`Block::vertices`): positions/normals from the model `.obj` and atlas UVs
  baked in by `Atlas::linkBlocksToAtlas`. The chunk mesher turns those quads
  into triangles with a fixed winding (`{o, o+2, o+1, o, o+3, o+2}`).
- `Game::buildHeldBlockMesh()` copies that vertex list and generates the same
  quad indices into a dedicated VAO. It runs only when
  `Player::selectedBlockID` changes (cached in `Game::heldBlockID`).
- `Game::renderHeldBlock()` (called at the end of `Game::render()`) draws that
  VAO with the **same** `block.vert` / `block.frag` shader and the same atlas
  texture. The only difference from world geometry is the transforms: instead
  of the world camera it uses a small private `glm::perspective` projection and
  a fixed model matrix that places the cube down-right, scales it, and tilts it
  ~45deg/-30deg so three faces are visible. Because it does not use the world
  view matrix, the cube stays anchored to the screen no matter where the player
  looks or moves.
- `glm::perspective`'s FOV is *vertical*, so the horizontal frustum width
  depends on the window aspect ratio. Rather than a hardcoded eye-space x/y
  (which drifts off a narrow/square window), the cube is anchored a fixed
  inset in from the visible frustum edge at its depth
  (`halfW = depth * tan(fovY/2) * aspect`). Its on-screen position is therefore
  identical at any window size or aspect ratio.
- Depth is cleared (`glClear(GL_DEPTH_BUFFER_BIT)`) right before the draw so
  the cube sits on top of the world without altering any world colors; the
  global depth test + face culling then let the cube self-sort normally. Fog
  is disabled for this one draw and immediately restored.

This adds no new shader, texture, or source file — only methods on `Game` — so
the engine and chunk-rendering pipeline are untouched.

### Files touched (held block)

| File | Purpose |
|---|---|
| `include/core/game/Game.h`, `src/core/game/Game.cpp` | `heldBlockVAO`/cache, `buildHeldBlockMesh()`, `renderHeldBlock()`, draw call + cleanup |

## Building

CMake discovers sources with `file(GLOB_RECURSE ...)`, so the new
`TextRenderer.cpp` is only picked up after CMake re-runs. Use a target that
reconfigures:

```bash
make all        # or: make configure && make build  (NOT just `make build`)
make run
```

`shaders/` is installed wholesale, so `text.frag` ships in packaged builds
with no extra wiring.

## Manual test

1. `make all && make run` and spawn into a world.
2. Scroll the mouse wheel. On each change a centered label appears (e.g.
   `Stone`, `Dirt`, `Birch Planks`, `Cobble Stone`) matching the
   `Selected Block ID:` line printed to stdout.
3. Stop scrolling and watch: the label stays solid for ~2s, then fades out
   over ~1s and disappears (gone by ~3s).
4. Middle-click a placed block to pick it. The label updates immediately to
   that block's name and the 3-second show/fade cycle restarts.
5. Select the same block again (no actual change) — the label does **not**
   re-trigger, confirming it only reacts to genuine selection changes.
6. Verify the label is horizontally centered and readable against the bright
   sky (it is drawn with a dark drop shadow).
7. Confirm a small 3D block sits in the bottom-right corner showing the
   selected block's textures (e.g. grass top/side), tilted so three faces are
   visible.
8. Scroll / middle-click to change the selection: the corner block swaps to
   the new block immediately and matches the name label.
9. Look and walk around: the corner block stays anchored to the screen (it is
   not part of the world) and is never fogged, even far out or underground.
10. Right-click to place a block and confirm it places the same block shown in
    the corner.
11. Toggle fullscreen (F11) and resize the window between square and
    widescreen: the corner block stays fully on screen in the same relative
    bottom-right spot at every aspect ratio.

## Notes / limitations

- Monospaced 8x8 font, white with a drop shadow. Glyph cell height scales with
  window height (~3.5%).
- Names come straight from the block registry (`registry/block_registry.json`
  / `assets/maps/blocks/*.json`); if a name is missing it falls back to
  `Block <id>`.
- The held block is static (no walk/swing bob). The corner insets
  (`insetX`/`insetY`), `depth`, scale and tilt are constants in
  `Game::renderHeldBlock()` and are easy to tune; the insets are in eye-space
  units measured in from the visible frustum edge, so tuning them holds across
  window sizes. Cross/plant models (e.g. Grass Plant, Dead Bush) are quad-based too, so
  they render as their crossed-plane sprites in hand — the same way they look
  in the world.
- This environment (Linux, no vcpkg/Windows toolchain) cannot compile or run
  the OpenGL client, so the steps above are the intended verification path on
  a normal Windows dev setup.
