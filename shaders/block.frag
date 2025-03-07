#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;

in vec4 location;
in vec3 Normal;
in vec3 camPos;
in vec3 pos;

void main()
{
    vec3 lightVec = vec3(0., 1.5, 0.) - pos;
    float distance = length(lightVec);
    float a = .45;
    float b = 1.0;
    float intensity = 1.0 / (a * distance * (distance + b) * (distance + 1.0));
    intensity = clamp(intensity, 0.1, 1.0);

    float ambient = .2;

    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(lightVec);
    float diffuse = max(dot(normal, lightDirection), 0.0);

    float specularLight = 0.6;
    vec3 viewDirection = normalize(camPos - pos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0), 8);
    float specular = specAmount * specularLight;

    //FragColor = normalize(location) * (vec4(lightColor, 1.0) * ((diffuse + specular) * intensity + ambient));
    FragColor = vec4(normalize(Normal), 1.0);
} 
