#version 460 core

// +BUFFER +INDEXED
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

// Globals
uniform int t;
uniform vec2 res;

uniform vec2 camPos;
uniform float camZoom;
uniform float camRotation;

uniform vec2 pos;
uniform vec2 size;
uniform vec4 color;

vec2 rotate(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

void main() {
    // Apply zoom
    float camScale = pow(0.5, camZoom);
    vec2 aPosCentered = aPos - camPos / res; // Shift everything relative to the camera
    vec2 aPosZoomed = aPosCentered / camScale;

    // Apply rotation
    float s = sin(-camRotation); // Negative to rotate the world opposite to camera
    float c = cos(-camRotation);
    vec2 aPosZoomedAndRotated = vec2(
            aPosZoomed.x * c - aPosZoomed.y * s,
            aPosZoomed.x * s + aPosZoomed.y * c
        );

    vec2 aspectRatio = rotate(vec2(res.x / res.y, 1.0), camRotation);
    vec2 camPosN = (camPos / res) * aspectRatio;
    vec2 aPosTransformed = aPosZoomedAndRotated - camPosN;

    gl_Position = vec4(aPosTransformed, 0.0, 1.0);

    vUV = aUV;
}
