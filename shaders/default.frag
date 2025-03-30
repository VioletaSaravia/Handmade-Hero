#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform vec4 color;

void main() {
    FragColor = color;
}
