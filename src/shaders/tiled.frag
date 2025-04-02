#version 460 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 color;

uniform vec2 tile_count;
uniform vec2 tile_size;

uniform vec2 map_size;
uniform float coords[512];

void main() {
    vec2 cur_char;
    vec2 char_offset = {
            modf(map_size.x * texCoord.x, cur_char.x) * tile_size.x,
            modf(map_size.y * texCoord.y, cur_char.y) * tile_size.y
        };

    int cur_letter = int(coords[int(cur_char.y * map_size.x + cur_char.x)]);
    int map_width = int(tile_count.x / tile_size.x);
    vec2 to_draw = {
            cur_letter % map_width,
            cur_letter / map_width
        };

    vec4 pixel = texture(tex0, vec2(
                (to_draw.x * tile_size.x + char_offset.x) / tile_count.x,
                (to_draw.y * tile_size.y + char_offset.y) / tile_count.y
            ));

    if (color.a == 0) {
        // PNG rendering
        if (pixel == vec4(0, 0, 0, 1)) discard;
        FragColor = pixel;
    }
    else {
        // Font rendering
        FragColor = pixel == vec4(1, 1, 1, 1) ? vec4(0) : color;
    }
}
