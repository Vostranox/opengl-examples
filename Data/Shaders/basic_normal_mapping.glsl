#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTextureCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

out vec3 Normals;
out vec2 TextureCoords;
out vec3 FragPos;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

out vec3 v_lightPos;
out vec3 v_viewPos;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    Normals       = mat3(transpose(inverse(model))) * aNormals;
    TextureCoords = aTextureCoords;
    FragPos       = vec3(model * vec4(aPosition, 1.0));

    vec3 T   = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 N   = normalize(vec3(model * vec4(aNormals, 0.0)));
    T        = normalize(T - dot(T, N) * N);
    vec3 B   = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    TangentLightPos = TBN * lightPos;
    TangentViewPos  = TBN * viewPos;
    TangentFragPos  = TBN * FragPos;

    v_lightPos = lightPos;
    v_viewPos  = viewPos;

    gl_Position = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;

in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

in vec3 v_lightPos;
in vec3 v_viewPos;
vec3 lightColor  = vec3(1.0, 1.0, 1.0);
vec3 objectColor = vec3(1.0, 0.0, 0.0);

uniform float lightZ;
uniform float cameraZ;

uniform float spec;

uniform sampler2D texture01;
uniform sampler2D texture02;
uniform sampler2D texture03;

uniform int useNormalMap;
uniform float heightScale;

vec2 ParallaxMapping(vec2 texCoord, vec3 viewDirection) {
    const float minLayers   = 8.0f;
    const float maxLayers   = 64.0f;
    float numLayers         = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, -1.0f), viewDirection)));
    float layerDepth        = 1.0f / numLayers;
    float currentLayerDepth = 0.0f;

    vec2 S        = viewDirection.xy / viewDirection.z * heightScale;
    vec2 deltaUVs = S / numLayers;

    vec2 UVs                   = texCoord;
    float currentDepthMapValue = 1.0f - texture(texture03, UVs).r;

    while (currentLayerDepth < currentDepthMapValue) {
        UVs -= deltaUVs;
        currentDepthMapValue = 1.0f - texture(texture03, UVs).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = UVs + deltaUVs;
    float afterDepth   = currentDepthMapValue - currentLayerDepth;
    float beforeDepth  = 1.0f - texture(texture03, prevTexCoords).r - currentLayerDepth + layerDepth;
    float weight       = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + UVs * (1.0f - weight);
}

void main() {
    float ambientStrength = 0.1;
    vec3 ambient          = ambientStrength * lightColor;

    vec3 normal;
    if (useNormalMap == 1) {
        vec3 viewDir = normalize(TangentViewPos - TangentFragPos);

        vec2 texCoords = TextureCoords;
        texCoords      = ParallaxMapping(TextureCoords, viewDir);
        if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
            discard;
        }

        normal = texture(texture02, texCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);

        vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
        float diff    = max(dot(lightDir, normal), 0.0);
        vec3 diffuse  = diff * lightColor;

        vec3 reflectDir = reflect(-lightDir, normal);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specular  = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 result     = (ambient + diffuse + specular) * texture(texture01, texCoords).xyz;
        FragColor       = vec4(result, 1.0);

    } else {
        normal        = normalize(Normals);
        vec3 lightDir = normalize(v_lightPos - FragPos);
        float diff    = max(dot(normal, lightDir), 0.0);
        vec3 diffuse  = diff * lightColor;

        vec3 viewDir    = normalize(v_viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular   = spec * spec * lightColor;
        vec3 result     = (ambient + diffuse + specular) * texture(texture01, TextureCoords).xyz;
        FragColor       = vec4(result, 1.0);
    }
}
#endif
