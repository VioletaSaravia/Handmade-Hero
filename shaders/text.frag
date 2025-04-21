#version 460 core

out vec4 FragColor;

in vec2 texCoord;
in vec4 fore_color;
in vec4 back_color;
flat in int format;

uniform sampler2D fontRegular;
uniform sampler2D fontBold;
uniform sampler2D fontItalic;

void main() {
    vec4 color;
    switch (format) {
        case 1:
        color = texture(fontBold, texCoord);
        break;
        case 2:
        color = texture(fontItalic, texCoord);
        break;
        default:
        color = texture(fontRegular, texCoord);
        break;
    }

    FragColor = color == vec4(0) ? back_color : fore_color;
}
