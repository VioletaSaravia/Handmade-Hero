#version 460 core

// #+ BUFFER
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 texCoord;

uniform vec2 res;
uniform vec2 pos;
uniform vec2 size;
uniform float scale;
uniform float globalScale;
uniform vec2 camera;

void main() {
    vec2 pos_n = globalScale * (pos - camera) / res;
    vec2 size_n = globalScale * scale * size / res;

    texCoord = aTexCoord;

    gl_Position = vec4(
            aPos.x * size_n.x + pos_n.x * 2 - 1 + size_n.x,
            aPos.y * size_n.y - pos_n.y * 2 + 1 - size_n.y,
            aPos.z,
            1.0f
        );
}
