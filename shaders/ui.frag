#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex0;

void main() {
    vec4 texColor = texture(tex0, TexCoord);
    if (texColor.a < 0.05)
        discard;
    FragColor = texColor;
}
