#version 460 core

out vec4 FragColor;

in vec2 vUV;

uniform sampler2D tex0;
uniform vec4 color;
uniform float radius;
uniform float border;

uniform vec2 pos;
uniform vec2 size;

void main() {
    vec2 corner = vec2(
            vUV.x > 0.5 ? 1.0 - radius : radius,
            vUV.y > 0.5 ? 1.0 - radius : radius);

    bool isInCorner = (vUV.x < radius || vUV.x > 1.0 - radius) &&
            (vUV.y < radius || vUV.y > 1.0 - radius);
    float dist = distance(vUV, corner);
    if (isInCorner && (dist > radius))
        discard;

    vec2 borderWidth = border / size;
    bool inBorder = vUV.x < borderWidth.x || vUV.x > 1.0 - borderWidth.x ||
            vUV.y < borderWidth.y || vUV.y > 1.0 - borderWidth.y;

    if (!inBorder) discard;

    FragColor = color != vec4(0) ? color : texture(tex0, vUV);
}
