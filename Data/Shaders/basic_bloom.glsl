#version 410 core

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
#    ifdef EMISSIVE
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;

vec3 lightColor = vec3(1.0, 0.0, 0.0);

uniform float lightIntensity;

void main() {
    FragColor        = vec4(lightColor, 1.0);
    float brightness = dot(FragColor.rgb * lightIntensity, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(FragColor.rgb * lightIntensity, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
#    else
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;

vec3 lightColor  = vec3(1.0, 1.0, 1.0);
vec3 objectColor = vec3(1.0, 0.0, 0.0);

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float spec;

uniform float lightIntensity;

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

    vec3 result = lightIntensity * (ambient + diffuse + specular) * objectColor;

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    FragColor = vec4(result, 1.0);
}
#    endif
#endif
