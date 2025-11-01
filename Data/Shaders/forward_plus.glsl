#version 460 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normals;
out vec2 TextureCoords;

void main() {
    Normals       = mat3(transpose(inverse(model))) * aNormals;
    TextureCoords = aTextureCoords;
    FragPos       = vec3(model * vec4(aPosition, 1.0));
    gl_Position   = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
struct PointLight {
    vec4 color;
    vec4 position;
    vec4 paddingAndRadius;
};

struct VisibleIndex {
    int index;
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    PointLight data[];
}
lightBuffer;

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer {
    VisibleIndex data[];
}
visibleLightIndicesBuffer;

uniform int numberOfTilesX;
int lightCount = 1024;

out vec4 FragColor;

in vec3 Normals;
in vec2 TextureCoords;
in vec3 FragPos;

vec3 objectColor = vec3(0.5, 0.5, 0.5);

uniform vec3 viewPos;

void main() {
    ivec2 location = ivec2(gl_FragCoord.xy);
    ivec2 tileID   = location / ivec2(16, 16);
    uint index     = tileID.y * numberOfTilesX + tileID.x;

    vec3 normal = normalize(Normals);
    vec4 color  = vec4(0.0, 0.0, 0.0, 1.0);

    vec3 viewDirection = normalize(viewPos - FragPos);
    uint offset        = index * lightCount;

    for (uint i = 0; i < lightCount && visibleLightIndicesBuffer.data[offset + i].index != -1; i++) {
        int lightIndex     = visibleLightIndicesBuffer.data[offset + i].index;
        PointLight light   = lightBuffer.data[lightIndex];
        vec3 lightPosition = light.position.xyz;
        float lightRadius  = light.paddingAndRadius.w;
        vec4 lightColor    = light.color;

        vec3 lightDirection = normalize(lightPosition - FragPos);

        float distance = length(lightPosition - FragPos);

        float attenuation = 1.0 / (distance * distance);
        if (distance > lightRadius) {
            attenuation = 0.0;
        } else {
            attenuation *= 1.0 - (distance / lightRadius);
        }

        float diff   = max(dot(normal, lightDirection), 0.0);
        vec3 diffuse = diff * lightColor.rgb;

        vec3 viewDir    = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDirection, normal);
        float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular   = spec * spec * lightColor.rgb;

        float ambientStrength = 0.1;
        vec3 ambient          = ambientStrength * lightColor.rgb;

        vec3 irradiance = (ambient + diffuse + specular) * attenuation * 20;
        color.rgb += irradiance;
    }

    color.rgb += objectColor * vec3(0.2);

    const float gamma = 2.2;
    vec3 hdrColor     = color.rgb;
    vec3 mapped       = hdrColor / (hdrColor + vec3(1.0));
    mapped            = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}
#endif

#ifdef COMPUTE_SHADER
struct PointLight {
    vec4 color;
    vec4 position;
    vec4 paddingAndRadius;
};

struct VisibleIndex {
    int index;
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    PointLight data[];
}
lightBuffer;

layout(std430, binding = 1) writeonly buffer VisibleLightIndicesBuffer {
    VisibleIndex data[];
}
visibleLightIndicesBuffer;

uniform sampler2D depthMap;
uniform mat4 view;
uniform mat4 projection;
uniform ivec2 screenSize;
uniform int lightCount;

shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared vec4 frustumPlanes[6];

shared int visibleLightIndices[1024];
shared mat4 viewProjection;

#    define TILE_SIZE 16

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

void main() {
    ivec2 location   = ivec2(gl_GlobalInvocationID.xy);
    ivec2 itemID     = ivec2(gl_LocalInvocationID.xy);
    ivec2 tileID     = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint index       = tileID.y * tileNumber.x + tileID.x;

    minDepthInt       = 0xFFFFFFFF;
    maxDepthInt       = 0;
    visibleLightCount = 0;
    viewProjection    = projection * view;

    barrier();

    float maxDepth, minDepth;
    vec2 text   = vec2(location) / screenSize;
    float depth = texture(depthMap, text).r;

    depth = (0.5 * projection[3][2]) / (depth + 0.5 * projection[2][2] - 0.5);

    uint depthInt = floatBitsToUint(depth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);

    barrier();

    if (gl_LocalInvocationIndex == 0) {

        minDepth = uintBitsToFloat(minDepthInt);
        maxDepth = uintBitsToFloat(maxDepthInt);

        vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
        vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

        frustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x);
        frustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x);
        frustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y);
        frustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y);
        frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth);
        frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth);

        for (uint i = 0; i < 4; i++) {
            frustumPlanes[i] *= viewProjection;
            frustumPlanes[i] /= length(frustumPlanes[i].xyz);
        }

        frustumPlanes[4] *= view;
        frustumPlanes[4] /= length(frustumPlanes[4].xyz);
        frustumPlanes[5] *= view;
        frustumPlanes[5] /= length(frustumPlanes[5].xyz);
    }

    barrier();

    uint threadCount = TILE_SIZE * TILE_SIZE;
    uint passCount   = (lightCount + threadCount - 1) / threadCount;

    for (uint i = 0; i < passCount; i++) {
        uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
        if (lightIndex >= lightCount) {
            break;
        }

        vec4 position = lightBuffer.data[lightIndex].position;
        float radius  = lightBuffer.data[lightIndex].paddingAndRadius.w;

        float distance = 0.0;

        for (uint j = 0; j < 6; j++) {
            distance = dot(position, frustumPlanes[j]) + radius;

            if (distance <= 0.0) {
                break;
            }
        }

        if (distance > 0.0) {

            uint offset                 = atomicAdd(visibleLightCount, 1);
            visibleLightIndices[offset] = int(lightIndex);
        }
    }

    barrier();

    if (gl_LocalInvocationIndex == 0) {
        uint offset = index * lightCount;

        for (uint i = 0; i < visibleLightCount; i++) {
            visibleLightIndicesBuffer.data[offset + i].index = visibleLightIndices[i];
        }

        if (visibleLightCount != lightCount) {
            visibleLightIndicesBuffer.data[offset + visibleLightCount].index = -1;
        }
    }
}
#endif
