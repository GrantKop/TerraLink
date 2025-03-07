#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 cameraMatrix;

out vec3 normal;

void main()
{
    normal = aNorm;

    gl_Position =  cameraMatrix * vec4(aPos, 1.0);
}
