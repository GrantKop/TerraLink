#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 model;
uniform mat4 cameraMatrix;

out vec4 location;
out vec3 Normal;
out vec3 camPos;
out vec3 pos;

void main()
{
    location = vec4(aPos.x + 1., aPos.y + 1., aPos.z + 1, 1.0);
    Normal = aNorm;
    camPos = vec3(cameraMatrix);
    pos = vec3(model * vec4(aPos, 1.0f));
    gl_Position =  cameraMatrix * vec4(aPos, 1.0);
}
