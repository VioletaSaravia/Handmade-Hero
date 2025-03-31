#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 color;

uniform vec2 font_count;
uniform vec2 font_size;

uniform vec2 text_size;
uniform float n_letters[512];

void main() {
    vec2 cur_char;
    vec2 char_offset = {
            modf(text_size.x * texCoord.x, cur_char.x) * font_size.x,
            modf(text_size.y * texCoord.y, cur_char.y) * font_size.y
        };

    int cur_letter = int(n_letters[int(cur_char.y * text_size.x + cur_char.x)]);
    int map_width = int(font_count.x / font_size.x);
    vec2 to_draw = {
            cur_letter % map_width,
            cur_letter / map_width
        };

    vec4 pixel = texture(tex0, vec2(
                (to_draw.x * font_size.x + char_offset.x) / font_count.x,
                (to_draw.y * font_size.y + char_offset.y) / font_count.y
            ));
    FragColor = pixel == vec4(1, 1, 1, 1) ? vec4(0) : color;
}
