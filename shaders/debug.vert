#version 460 core

// #+ BUFFER
layout(location = 0) in vec2 aPos;

// #+ BUFFER DIV
layout(location = 1) in int type;
layout(location = 2) in vec2 pos;
layout(location = 3) in vec2 size;
layout(location = 4) in vec4 color;

out vec2 vUV; // ???

out vec2 fragPos;
out vec2 fragSize;
out vec4 fragColor;
out int fragType;

uniform vec2 res;
uniform vec2 camera;
uniform float globalScale;

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

    fragPos = pos;
    fragSize = size;
    fragColor = color;
    fragType = type;
}