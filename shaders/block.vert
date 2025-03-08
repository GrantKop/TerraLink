#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTex;

uniform mat4 cameraMatrix;

out vec3 normal;
out vec2 texCoord;

void main()
{
    normal = aNorm;
    texCoord = aTex;

    gl_Position =  cameraMatrix * vec4(aPos, 1.0);
}
