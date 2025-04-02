#version 330 core

uniform sampler2D tex0;

uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

in vec3 normal;
in vec2 texCoord;
in vec3 curPos;

out vec4 FragColor;

void main()
{
    float ambient = 0.6;

    vec3 Normal = normalize(normal);
    vec3 lightDirection = normalize(lightPos - curPos);
    float diffuse = max(dot(Normal, lightDirection), 0.0);

    float specularLight = 0.5;
    vec3 viewDirection = normalize(camPos - curPos);

    vec3 halfwayVec = normalize(viewDirection + lightDirection);

    float specAmount = pow(max(dot(Normal, halfwayVec), 0.0), 8);
    float specular = specAmount * specularLight;

    FragColor = texture(tex0, texCoord) * lightColor * (diffuse + ambient + specular);
    //FragColor = texture(tex0, texCoord);
} 
