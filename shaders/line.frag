#version 460 core

in vec2 vUV;

out vec4 FragColor;

uniform float thickness;
uniform float rounding;
uniform vec4 color;

float sdSegment(in vec2 p, in vec2 a, in vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 aspect = vec2(res.x / res.y, 1.0);
    vec2 p = (vUV - pos / res) * aspect;
    float thicknessN = thickness / min(res.x, res.y);

    if (sdSegment(p, pos / res, (pos + size) / res) > thicknessN) discard;

    FragColor = color;
}
