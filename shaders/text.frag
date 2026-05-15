#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uFont;   // single-channel glyph mask (R8)
uniform vec4      uColor;  // tint + alpha multiplier

void main() {
    float mask = texture(uFont, TexCoord).r;
    if (mask < 0.5) discard;
    FragColor = uColor;
}
