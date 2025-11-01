#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;

uniform mat4 projection;
uniform mat4 lookAt;
uniform mat4 model;

void main() {
    gl_Position = projection * lookAt * model * vec4(aPosition, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
void main() {}
#endif
