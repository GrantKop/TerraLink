#ifndef FONT_8X8_H
#define FONT_8X8_H

#include <cstdint>

// Tiny embedded 8x8 ASCII bitmap font used only by TextRenderer.
//
// The font has no external asset.  Glyphs are defined in Font8x8.cpp as
// 8-row strings using '#' for "pixel on", and buildAsciiBitmap() rasterises
// them into a 128-entry table that TextRenderer uploads to a single GL
// texture.
//
// Each entry in the output is 8 rows of 8 bits.  Bit 7 (MSB) of a row is the
// leftmost pixel; bit 0 is the rightmost.  Undefined codepoints are blank.
namespace terralink::font {

constexpr int GLYPH_W = 8;
constexpr int GLYPH_H = 8;
constexpr int GLYPH_COUNT = 128;

// Fills `out` with one 8-byte bitmap row pattern per ASCII codepoint.
void buildAsciiBitmap(std::uint8_t out[GLYPH_COUNT][GLYPH_H]);

} // namespace terralink::font

#endif
