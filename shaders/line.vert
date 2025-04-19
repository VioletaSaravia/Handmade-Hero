#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

uniform vec2 pos;
uniform vec2 size;
uniform vec2 res;
uniform vec2 camera;

void main() {
    // Map aPos from [-1, 1] to [0, 1]
    vec2 pos01 = (aPos + 1.0) * 0.5;

    // Scale and offset in pixel space
    vec2 posPixels = pos - camera + pos01 * size;

    // Convert to NDC
    vec2 posNDC = (posPixels / res) * 2.0 - 1.0;

    // Flip Y if needed (OpenGL NDC Y=+1 is top)
    posNDC.y = -posNDC.y;

    gl_Position = vec4(posNDC, 0.0, 1.0);

    vUV = (aPos + 1.0) * 0.5; // Map to [0, 1]
}

