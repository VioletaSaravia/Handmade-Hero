#version 460 core

out vec4 FragColor;

in vec2 vUV;

uniform sampler2D tex0;
uniform vec4 color;

void main() {
    FragColor = color != vec4(0) ? color : texture(tex0, vUV);
}
