#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords   = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

const int lightNumber = 500;
layout(std140) uniform LightBlock {
    vec4 lightPositions[lightNumber];
    vec4 lightColor[lightNumber];
};
uniform vec3 lightDir;
uniform vec3 camPos;

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

uniform float radius;
uniform vec3 attenuation;

void main() {
    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal   = texture(gNormal, TexCoords).rgb;

    if (Normal == vec3(0.0)) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;

    float roughness = texture(gAlbedoSpec, TexCoords).a;
    float metallic  = texture(gNormal, TexCoords).a;

    vec3 N = Normal;
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    float radius2 = radius * radius;
    for (int i = 0; i < lightNumber; ++i) {
        vec3 toLight = lightPositions[i].xyz - WorldPos;
        float dist2  = dot(toLight, toLight);

        if (dist2 < radius2) {
            float dist = sqrt(dist2);
            vec3 L     = toLight / dist;
            vec3 H     = normalize(V + L);

            float atten = 1.0 / (attenuation.x + attenuation.y * dist + attenuation.z * dist * dist);

            vec3 halfway = normalize(N + L);
            float NdotL  = max(dot(Normal, L), 0.00001);
            float NdotH  = max(dot(Normal, halfway), 0.00001);
            float VdotH  = max(dot(N, halfway), 0.00001);

            float D = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

            vec3 diffuse  = (1.0 - F) * albedo / PI;
            vec3 specular = F * G * D / (4.0 * NdotL * max(NdotH, 0.00001));

            Lo += atten * 1.0 * NdotL * (diffuse + specular) * lightColor[i].rgb;
        }
    }

    vec3 ambient = vec3(0.04) * albedo;
    vec3 color   = ambient + Lo;
    color        = color / (color + vec3(1.0));
    color        = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
#endif
