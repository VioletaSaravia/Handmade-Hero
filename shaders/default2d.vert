#version 460 core

// +BUFFER +INDEXED
layout(location = 0) in vec2 aPos;
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

mat2 rotate(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

void main() {
    vUV = aUV;

    // Object transform
    vec2 local = aPos * size;
    local = rotate(rotation) * local;
    vec2 world = pos + local;

    // Camera transform
    vec2 view = (world - camPos) * (pow(2, camZoom));
    view = rotate(-camRotation) * view;

    // Normalize to [-1, 1] (NDC)
    vec2 ndc = world / res * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
}
