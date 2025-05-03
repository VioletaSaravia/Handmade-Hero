#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

// Globals
uniform int t;
uniform vec2 res;

uniform vec2 camPos;
uniform float camZoom;
uniform float camRotation;

// Locals
uniform vec2 pos;
uniform vec2 size;
uniform float rotation;

vec2 rotate(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

void main() {
    float c = cos(rotation);
    float s = sin(rotation);
    mat2 rot = mat2(c, -s, s, c);
    vec2 rotatedPos = rot * aPos;

    // Map aPos from [-1, 1] to [0, 1]
    vec2 pos01 = (rotatedPos + 1.0) * 0.5;

    // Scale and offset in pixel space
    vec2 posPixels = pos - camPos + pos01 * size;

    // Convert to NDC
    vec2 posNDC = (posPixels / res) * 2.0 - 1.0;

    // Flip Y if needed (OpenGL NDC Y=+1 is top)
    posNDC.y = -posNDC.y;

    gl_Position = vec4(posNDC, 0.0, 1.0);

    // vUV = (aPos + 1.0) * 0.5; // Map to [0, 1]
    vUV = aUV;
}
