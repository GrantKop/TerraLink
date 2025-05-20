#version 330 core

uniform sampler2D tex0;

uniform vec4 lightColor;
uniform vec3 fogColor;
uniform vec3 camPos;
uniform vec3 foliageColor;
uniform float fogDensity;
uniform vec3 lightDir;
uniform bool useFog;
uniform float fogStart;
uniform float fogEnd;
uniform float fogBottom;

in vec3 normal;
in vec2 texCoord;
in vec3 curPos;
in vec3 fragWorldPos;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(tex0, texCoord);
    if (texColor.a < 0.05)
        discard;

    // fog
    float distToCam = length(fragWorldPos.xz - camPos.xz); 
    float distFog = clamp((fogEnd - distToCam) / (fogEnd - fogStart), 0.0, 1.0);

    float fogBottomEnd = fogBottom + 40;
    float fogWallStrength = smoothstep(fogBottom, fogBottomEnd, camPos.y);

    float heightFade = clamp((fragWorldPos.y - camPos.y + 16.0) / 48.0, 0.0, 1.0);

    float heightFog = mix(1.0, heightFade, fogWallStrength);

    float fogFactor = min(distFog, heightFog);

    // Lighting
    vec3 N = normalize(normal);
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(camPos - curPos);
    vec3 H = normalize(L + V);

    float ambient = 0.4;
    float diffuse = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), 16.0);

    vec3 lighting = (ambient + diffuse + 0.5 * specular) * lightColor.rgb;

    vec3 surfaceColor = texColor.rgb;
    float gray = dot(surfaceColor, vec3(0.33, 0.63, 0.145));
    if (texColor.g > texColor.r && texColor.g > texColor.b)
        surfaceColor = mix(vec3(gray), surfaceColor, 0.96);

    vec3 litColor = surfaceColor * lighting;

    vec3 finalColor;

    if (useFog) {
        finalColor = mix(fogColor, litColor, fogFactor);
    } else {
        finalColor = mix(fogColor, litColor, 1.0);
    }
    FragColor = vec4(finalColor, texColor.a);
}
