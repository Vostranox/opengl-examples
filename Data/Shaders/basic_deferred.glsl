#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;
    TexCoords     = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal            = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

out vec4 FragCol;

void main() {
    gPosition   = vec4(FragPos, 1.0);
    gNormal     = vec4(normalize(Normal), metallic);
    gAlbedoSpec = vec4(albedo, roughness);
}
#endif
