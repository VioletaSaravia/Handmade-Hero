#version 460 core

// +BUFFER +INDEXED
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

// Globals
uniform int t;
uniform vec2 res;

// Camera
uniform vec2 camPos;
uniform float camZoom;
uniform float camRotation;

// Locals
uniform vec2 pos;
uniform vec2 size;
uniform float rotation;

void main() {
    vec2 pos_n = (pos - camPos) / res;
    vec2 size_n = (camZoom + 1) * size / res;

    vUV = aUV;

    gl_Position = vec4(
            aPos.x * size_n.x + pos_n.x * 2 - 1 + size_n.x,
            aPos.y * size_n.y - pos_n.y * 2 + 1 - size_n.y,
            aPos.z,
            1.0f
        );
}
