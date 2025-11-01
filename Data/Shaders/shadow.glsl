#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;
uniform mat4 lightProjView;

out vec3 Normals;
out vec2 TextureCoords;
out vec3 FragPos;
out vec4 FragPosLightSpace;

void main() {
    Normals           = mat3(transpose(inverse(model))) * aNormals;
    TextureCoords     = aTextureCoords;
    FragPos           = vec3(model * vec4(aPosition, 1.0));
    FragPosLightSpace = lightProjView * model * vec4(aPosition, 1.0);
    gl_Position       = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;
in vec4 FragPosLightSpace;

vec3 lightColor  = vec3(1.0, 1.0, 1.0);
vec3 objectColor = vec3(1.0, 0.0, 0.0);

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D shadowMap;

float Shadow() {
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias   = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main() {
    float ambientStrength = 0.1;
    vec3 ambient          = ambientStrength * lightColor;

    vec3 norm     = normalize(Normals);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir  = normalize(viewPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;

    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular   = spec * spec * lightColor;

    float shadow = Shadow();

    vec3 temp = (1.0 - shadow) * (diffuse + specular);

    vec3 result = (ambient + temp) * objectColor;
    FragColor   = vec4(result, 1.0);
}
#endif
