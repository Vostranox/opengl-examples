#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

out vec3 Normal;
out vec2 TextureCoords;
out vec3 WorldPos;

void main() {
    Normal        = mat3(transpose(inverse(model))) * aNormals;
    TextureCoords = aTextureCoords;
    WorldPos      = vec3(model * vec4(aPosition, 1.0));
    gl_Position   = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec3 Normal;
in vec2 TextureCoords;
in vec3 WorldPos;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform vec3 lightPositions;
uniform vec3 lightDir;
uniform vec3 camPos;

uniform float lightCutoffAngle;
uniform float innerCone;

uniform int lightType;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateDirectionalLight(vec3 N, vec3 V, vec3 F0) {
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);

    vec3 halfway = normalize(N - lightDir);

    float NdotL = max(dot(Normal, lightDir), 0.00001);
    float NdotH = max(dot(Normal, halfway), 0.00001);
    float VdotH = max(dot(N, halfway), 0.00001);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 diffuse  = (1.0 - F) * albedo / PI;
    vec3 specular = F * G * D / (4.0 * NdotL * max(NdotH, 0.00001));
    return 1.0 * NdotL * (diffuse + specular) * vec3(1.0, 1.0, 1.0);
}

vec3 CalculatePointLight(vec3 N, vec3 V, vec3 F0) {
    vec3 L = normalize(lightPositions - WorldPos);
    vec3 H = normalize(V + L);

    float dist  = length(lightPositions - WorldPos);
    float atten = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);

    vec3 halfway = normalize(N + L);
    float NdotL  = max(dot(Normal, L), 0.00001);
    float NdotH  = max(dot(Normal, halfway), 0.00001);
    float VdotH  = max(dot(N, halfway), 0.00001);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 diffuse  = (1.0 - F) * albedo / PI;
    vec3 specular = F * G * D / (4.0 * NdotL * max(NdotH, 0.00001));

    return atten * 1.0 * NdotL * (diffuse + specular) * vec3(1.0, 1.0, 1.0);
}

vec3 CalculateSpotLight(vec3 N, vec3 V, vec3 F0) {
    vec3 L = normalize(lightPositions - WorldPos);
    vec3 H = normalize(V + L);

    float dist  = length(lightPositions - WorldPos);
    float atten = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);

    float cosAngle         = dot(L, lightDir);
    float outerConeCos     = cos(radians(lightCutoffAngle));
    float innerConeCos     = cos(radians(lightCutoffAngle * innerCone));
    float deltaCos         = (outerConeCos - innerConeCos);
    float smoothstepFactor = smoothstep(outerConeCos, outerConeCos - deltaCos, cosAngle);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 halfway = normalize(N + lightDir);
    float NdotL  = max(dot(N, L), 0.00001);
    float NdotH  = max(dot(Normal, halfway), 0.00001);

    vec3 diffuse  = (1.0 - F) * albedo / PI;
    vec3 specular = F * G * D / (4.0 * NdotL * max(NdotH, 0.00001));

    return atten * 1.0 * NdotL * (diffuse + specular) * vec3(1.0, 1.0, 1.0) * smoothstepFactor;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    if (lightType == 0) {
        Lo += CalculateDirectionalLight(N, V, F0);
    } else if (lightType == 1) {
        Lo += CalculateSpotLight(N, V, F0);
    } else if (lightType == 2) {
        Lo += CalculatePointLight(N, V, F0);
    }

    vec3 ambient = vec3(0.04) * albedo * ao;
    vec3 color   = ambient + Lo;
    color        = color / (color + vec3(1.0));
    color        = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
#endif
