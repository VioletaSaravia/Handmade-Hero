#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 color;

void main() {
    vec4 texColor = texture(tex0, texCoord);
    FragColor = (texColor == vec4(0, 0, 0, 1) ? vec4(0) : texColor) * color;
}
