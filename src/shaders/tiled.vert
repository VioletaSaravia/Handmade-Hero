#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 2) in vec2 instancePos;
layout(location = 3) in int instanceTile;
layout(location = 4) in vec4 foreColor;
layout(location = 5) in vec4 backColor;

out vec2 texCoord;
out vec4 fore_color;
out vec4 back_color;

uniform vec2 tile_size;
uniform ivec2 tileset_size;

uniform vec2 res;
uniform vec2 pos;
uniform float scale;
uniform bool two_color;

void main() {
    if (instanceTile == -1) {
        texCoord = vec2(0);
        return;
    }

    vec2 world_pos = pos + instancePos + aPos * tile_size;
    vec2 scale_px = vec2(scale / res.x, scale / res.y);
    vec2 ndc = world_pos * scale_px - vec2(1);
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);

    int tu = instanceTile % tileset_size.x;
    int tv = instanceTile / tileset_size.x;
    vec2 tileOffset = vec2(tu, tv) * tile_size;
    vec2 realTilesetSize = vec2(tile_size.x * tileset_size.x, tile_size.y * tileset_size.y);

    vec2 tilePos = tileOffset + aTexCoord * tile_size;
    texCoord = vec2(tilePos.x / realTilesetSize.x, tilePos.y / realTilesetSize.y);

    if (two_color) {
        fore_color = foreColor;
        back_color = backColor;
    }
}
