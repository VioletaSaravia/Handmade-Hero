#version 460

in vec2 worldPos;
out vec4 fragColor;

uniform sampler2D tileAtlas; // The tile atlas
uniform usampler2D tilemap; // Integer texture with tile indices

uniform ivec2 mapSize; // Width/height of tilemap in tiles
uniform ivec2 atlasSize; // Width/height of atlas in tiles
uniform float tileSize; // Size of each tile in world units

void main() {
    // Compute which tile this fragment is on
    ivec2 tileCoord = ivec2(floor(worldPos / tileSize));

    // Discard fragments outside the map
    if (tileCoord.x < 0 || tileCoord.y < 0 || tileCoord.x >= mapSize.x || tileCoord.y >= mapSize.y)
        discard;

    // Get the tile index
    uint tileIndex = texelFetch(tilemap, tileCoord, 0).r;

    // Compute the UV offset in the atlas
    ivec2 atlasTileCoord = ivec2(tileIndex % atlasSize.x, tileIndex / atlasSize.x);

    // Local UV within the tile
    vec2 localUV = fract(worldPos / tileSize);

    // Final UV to sample from the atlas
    vec2 uv = (vec2(atlasTileCoord) + localUV) / vec2(atlasSize);

    fragColor = texture(tileAtlas, uv);
}
