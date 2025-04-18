#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec4 color;
uniform vec2 size; // rectangle size in pixels

void main() {
    float uRadius = 0;
    // Distance to nearest edge in normalized UV space
    vec2 distUV = min(vUV, 1.0 - vUV);

    // Convert to pixel space
    vec2 distPixels = distUV * abs(size);

    // Base alpha
    float alpha = 1.0;

    // In corner region, compute distance and fade
    if (distPixels.x < uRadius && distPixels.y < uRadius)
    {
        float distToCorner = length(distPixels - vec2(uRadius));
        alpha = smoothstep(uRadius, uRadius - 1.0, distToCorner);
    }

    FragColor = vec4(color.rgb, color.a * alpha);
}
