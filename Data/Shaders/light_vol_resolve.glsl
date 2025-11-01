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

uniform sampler2D gAlbedoSpec;
uniform sampler2D lightAccum;

void main() {
    vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;
    vec3 Lo     = texture(lightAccum, TexCoords).rgb;

    vec3 ambient = vec3(0.04) * albedo;
    vec3 color   = ambient + Lo;
    color        = color / (color + vec3(1.0));
    color        = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
#endif
