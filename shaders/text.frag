#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

// Single-channel glyph atlas: .r is glyph coverage (1 = ink, 0 = empty).
uniform sampler2D tex0;
// Text tint + alpha. Alpha is driven by the HUD fade timer.
uniform vec4 uColor;

void main() {
    float coverage = texture(tex0, TexCoord).r;
    if (coverage < 0.01)
        discard;
    FragColor = vec4(uColor.rgb, uColor.a * coverage);
}
