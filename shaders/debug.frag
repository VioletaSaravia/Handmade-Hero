#version 460 core

out vec4 FragColor;

in vec2 vUV;

in vec2 fragPos;
in vec2 fragSize;
in vec4 fragColor;
flat in int fragType;

void main() {
    switch (fragType) {
        case 1: // LINE
        vec2 p = gl_FragCoord.xy;

        // vector from start to end
        vec2 d = fragSize;
        float len = length(d);
        vec2 dir = d / len;

        // vector from start to current pixel
        vec2 v = p - fragPos;

        // projection length of v onto dir
        float t = dot(v, dir);

        // closest point on line segment
        t = clamp(t, 0.0, len);
        vec2 closest = fragPos + dir * t;

        // distance to line
        float dist = length(p - closest);

        // draw if within thickness
        if (dist < /*thickness * */ 0.5) {
            FragColor = fragColor;
        } else {
            discard;
        }
        break;

        case 2: // RECTANGLE
        float radius = 8; // PARAM
        // Distance to nearest edge in normalized UV space
        vec2 distUV = min(vUV, 1.0 - vUV);

        // Convert to pixel space
        vec2 distPixels = distUV * abs(fragSize);

        // Base alpha
        float alpha = 1.0;

        // In corner region, compute distance and fade
        if (distPixels.x < radius && distPixels.y < radius)
        {
            float distToCorner = length(distPixels - vec2(radius));
            alpha = smoothstep(radius, radius - 1.0, distToCorner);
        }

        FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
        break;

        case 3: // CIRCLE
        // Circle center in UV space is (0.5, 0.5)
        vec2 centerUV = vec2(0.5);

        // Convert UV distance to pixels
        float dist = length((vUV - centerUV) * abs(fragSize));

        // Parameters in pixels
        float radius = 32.0; // example
        float thickness = 4.0; // example
        float halfThickness = thickness * 0.5;

        // Base alpha using smoothstep for antialiased ring
        float alpha = smoothstep(radius + halfThickness, radius + halfThickness - 1.0, dist)
                * smoothstep(radius - halfThickness, radius - halfThickness + 1.0, dist);

        FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
        break;
    }
}
