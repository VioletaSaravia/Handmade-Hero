#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex0;

uniform vec2 pos;
uniform vec2 size;
uniform vec2 res;
uniform float thickness;
uniform vec4 color;

void main() {
    vec2 p = gl_FragCoord.xy;

    // vector from start to end
    vec2 d = size;
    float len = length(d);
    vec2 dir = d / len;

    // vector from start to current pixel
    vec2 v = p - pos;

    // projection length of v onto dir
    float t = dot(v, dir);

    // closest point on line segment
    t = clamp(t, 0.0, len);
    vec2 closest = pos + dir * t;

    // distance to line
    float dist = length(p - closest);

    // draw if within thickness
    if (dist < thickness * 0.5) {
        FragColor = color;
    } else {
        discard;
    }
}
