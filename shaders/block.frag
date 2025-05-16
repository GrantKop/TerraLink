#version 330 core

uniform sampler2D tex0;

uniform vec4 lightColor;
uniform vec3 fogColor;
uniform vec3 camPos;
uniform vec3 foliageColor;  // You can tint grass/leaves here
uniform float fogDensity;
uniform vec3 lightDir;      // New: directional light direction (normalized)

in vec3 normal;
in vec2 texCoord;
in vec3 curPos;
in vec3 fragWorldPos;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(tex0, texCoord);
    if (texColor.a < 0.05)
        discard;

    // Fog
    vec2 delta = fragWorldPos.xz - camPos.xz;
    float distance = length(delta);
    float fogStart = 225.0;
    float fogEnd = 265.0;

    float fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);

    // Lighting
    vec3 N = normalize(normal);
    vec3 L = normalize(-lightDir);  // Direction *to* light
    vec3 V = normalize(camPos - curPos);
    vec3 H = normalize(L + V);      // Halfway vector for specular

    float ambient = 0.4;
    float diffuse = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), 16.0);  // shininess factor = 16

    vec3 lighting = (ambient + diffuse + 0.5 * specular) * lightColor.rgb;

    // Optional foliage tint
    vec3 surfaceColor = texColor.rgb;
    float gray = dot(surfaceColor, vec3(0.299, 0.587, 0.114));
    if (texColor.g > texColor.r && texColor.g > texColor.b)
        surfaceColor = mix(vec3(gray), surfaceColor, 0.93);

    vec3 litColor = surfaceColor * lighting;

    // Mix with fog
    vec3 finalColor = mix(fogColor, litColor, fogFactor);
    FragColor = vec4(finalColor, texColor.a);
}
