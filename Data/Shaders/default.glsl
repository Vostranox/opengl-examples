#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTextureCoords;

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

out vec2 TextureCoords;

void main() {
    TextureCoords = aTextureCoords;
    gl_Position   = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 FragColor;

in vec2 TextureCoords;

uniform sampler2D texture01;
uniform sampler2D texture02;
uniform bool drawTexture;

void main() {
    if (drawTexture == true) {
        FragColor = texture(texture01, TextureCoords);
    } else {
        FragColor = texture(texture02, TextureCoords);
    }
}
#endif
