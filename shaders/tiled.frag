#version 460 core

out vec4 FragColor;

in vec2 texCoord;
in vec4 fore_color;
in vec4 back_color;

uniform sampler2D tex0;

uniform bool two_color;

void main() {
    vec4 color = texture(tex0, texCoord);

    if (!two_color) FragColor = color;
    else FragColor = color == vec4(0) ? back_color : fore_color;
}
