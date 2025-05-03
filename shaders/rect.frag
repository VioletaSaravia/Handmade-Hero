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

vec2 rotate(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

float pointLineDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    vec2 closest = a + t * ab;
    return length(p - closest);
}

float sdSegment(in vec2 p, in vec2 a, in vec2 b)
{
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float sdBox(in vec2 p, in vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdCircle(vec2 p, float r)
{
    return length(p) - r;
}

float sdRoundedBox(in vec2 p, in vec2 b, in vec4 r)
{
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x = (p.y > 0.0) ? r.x : r.y;
    vec2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

float sdHexagon(in vec2 p, in float r)
{
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}

float sdEquilateralTriangle(in vec2 p, in float r)
{
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

void main() {
    vec2 aspect = vec2(res.x / res.y, 1.0);

    vec2 p = (vUV - pos / res) * aspect;
    vec2 halfSize = (size / res) * aspect * 0.5;
    float thicknessN = thickness / min(res.x, res.y);

    float alpha = 1;
    float dist = 0;

    switch (shape) {
        case SHAPE_RECT:
        float radius = rounding * min(halfSize.x, halfSize.y);
        dist = sdRoundedBox(p, halfSize, vec4(radius));
        if (dist > 0) discard;
        break;

        case SHAPE_CIRCLE:
        dist = sdCircle(p, halfSize.x);
        if (dist > 0) discard;
        break;

        case SHAPE_LINE:
        dist = sdSegment(p, pos / res, (pos + size) / res);
        if (dist > thicknessN) discard;
        break;

        case SHAPE_HEXAGON:
        p = rotate(p, radians(90));
        dist = sdHexagon(p, halfSize.x);
        if (dist > 0) discard;
        break;

        case SHAPE_TRIANGLE: // Not implemented
        dist = sdEquilateralTriangle(p, 0.1);
        if (dist > 0) discard;
        break;

        default:
        dist = 0;
        break;
    }

    if (line && dist < -thicknessN) discard;
    FragColor = vec4(color.rgb, color.a * alpha);
}
