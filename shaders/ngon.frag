#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec4 color;
uniform vec2 size;      // Quad size in pixels
uniform int sides;      // Number of polygon sides (≥3)
uniform float radius;   // Radius in pixels (from center to edge)
uniform float rotation; // Rotation angle in radians

void main() {
    // Convert UV to pixel coordinates centered at (0, 0)
    vec2 p = (vUV - 0.5) * size;

    // Apply rotation
    float c = cos(rotation);
    float s = sin(rotation);
    p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

    // Get polar coordinates
    float angle = atan(p.y, p.x);
    float dist = length(p);

    // Map angle to [0, 2π]
    float pi2 = 6.28318530718;
    angle = mod(angle + pi2, pi2);

    // Angle between polygon edges
    float segAngle = pi2 / float(sides);

    // Distance to nearest polygon edge direction
    float edgeFactor = cos(floor(0.5 + angle / segAngle) * segAngle - angle);
    float d = edgeFactor * dist;

    // Discard outside
    if (d > radius)
        discard;

    FragColor = color;
}