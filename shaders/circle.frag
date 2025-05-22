#version 460 core

in vec2 vUV;

out vec4 FragColor;

// Globals
uniform int t;
uniform vec2 res;

uniform int shape;
uniform bool line;
uniform float thickness; // when line
uniform float rounding; // [0, 1]

uniform vec2 pos;
uniform vec2 size;
uniform vec4 color;

#define SHAPE_RECT 0
#define SHAPE_LINE 1
#define SHAPE_CIRCLE 2
#define SHAPE_HEXAGON 3
#define SHAPE_TRIANGLE 4

void main() {
    vec2 aspect = vec2(res.x / res.y, 1.0);

    vec2 p = (vUV - pos / res) * aspect;
    vec2 halfSizeN = (size / res) * aspect * 0.5;
    float thicknessN = thickness / min(res.x, res.y);

    float alpha = 1;
    float dist = 0;

    dist = length(p) - halfSizeN.x;
    if (dist > 0) discard;

    if (line && dist < -thicknessN) discard;
    FragColor = vec4(color.rgb, color.a * alpha);
}
