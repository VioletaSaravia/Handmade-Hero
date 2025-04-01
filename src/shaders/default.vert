#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 texCoord;

uniform ivec2 res;
uniform vec2 pos;
uniform vec2 size;

void main() {
    vec2 pos_n = pos / res;
    vec2 size_n = size / res;

    texCoord = aTexCoord;

    gl_Position = vec4(
            aPos.x * size_n.x + pos_n.x * 2 - 1 + size_n.x,
            aPos.y * size_n.y - pos_n.y * 2 + 1 - size_n.y,
            aPos.z,
            1.0f
        );
}
