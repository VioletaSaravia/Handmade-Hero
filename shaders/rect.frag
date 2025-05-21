#version 460 core

in vec2 vUV;

out vec4 FragColor;

uniform vec4 color;

void main() {
    FragColor = color;
    return;
}
