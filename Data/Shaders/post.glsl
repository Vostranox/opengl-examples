#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTextureCoords;

out vec2 TextureCoords;

void main() {
    TextureCoords = aTextureCoords;
    gl_Position   = vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec2 TextureCoords;

uniform sampler2D texture01;

void main() {
    vec4 col  = texture(texture01, TextureCoords);
    FragColor = vec4(col.x, col.y, col.z, 1.0);
}
#endif
