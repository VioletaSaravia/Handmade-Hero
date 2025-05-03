#version 460 core

in vec2 vUV;

out vec4 FragColor;

uniform int t;

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

void main() {
    // vec2 cUV = vUV - 0.5;
    // FragColor = vec4(cUV, 0, 1);
    // return;
    // Distance to nearest edge in normalized UV space
    vec2 distUV = min(vUV, 1.0 - vUV);
    // Convert to pixel space
    vec2 distPixels = distUV * abs(size);
    // Base alpha
    float alpha = 1.0;

    switch (shape) {
        case SHAPE_RECT:
        float roundingAbs = rounding * min(size.x, size.y);
        // In corner region, compute distance and fade
        if (distPixels.x < roundingAbs && distPixels.y < roundingAbs)
        {
            float distToCorner = length(distPixels - vec2(roundingAbs));
            alpha = smoothstep(roundingAbs, roundingAbs - 1.0, distToCorner);
        }

        float centerDist = distance(vUV - 0.5, vec2(0));
        float centerDistPixels = distance((vUV - 0.5) * abs(size), vec2(0));

        if (line && distPixels.x > thickness && distPixels.y > thickness) alpha = 0;

        FragColor = vec4(color.rgb, color.a * alpha);
        return;

        case SHAPE_LINE:
        float dist = pointLineDistance(vUV, vec2(0), vec2(1, 1));
        if (dist > (thickness / abs(max(size.x, size.y)))) alpha = 0;
        FragColor = vec4(color.rgb, alpha);

        return;

        case SHAPE_CIRCLE:
        centerDist = distance(vUV - 0.5, vec2(0));
        centerDistPixels = distance((vUV - 0.5) * abs(size), vec2(0));

        if (centerDist > 0.5) alpha = 0;
        if (line && centerDistPixels < size.x / 2 - thickness) alpha = 0;

        FragColor = vec4(color.rgb, color.a * alpha);
        return;

        case SHAPE_HEXAGON:
        vec2 p = rotate(vUV.xy - 0.5, radians(90));
        float r = 0.425;
        const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
        p = abs(p);
        p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
        p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);

        if (length(p) * sign(p.y) * 100 > 0) alpha = 0;
        if (line && length(p) * sign(p.y) < -(thickness / min(size.x, size.y))) alpha = 0;

        FragColor = vec4(color.rgb, color.a * alpha);
        return;

        default:
        FragColor = color;
        return;
    }
}
