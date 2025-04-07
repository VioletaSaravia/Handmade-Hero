#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 2) in vec2 instancePos;
layout(location = 3) in int instanceTile;

out vec2 texCoord;

uniform vec2 tileSize;
uniform int tilesetCols;
uniform vec2 screenSize;

void main() {
    vec2 worldPos = instancePos + aPos * tileSize;
    vec2 scale = vec2(2.0 / screenSize.x, 2.0 / screenSize.y);
    vec2 ndc = worldPos * scale - vec2(1);
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);

    int tu = instanceTile % tilesetCols;
    int tv = instanceTile / tilesetCols;

    vec2 tileOffset = vec2(tu, tv) * tileSize;
    vec2 tilesetSize = tileSize * float(tilesetCols);

    texCoord = (tileOffset + aTexCoord * tileSize) / tilesetSize;
}
