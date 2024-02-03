#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 color;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal; // Transform normal to world space
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    color = vec3(1.0,1.0,1.0);
}

