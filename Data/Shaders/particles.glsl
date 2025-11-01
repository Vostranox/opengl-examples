#version 410 core

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 aPosition;

layout(std140) uniform OffsetBlock {
    vec4 offsets[500];
};

uniform mat4 model;
uniform mat4 lookAt;
uniform mat4 projection;

void main() {
    gl_Position = projection * lookAt * model * vec4(aPosition + offsets[gl_InstanceID].xyz, 1);
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(1, 1, 1, 1);
}
#endif
