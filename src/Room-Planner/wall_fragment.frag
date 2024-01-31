#version 330 core
out vec4 FragColor;

in vec3 color;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 viewPos;
uniform vec3 lightColor1;
uniform vec3 lightColor2;

void main()
{    
    // First Light
    vec3 ambient1 = 0.1 * lightColor1;

    vec3 norm = normalize(Normal);
    vec3 lightDir1 = normalize(lightPos1 - FragPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * lightColor1;

    vec3 viewDir1 = normalize(viewPos - FragPos);
    vec3 reflectDir1 = reflect(-lightDir1, norm);
    float spec1 = pow(max(dot(viewDir1, reflectDir1), 0.0), 32.0);
    vec3 specular1 = spec1 * lightColor1;

    // Second Light
    vec3 ambient2 = 0.1 * lightColor2;

    vec3 lightDir2 = normalize(lightPos2 - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * lightColor2;

    vec3 viewDir2 = normalize(viewPos - FragPos);
    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec2 = pow(max(dot(viewDir2, reflectDir2), 0.0), 32.0);
    vec3 specular2 = spec2 * lightColor2;

    // Combine both lights
    vec3 result = (ambient1 + diffuse1 + specular1 + ambient2 + diffuse2 + specular2) * vec3(0.5, 0.5, 0.5); 

    FragColor = vec4(result, 1.0);
}