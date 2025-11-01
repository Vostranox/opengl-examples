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

uniform sampler2D scene;
uniform sampler2D bloomBlur;

uniform float exposure;
uniform float bloomStrength = 0.04f;
uniform int programChoice;

vec3 bloom_none() {
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    return hdrColor;
}

vec3 bloom_old() {
    vec3 hdrColor   = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    return hdrColor + bloomColor; // additive blending
}

vec3 bloom_new() {
    vec3 hdrColor   = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    return mix(hdrColor, bloomColor, bloomStrength); // linear interpolation
}

void main() {
    vec3 result       = vec3(0.0);
    result            = bloom_new();
    result            = vec3(1.0) - exp(-result * exposure);
    const float gamma = 2.2;
    result            = pow(result, vec3(1.0 / gamma));
    FragColor         = vec4(result, 1.0);
}
#endif
