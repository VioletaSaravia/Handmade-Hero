#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 color;

void main() {
    FragColor = texture(tex0, texCoord);
}
