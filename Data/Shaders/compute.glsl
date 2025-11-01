#version 460 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

out vec3 Normals;
out vec2 TextureCoords;
out vec3 FragPos;

void main() {
    Normals       = mat3(transpose(inverse(model))) * aNormals;
    TextureCoords = aTextureCoords;
    FragPos       = vec3(model * vec4(aPosition, 1.0));
    gl_Position   = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;

vec3 lightColor  = vec3(1.0, 1.0, 1.0);
vec3 objectColor = vec3(1.0, 0.0, 0.0);

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    float ambientStrength = 0.1;
    vec3 ambient          = ambientStrength * lightColor;

    vec3 norm     = normalize(Normals);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;

    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular   = spec * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor   = vec4(result, 1.0);
}
#endif

#ifdef COMPUTE_SHADER
layout(local_size_x = 2, local_size_y = 2, local_size_z = 1) in;

layout(std430) buffer Buffer {
    uint val[16];
};

shared uint total;

void main() {
    uint index  = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_WorkGroupSize.y * gl_NumWorkGroups.y;
    uint value  = gl_LocalInvocationID.x + gl_WorkGroupID.x * gl_WorkGroupSize.x;
    uint value2 = gl_LocalInvocationID.y + gl_WorkGroupID.y * gl_WorkGroupSize.y;

    val[index] = value + value2 * (gl_WorkGroupSize.y * gl_NumWorkGroups.y);
}
#endif
