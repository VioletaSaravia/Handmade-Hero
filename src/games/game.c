#include "../engine.c"

typedef struct {
    Texture tex;
    v2i     tileSize;
    v2i     atlasSize;
} Tileset;

Tileset NewTileset(cstr path, v2i tileSize) {
    Tileset result = (Tileset){
        .tex       = NewTexture(path),
        .tileSize  = tileSize,
        .atlasSize = (v2i){result.tex.size.w / tileSize.w, result.tex.size.h / tileSize.h}};

    return result;
}

typedef struct {
    Texture tex;
    u32    *data;
    v2i     size;
} Tilemap;

Tilemap NewTilemap(u32 *data, v2i size) {
    Tilemap result = {
        .data = data,
        .size = size,
    };
    glGenTextures(1, &result.tex.id);
    glBindTexture(GL_TEXTURE_2D, result.tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, size.w, size.h, 0, GL_RED_INTEGER, GL_UNSIGNED_INT,
                 result.data);

    return result;
}

void TilemapUpdate(Tilemap map) {
    glBindTexture(GL_TEXTURE_2D, map.tex.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, map.size.w, map.size.h, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    map.data);
}

void TilemapDraw(Tilemap map, Tileset set, v2 pos) {
    TextureUse(set.tex, 0);
    SetUniform1i("tileAtlas", 0);
    TextureUse(map.tex, 1);
    SetUniform1i("tilemap", 1);
    SetUniform2i("mapSize", map.size);
    SetUniform2i("atlasSize", set.atlasSize);
    SetUniform1f("tileSize", set.tileSize.h);

    ShaderUse(Graphics()->builtinShaders[SHADER_Tiles]);
    SetUniform2f("pos", pos);
    // SetUniform2f("size", (v2){100, 200});
    SetUniform1f("rotation", 0);
    SetUniform1f("radius", 0);
    SetUniform1f("border", 100);

    SetUniform4f("color", COLOR_NULL);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        DrawElement();
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

struct GameState {
    Arena   scene;
    Camera  cam;
    Text    text;
    Tileset set;
    Tilemap map;
};

enum Action {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_ACCEPT,
    ACTION_CANCEL,
    ACTION_COUNT,
};

global i32 Keymap[ACTION_COUNT] = {
    [ACTION_UP] = 283,
};

export void Setup() {
    *Settings() = (GameSettings){
        .name       = "Test",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
    };
}

export void Init() {
    S->scene = NewArena((u8 *)EngineGetMemory() + 5000, 5000); // FIXME
    S->text  = NewText("Hello. This is a sentence. Bye!", "data\\jetbrains.ttf", 12, 15);
    S->set   = NewTileset("data\\monogram.png", (v2i){6, 12});

    v2i mapSize = {30, 30};
    S->map      = NewTilemap((u32 *)Alloc(&S->scene, sizeof(u32) * mapSize.w * mapSize.h), mapSize);
}

export void Update() {}

export void Draw() {
    DrawText(S->text, (v2){100, 100});
}