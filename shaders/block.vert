#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTex;

uniform mat4 cameraMatrix;
uniform mat4 model;

out vec3 normal;
out vec2 texCoord;
out vec3 fragWorldPos;

void main()
{
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    fragWorldPos = worldPos;
    normal = aNorm;
    texCoord = aTex;

    gl_Position = cameraMatrix * vec4(worldPos, 1.0);
}
