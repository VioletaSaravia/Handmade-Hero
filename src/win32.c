#include "win32.h"

void PrintShaderError(u32 shader) {
    i32 logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetShaderInfoLog(shader, logLength, 0, infoLog);

    printf("Shader compilation failed: %s\n", infoLog);

    free(infoLog);
}

void PrintProgramError(u32 program) {
    i32 logLength = 0;
    glGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetProgramInfoLog(program, logLength, 0, infoLog);

    printf("Program linking failed: %s\n", infoLog);

    free(infoLog);
}

Shader NewShader(cstr vertFile, cstr fragFile) {
    Shader result = {0};
    i32    ok     = 0;

    if (!vertFile) vertFile = "shaders\\default.vert";
    if (!fragFile) fragFile = "shaders\\default.frag";

#ifdef DEBUG
    result.vertPath = vertFile;
    result.fragPath = fragFile;

    result.vertWrite = GetLastWriteTime(result.vertPath);
    result.fragWrite = GetLastWriteTime(result.fragPath);
#endif

    string vertSrc = ReadEntireFile(vertFile);
    string fragSrc = ReadEntireFile(fragFile);

    // FIXME(violeta): Esto es sólo para evitar errores al hacer reload.
    // Cambiar por algo mejor.
    while (vertSrc.data == 0) vertSrc = ReadEntireFile(vertFile);
    while (fragSrc.data == 0) fragSrc = ReadEntireFile(fragFile);

    u32 vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc.data, 0);
    glCompileShader(vertShader);

    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        PrintShaderError(vertShader);
        return (Shader){0};
    }

    u32 fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc.data, 0);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        PrintShaderError(fragShader);
        return (Shader){0};
    }

    result.id = glCreateProgram();
    glAttachShader(result.id, vertShader);
    glAttachShader(result.id, fragShader);
    glLinkProgram(result.id);
    glGetProgramiv(result.id, GL_LINK_STATUS, &ok);
    if (!ok) {
        PrintProgramError(result.id);
        return (Shader){0};
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return result;
}

Framebuffer NewFramebuffer(cstr fragPath) {
    Framebuffer result = {0};

    persist f32 verts[] = {-1, 1, 0, 1, -1, -1, 0, 0, 1, -1, 1, 0,
                           -1, 1, 0, 1, 1,  -1, 1, 0, 1, 1,  1, 1};
    u32         quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(v4), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(v4), (void *)(sizeof(v2)));
    glEnableVertexAttribArray(1);
    result.vao = quadVAO;

    glGenFramebuffers(1, &result.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);

    glGenTextures(1, &result.tex);
    glBindTexture(GL_TEXTURE_2D, result.tex);
    v2 res = GetResolution();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (i32)res.w, (i32)res.h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.tex, 0);

    glGenRenderbuffers(1, &result.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, result.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (i32)res.w, (i32)res.h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              result.rbo);

    while (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) continue;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader shader = NewShader("shaders\\post.vert", fragPath);
    result.shader = shader;

    return result;
}

Mesh NewMesh(f32 *verts, u64 vertCount, u32 *indices, u64 idxCount) {
    Mesh result = {0};
    u32  ebo, vbo;

    glGenBuffers(1, &ebo);
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &result.vao);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * vertCount, verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * idxCount, indices, GL_STATIC_DRAW);

    i32 stride = 5 * sizeof(f32);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(v3)));
    glEnableVertexAttribArray(1);

    return result;
}

void DrawMesh(Mesh mesh) {
    glBindVertexArray(mesh.vao);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    glBindVertexArray(0);
}

Tileset NewTileset(cstr path, v2 tileSize) {
    return (Tileset){.tex = NewTexture(path), .tileSize = tileSize};
}

// TODO VAO instancing
typedef struct {
    u32  type, stride, count;
    u64  offset;
    bool div;
} Attrib;

typedef struct {
    u32    id;
    Attrib attribs[4];
    u32    dataType, drawType;
    i32    size;
    void  *data;
} BO;

typedef struct {
    u32 id;
    BO  buffers[16];
    i32 curAttrib;
} VAO;

VAO InitVao(VAO result) {
    glGenVertexArrays(1, &result.id);

    for (size_t i = 0; i < 16; i++) {
        if (result.buffers[i].data != 0) {
            glGenBuffers(1, &result.buffers[i].id);
        }
    }

    glBindVertexArray(result.id);

    for (size_t i = 0; i < 16; i++) {
        BO buf = result.buffers[i];
        glBindBuffer(buf.dataType, buf.id);
        glBufferData(buf.dataType, buf.size, buf.data, buf.drawType);

        u64 offset = 0;
        for (size_t j = 0; j < 4; j++) {
            Attrib attrib = buf.attribs[j];
            if (attrib.type == GL_FLOAT)
                glVertexAttribPointer(result.curAttrib, attrib.count, GL_FLOAT, GL_FALSE,
                                      attrib.stride, (void *)offset);
            else if (attrib.type == GL_INT)
                glVertexAttribIPointer(3, 1, GL_INT, 0, 0);

            glEnableVertexAttribArray(result.curAttrib);

            if (attrib.div) glVertexAttribDivisor(result.curAttrib, 1);

            result.curAttrib++;
            offset += attrib.offset;
        }
    }

    return result;
}

void UseVAO(const VAO *vao) {
    glBindVertexArray(vao->id);

    for (size_t i = 0; i < 16; i++) {
        BO buf = vao->buffers[i];
        if (buf.drawType == GL_DYNAMIC_DRAW) {
            glBindBuffer(buf.dataType, buf.id);
            glBufferSubData(buf.dataType, 0, buf.size, buf.data);
        }
    }
}

// TODO Shader attrib parser
typedef struct {
    u32  location;
    char type[32];
    char name[64];
} ShaderAttrib;

void parse_shader_attributes(const char *filename, ShaderAttrib *attribs, int *attribCount) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open shader file");
        exit(EXIT_FAILURE);
    }

    char line[512];
    *attribCount = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "layout(location")) {
            ShaderAttrib a;

            // Parse with sscanf
            if (sscanf(line, "layout(location = %u) in %31s %63[^;];", &a.location, a.type,
                       a.name) == 3) {
                attribs[(*attribCount)++] = a;
            }
        }
    }

    fclose(file);
}

u32 glsl_type_to_count(const char *type) {
    if (strcmp(type, "float") == 0) return 1;
    if (strcmp(type, "vec2") == 0) return 2;
    if (strcmp(type, "vec3") == 0) return 3;
    if (strcmp(type, "vec4") == 0) return 4;
    if (strcmp(type, "int") == 0) return 1;
    // extend as needed
    return 0;
}

void create_vao_from_shader(const char *shaderFile, VAO *vao) {
    ShaderAttrib attribs[16];
    int          attribCount = 0;
    parse_shader_attributes(shaderFile, attribs, &attribCount);

    vao->id        = 1;
    vao->curAttrib = 0;

    for (int i = 0; i < attribCount; i++) {
        Attrib attr;
        attr.type   = 0; // you can map GLSL types to OpenGL types if needed
        attr.count  = glsl_type_to_count(attribs[i].type);
        attr.stride = 0; // fill this if you have buffer layouts
        attr.offset = 0; // likewise
        attr.div    = false;

        vao->buffers[0].attribs[attribs[i].location] = attr;
        vao->curAttrib++;
    }
}

Tilemap NewTilemap(Tileset tileset) {
    Tilemap result = {0};

    persist f32 quadVertices[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1};
    persist i32 quadIndices[]  = {0, 1, 2, 2, 3, 0};

    result.tileset = tileset;

    result.width = TILES_X * tileset.tileSize.x;

    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);
    glGenBuffers(1, &result.idVbo);
    glGenBuffers(1, &result.posVbo);
    glGenBuffers(1, &result.colForeVbo);
    glGenBuffers(1, &result.colBackVbo);
    glBindVertexArray(result.vao);

    // VERTEX VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)sizeof(v2));
    glEnableVertexAttribArray(1);

    // INDEX VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    for (int iy = 0; iy < TILES_Y; iy++)
        for (int ix = 0; ix < TILES_X; ix++) {
            int i             = iy * TILES_X + ix;
            result.instPos[i] = (v2){tileset.tileSize.x * (f32)ix, tileset.tileSize.y * (f32)iy};
        }

    // INSTANCE POS VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.posVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * TILES_X * TILES_Y, result.instPos, GL_STATIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // INSTANCE ID VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.idVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(i32) * TILES_X * TILES_Y, result.instIdx, GL_DYNAMIC_DRAW);

    glVertexAttribIPointer(3, 1, GL_INT, 0, 0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // INSTANCE FORE COLOR VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.colForeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v4) * TILES_X * TILES_Y, result.instForeColor,
                 GL_DYNAMIC_DRAW);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // INSTANCE BACK COLOR VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.colBackVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v4) * TILES_X * TILES_Y, result.instBackColor,
                 GL_DYNAMIC_DRAW);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    for (int i = 0; i < TILES_X * TILES_Y; i++) {
        result.instForeColor[i] = (v4){1, 1, 1, 1};
        result.instBackColor[i] = (v4){0};
    }

    return result;
}

void UseTexture(Texture tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
}

void UseShader(Shader shader) {
    glUseProgram(shader.id);
    Graphics()->activeShader = shader.id;
    SetUniform2f("camera", Graphics()->cam.pos);
}

void DrawTilemap(const Tilemap *tilemap, v2 pos, f32 scale, bool twoColor) {
    UseShader(Graphics()->shaders[SHADER_Tiled]);
    SetUniform2f("tile_size", tilemap->tileset.tileSize);
    v2i tilesetSize = (v2i){
        tilemap->tileset.tex.size.x / tilemap->tileset.tileSize.x,
        tilemap->tileset.tex.size.y / tilemap->tileset.tileSize.y,
    };

    SetUniform2i("tileset_size", tilesetSize);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", pos);
    SetUniform1f("scale", scale * 2);
    SetUniform1f("scaling", Settings()->scale);
    SetUniform1b("two_color", twoColor);
    SetUniform1i("tex0", 0);

    UseTexture(tilemap->tileset.tex);

    glBindVertexArray(tilemap->vao);

    // on-the-fly tilemap updating
    glBindBuffer(GL_ARRAY_BUFFER, tilemap->idVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(i32) * TILES_X * TILES_Y, tilemap->instIdx);

    if (twoColor) {
        glBindBuffer(GL_ARRAY_BUFFER, tilemap->colForeVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v4) * TILES_X * TILES_Y, tilemap->instForeColor);
        glBindBuffer(GL_ARRAY_BUFFER, tilemap->colBackVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v4) * TILES_X * TILES_Y, tilemap->instBackColor);
    }

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, TILES_X * TILES_Y);
}

void TilemapLoadCsv(Tilemap *tilemap, cstr csvPath) {
    u8 *data = 0;
    // TODO Load Tiled CSVs
    for (int i = 0; i < TILES_X * TILES_Y; i++) {
        i32 num             = 0;
        tilemap->instIdx[i] = num;
    }
}

Image LoadBMP32x32Image(cstr path) {
    string data   = ReadEntireFile(path);
    Image  result = {0};

    AseBMP32x32 *bmpData = (AseBMP32x32 *)data.data;
    result.data          = malloc(1024 * 4);
    for (u64 i = 0; i < 256; i++) {
        i32 iData          = i * 4;
        result.data[iData] = bmpData->rgbq[i].r;
        result.data[iData] = bmpData->rgbq[i].g;
        result.data[iData] = bmpData->rgbq[i].b;
        result.data[iData] = bmpData->rgbq[i].a;
    }
    return result;
}

void WriteText(cstr text, Tilemap *tilemap, v2 pos, f32 scale) {
    i32  iText = 0, iBox = 0, nextSpace = 0;
    bool addWhitespace = false;

    while (iText < strlen(text) && iBox < TILES_X * TILES_Y) {
        if (!addWhitespace)
            for (int i = 0; i < strlen(text); i++) {
                if (text[i] == ' ' || (i == strlen(text) - 1)) {
                    nextSpace = i - iText;
                    break;
                }
            }

        addWhitespace          = nextSpace >= TILES_X - (iBox % TILES_X);
        tilemap->instIdx[iBox] = !addWhitespace ? 0 : 0;

        iText += !addWhitespace ? 1 : 0;
        iBox += 1;

        addWhitespace = addWhitespace && ((iBox & TILES_X) != 0);
    }

    if (strlen(text) < TILES_X * TILES_Y)
        for (int i = iBox; i < TILES_X * TILES_Y; i++) {
            tilemap->instIdx[i] = 0;
        }

    DrawTilemap(tilemap, pos, scale, true);
}

Texture NewTexture(cstr path) {
    Texture result = {0};

    string file = ReadEntireFile(path);

    u8 *img = stbi_load_from_memory(file.data, file.len, &result.size.x, &result.size.y,
                                    &result.nChan, 0);

    if (!img) {
        // LOG
        return (Texture){0};
    }

    glGenTextures(1, &result.id);
    glBindTexture(GL_TEXTURE_2D, result.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    u32 format = GL_RGBA; // RGB for BMP

    glTexImage2D(GL_TEXTURE_2D, 0, format, result.size.x, result.size.y, 0, format,
                 GL_UNSIGNED_BYTE, img);
    // GL_GenerateMipmap(GL_TEXTURE_2D)

    stbi_image_free(img);

    return result;
}

void ReloadShader(Shader *shader) {
#ifdef DEBUG
    u64 vertTime = GetLastWriteTime(shader->vertPath);
    u64 fragTime = GetLastWriteTime(shader->fragPath);

    if (vertTime <= shader->vertWrite && fragTime <= shader->fragWrite) return;

    *shader = NewShader(shader->vertPath, shader->fragPath);
    printf("[Info] [%s] %s and %s succeded", __func__, shader->vertPath, shader->fragPath);
#endif
    return;
}

void SetUniform1i(cstr name, i32 value) {
    glUniform1i(glGetUniformLocation(Graphics()->activeShader, name), value);
}

void SetUniform2i(cstr name, v2i value) {
    glUniform2i(glGetUniformLocation(Graphics()->activeShader, name), value.x, value.y);
}

void SetUniform1f(cstr name, f32 value) {
    glUniform1f(glGetUniformLocation(Graphics()->activeShader, name), value);
}

void SetUniform1fv(cstr name, f32 *value, u64 count) {
    glUniform1fv(glGetUniformLocation(Graphics()->activeShader, name), count, value);
}

void SetUniform2f(cstr name, v2 value) {
    glUniform2f(glGetUniformLocation(Graphics()->activeShader, name), value.x, value.y);
}

void SetUniform3f(cstr name, v3 value) {
    glUniform3f(glGetUniformLocation(Graphics()->activeShader, name), value.x, value.y, value.z);
}

void SetUniform4f(cstr name, v4 value) {
    glUniform4f(glGetUniformLocation(Graphics()->activeShader, name), value.x, value.y, value.z,
                value.w);
}

void SetUniform1b(cstr name, bool value) {
    SetUniform1i(name, (value ? 1 : 0));
}

void DrawTexture(Texture tex, v2 pos, f32 scale) {
    UseShader(Graphics()->shaders[SHADER_Default]);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", pos);
    SetUniform1f("scale", scale);
    SetUniform1f("scaling", Settings()->scale);
    SetUniform2f("size", (v2){tex.size.x, tex.size.y});
    SetUniform4f("color", WHITE);
    UseTexture(tex);
    DrawMesh(Graphics()->squareMesh);
}

global f32 squareVerts[] = {1, 1, 0, 1, 0, 1, -1, 0, 1, 1, -1, -1, 1, 0, 1, -1, 1, 0, 0, 0};
global u32 squareIds[]   = {0, 1, 3, 1, 2, 3};

GraphicsCtx InitGraphics(const WindowCtx *ctx, const GameSettings *settings) {
    GraphicsCtx result = {0};
    InitOpenGL(ctx->window, settings);

    result.shaders[SHADER_Default] = NewShader(0, 0);
    result.shaders[SHADER_Tiled]   = NewShader("shaders\\tiled.vert", "shaders\\tiled.frag");
    result.shaders[SHADER_Rect]    = NewShader("shaders\\rect.vert", "shaders\\rect.frag");
    result.squareMesh              = NewMesh(squareVerts, 20, squareIds, 6);
    result.mouse                   = NewTexture("data\\pointer.png");
    result.postprocessing          = NewFramebuffer("shaders\\post.frag");

    return result;
}

void ClearScreen(v4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void UseFramebuffer(Framebuffer shader) {
    glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);
}

void DrawFramebuffer(Framebuffer shader) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ClearScreen((v4){0});

    UseShader(shader.shader);
    SetUniform2f("res", GetResolution());

    glBindVertexArray(shader.vao);
    glBindTexture(GL_TEXTURE_2D, shader.tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

EngineCtx *E;

InputCtx *Input() {
    return &E->Input;
}
WindowCtx *Window() {
    return &E->Window;
}
GraphicsCtx *Graphics() {
    return &E->Graphics;
}
AudioCtx *Audio() {
    return &E->Audio;
}
GameSettings *Settings() {
    return &E->Settings;
}
f32 Delta() {
    return E->Timing.delta;
}

extern void GameLoad(void (*setup)(), void (*init)(), void (*update)(), void (*draw)()) {
    E->Game.Setup  = setup;
    E->Game.Init   = init;
    E->Game.Update = update;
    E->Game.Draw   = draw;
}

extern void GameEngineInit() {
    srand((u32)time(0));
    E->Game.Setup();

    E->Window   = InitWindow();
    E->Graphics = InitGraphics(&E->Window, &E->Settings);
    InitAudio(&E->Audio);
    E->Timing = InitTiming(E->Window.refreshRate);

    E->Game.Init();
}

void CameraBegin(Camera cam) {
    Graphics()->cam = cam;
}

void CameraEnd() {
    Graphics()->cam = (Camera){0};
}

void TimeAndRender(TimingCtx *timing, const WindowCtx *window, const GraphicsCtx *graphics,
                   bool disableMouse) {
    u64 endCycleCount    = __rdtsc();
    u64 cyclesElapsed    = endCycleCount - timing->lastCycleCount;
    f64 mgCyclesPerFrame = (f64)(cyclesElapsed) / (1000.0 * 1000.0);

    timing->delta = GetSecondsElapsed(timing->perfCountFreq, timing->lastCounter, GetWallClock());
    while (timing->delta < timing->targetSpf) {
        if (timing->granularSleepOn) Sleep((u32)(1000.0 * (timing->targetSpf - timing->delta)));
        timing->delta =
            GetSecondsElapsed(timing->perfCountFreq, timing->lastCounter, GetWallClock());
    }

    f32 msPerFrame = timing->delta * 1000.0f;
    f32 msBehind   = (timing->delta - timing->targetSpf) * 1000.0f;
    f64 fps        = (f64)(timing->perfCountFreq) / (f64)(GetWallClock() - timing->lastCounter);

    UseFramebuffer(graphics->postprocessing);
    {
        ClearScreen((v4){0.3, 0.2, 0.4, 1});
        E->Game.Draw();
        CameraEnd();
        if (!disableMouse) DrawTexture(graphics->mouse, Mouse(), 1.0f);
    }
    DrawFramebuffer(graphics->postprocessing);

    SwapBuffers(window->dc);
    ReleaseDC(window->window, window->dc);

    timing->lastCounter    = GetWallClock();
    timing->lastCycleCount = endCycleCount;
}

extern void GameEngineUpdate() {
    ProcessKeyboard(E->Input.keys, &E->Window.running);
    ProcessGamepads(E->Input.gamepads);
    ProcessMouse(&E->Input.mouse);

    if (E->Input.keys[KEY_F12] == JustPressed) E->Game.Init();
    if (E->Input.keys[KEY_F11] == JustPressed) ToggleFullscreen();
    if (E->Input.keys[KEY_Esc] == JustPressed) E->Window.running = 0;

    for (i32 i = 0; i < KEY_COUNT; i++) {
        ButtonState k = E->Input.keys[i];
        if (k == JustReleased) printf("Key %d was released.\n", i);
        if (k == JustPressed) printf("Key %d was pressed.\n", i);
    }

    ReloadShader(&E->Graphics.postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ReloadShader(&E->Graphics.shaders[i]);

    E->Game.Update();
    TimeAndRender(&E->Timing, &E->Window, &E->Graphics, E->Settings.disableMouse);
}

void GameEngineShutdown() {
    ShutdownAudio(&E->Audio);
}

extern void *GameGetMemory() {
    return E;
}

extern bool GameIsRunning() {
    return E->Window.running;
}

u8 *RingAlloc(MemRegion *buf, u32 size) {
    if (buf->count + size > buf->size) buf->count = 0;

    u8 *result = &buf->data[buf->count];
    buf->count += size;
    return result;
}

MemRegion NewMemRegion(u32 size) {
    return (MemRegion){
        .count = 0,
        .size  = size,
        .data  = ALLOC(size),
    };
}

u8 *BufferAlloc(MemRegion *buffer, u32 count) {
    u8 *result = &buffer->data[buffer->count];
    buffer->count += count;
    return result;
}

string ReadEntireFile(const char *filename) {
    string result = {0};
    HANDLE file   = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        // LOG
        return (string){0};
    }

    u32 size = GetFileSize(file, 0);
    if (size == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        CloseHandle(file);
        // LOG
        return (string){0};
    }

    u64 allocSize = size + 1;

    result.data = (char *)VirtualAlloc(0, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!result.data) {
        // LOG
        CloseHandle(file);
        return (string){0};
    }

    bool success = ReadFile(file, result.data, size, &result.len, 0);
    CloseHandle(file);

    if (!success || result.len != size) {
        // LOG
        VirtualFree(result.data, 0, MEM_RELEASE);
        return (string){0};
    }
    result.data[size] = '\0';
    return result;
}

i64 GetWallClock() {
    LARGE_INTEGER result = {0};
    QueryPerformanceCounter(&result);
    return (i64)result.QuadPart;
}

i64 InitPerformanceCounter(i64 *freq) {
    LARGE_INTEGER result = {0};
    QueryPerformanceFrequency(&result);
    *freq = (i64)(result.QuadPart);
    return GetWallClock();
}

u64 GetLastWriteTime(cstr file) {
    u64 result = 0;

    WIN32_FILE_ATTRIBUTE_DATA fileInfo;

    if (!GetFileAttributesEx(file, GetFileExInfoStandard, &fileInfo)) {
        printf("Failed to get file attributes. Error code: %lu\n", GetLastError());
        return 0;
    }

    FILETIME writeTime = fileInfo.ftLastWriteTime;

    result = ((u64)(writeTime.dwHighDateTime) << 32) | (u64)(writeTime.dwLowDateTime);
    return result;
}

v2 GetResolution() {
    RECT clientRect = {0};
    // KB global
    GetClientRect(Window()->window, &clientRect);

    return (v2){clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
}

LRESULT WINAPI MainWindowCallback(HWND window, u32 msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR:
        HCURSOR cursor = LOWORD(lParam) == HTCLIENT ? 0 : LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        SetCursor(cursor);
        break;

    case WM_SIZE: break;

    case WM_DESTROY: Window()->running = false; break;

    case WM_MOUSEHWHEEL:
        // TODO mousewheel
        break;

    case WM_PAINT:
        PAINTSTRUCT paint;
        HDC         deviceCtx = BeginPaint(window, &paint);
        EndPaint(window, &paint);
        break;

    case WM_ACTIVATEAPP: break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        bool wasDown = lParam & (1 << 30);
        bool isDown  = !(lParam & (1 << 31));

        Input()->keys[wParam] = wasDown && isDown    ? Pressed
                                : !wasDown && isDown ? JustPressed
                                : wasDown && !isDown ? JustReleased
                                                     : Released;

    default: return DefWindowProcW(window, msg, wParam, lParam);
    }

    return 0;
}

void ResizeDIBSection(WindowCtx *window, v2i size) {
    if (window->memory != 0) VirtualFree(window->memory, 0, MEM_RELEASE);

    window->w          = size.w;
    window->h          = size.h;
    window->bytesPerPx = 4;
    window->info       = (BITMAPINFO){
              .bmiHeader =
            {
                      .biSize  = sizeof(window->info.bmiHeader),
                      .biWidth = window->w,
                // Negative height tells Windows to treat the window's y axis as top-down
                      .biHeight        = -window->h,
                      .biPlanes        = 1,
                      .biBitCount      = 32, // 4 byte align
                      .biCompression   = BI_RGB,
                      .biSizeImage     = 0,
                      .biXPelsPerMeter = 0,
                      .biYPelsPerMeter = 0,
                      .biClrUsed       = 0,
                      .biClrImportant  = 0,
            },
    };

    window->memoryLen = window->w * window->h * window->bytesPerPx;
    window->memory =
        (u8 *)(VirtualAlloc(0, window->memoryLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

u32 GetRefreshRate(HWND hWnd) {
    HMONITOR       monitor     = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW monitorInfo = (MONITORINFOEXW){
        .cbSize = sizeof(MONITORINFOEXW),
    };

    GetMonitorInfoW(monitor, (LPMONITORINFO)&monitorInfo);
    DEVMODEW dm = (DEVMODEW){.dmSize = sizeof(DEVMODEW)};

    EnumDisplaySettingsW(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    return dm.dmDisplayFrequency;
}

WindowCtx InitWindow() {
    WindowCtx buffer;

    RECT rect = {0, 0, Settings()->resolution.w * Settings()->scale,
                 Settings()->resolution.h * Settings()->scale};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

    v2i size = (v2i){rect.right - rect.left, rect.bottom - rect.top};
    ResizeDIBSection(&buffer, size);
    HANDLE instance = GetModuleHandleW(0);

    WNDCLASSW window_class = {
        .style       = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = MainWindowCallback,
        .hInstance   = instance,
        .hIcon       = LoadImageW(instance, L"data\\icon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE),
        .lpszClassName = (LPCWSTR)Settings()->name,
    };

    if (RegisterClassW(&window_class) == 0) {
        // LOG
        return (WindowCtx){0};
    }

    buffer.window =
        CreateWindowExW(0, window_class.lpszClassName, (LPCWSTR)Settings()->name,
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, size.w, size.h, 0, 0, instance, 0);

    if (buffer.window == 0) {
        OutputDebugStringA("ERROR");
        return (WindowCtx){0};
    }

    buffer.dc          = GetDC(buffer.window);
    buffer.refreshRate = GetRefreshRate(buffer.window);
    buffer.running     = 1;

    return buffer;
}

internal void *Win32GetProcAddress(const char *name) {
    void *p = (void *)wglGetProcAddress(name);
    if (!p || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p              = (void *)GetProcAddress(module, name);
    }
    return p;
}

void InitOpenGL(HWND hWnd, const GameSettings *settings) {
    PIXELFORMATDESCRIPTOR desired = (PIXELFORMATDESCRIPTOR){
        .nSize      = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion   = 1,
        .dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cAlphaBits = 8,
    };

    HDC                   windowDC       = GetDC(hWnd);
    i32                   suggestedIndex = ChoosePixelFormat(windowDC, &desired);
    PIXELFORMATDESCRIPTOR suggested;
    SetPixelFormat(windowDC, suggestedIndex, &suggested);

    HGLRC openglrc = wglCreateContext(windowDC);
    if (wglMakeCurrent(windowDC, openglrc) == 0) return;

    if (!gladLoadGLLoader((GLADloadproc)Win32GetProcAddress)) {
        // LOG
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, settings->resolution.w * settings->scale,
               settings->resolution.h * settings->scale);

    ReleaseDC(hWnd, windowDC);
}

void AudioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    SoundBuffer *sounds = (SoundBuffer *)pDevice->pUserData;
    f32         *out    = (f32 *)pOutput;
    memset(out, 0, frameCount * sizeof(f32) * 2);

    f32 temp[4096] = {0};

    for (u32 i = 0; i < MAX_SOUNDS; i++) {
        SoundBuffer *s = &sounds[i];
        if (!s->playing) continue;

        u64 read = 0;
        ma_decoder_read_pcm_frames(&s->decoder, temp, frameCount, &read);

        if (read == 0) {
            s->playing = (s->type == LOOPING);
            ma_decoder_seek_to_pcm_frame(&s->decoder, 0);
            continue;
        }

        f32 leftGain  = 1.0f - (s->pan > 0 ? s->pan : 0.0f);
        f32 rightGain = 1.0f + (s->pan < 0 ? s->pan : 0.0f);
        for (size_t j = 0; j < read * 2 /* TODO CHANNELS */; j++) {
            out[j] /* CHANNELS */ += temp[j] * s->vol * (j % 2 == 0 ? leftGain : rightGain);
        }
    }
}

void InitAudio(AudioCtx *ctx) {
    ctx->config = ma_device_config_init(ma_device_type_playback);

    ctx->config.playback.format   = ma_format_f32; // Set to ma_format_unknown to use the device's
    ctx->config.playback.channels = 2;     // Set to 0 to use the device's native channel count.
    ctx->config.sampleRate        = 48000; // Set to 0 to use the device's native sample rate.
    ctx->config.dataCallback      = AudioCallback; // called when miniaudio needs more data.
    ctx->config.pUserData         = ctx->sounds;   // Can be accessed from (device.pUserData).

    if (ma_device_init(0, &ctx->config, &ctx->device) != MA_SUCCESS) {
        printf("[Fatal] [%s] Couldn't initialize audio device\n", __func__);
        return;
    }

    ma_result err = ma_device_start(&ctx->device);
    if (err != MA_SUCCESS) {
        printf("[Fatal] [%s] Couldn't start audio device: %d\n", __func__, err);
        ma_device_uninit(&ctx->device);
        return;
    }

    return;
}

void ShutdownAudio(AudioCtx *audio) {
    ma_device_stop(&audio->device);
    ma_device_uninit(&audio->device);
    for (size_t i = 0; i < MAX_SOUNDS; i++) {
        ma_decoder_uninit(&Audio()->sounds[i].decoder);
    }
}

Sound LoadSound(cstr path, PlaybackType type) {
    string      data   = ReadEntireFile(path);
    SoundBuffer result = {
        .data    = (u16 *)data.data,
        .dataLen = data.len,
        .vol     = 1.0,
        .pan     = 0.0,
        .type    = type,
    };

    ma_decoder_config config =
        ma_decoder_config_init(Audio()->device.playback.format, Audio()->device.playback.channels,
                               Audio()->device.sampleRate);

    i32 err = ma_decoder_init_memory(result.data, result.dataLen, &config, &result.decoder);
    if (err != MA_SUCCESS) {
        printf("[Error] [%s] Can't initialize memory: %d", __func__, err);
        return (Sound){-1};
    }

    u32 id              = Audio()->count;
    Audio()->sounds[id] = result;
    Audio()->count      = (id + 1) % MAX_SOUNDS;
    return (Sound){id};
}

void PlaySound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = true;
    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
}
void PauseSound(Sound sound) {
    Audio()->sounds[sound.id].playing = false;
}
void StopSound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = false;
    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
}
void ResumeSound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = true;
}

TimingCtx InitTiming(f32 refreshRate) {
    i64       freq   = 0;
    TimingCtx result = {
        .delta              = 1.0f / refreshRate,
        .targetSpf          = result.delta,
        .lastCounter        = InitPerformanceCounter(&freq),
        .lastCycleCount     = __rdtsc(),
        .perfCountFreq      = freq,
        .desiredSchedulerMs = 1,
        .granularSleepOn    = timeBeginPeriod(1),
    };
    return result;
}

extern void GameReloadMemory(void *memory) {
    // TODO DLL Reloading
    // E = memory;
    // FIXME Así se reinicia el dll? Hace falta?
    if (!gladLoadGLLoader((GLADloadproc)Win32GetProcAddress)) {
        // LOG
        return;
    }
}

f32 GetSecondsElapsed(i64 perfCountFreq, i64 start, i64 end) {
    return (f32)(end - start) / (f32)(perfCountFreq);
}

void ResizeWindow(HWND hWnd, v2i size, bool fullscreen) {
    size.h    = size.h == 0 ? 1 : size.h;
    RECT rect = (RECT){0, 0, size.w, size.h};

    if (!fullscreen) {
        AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);
    }

    SetWindowPos(hWnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);

    glViewport(0, 0, size.w, size.h);
}

void ToggleFullscreen() {
    v2i  size   = (v2i){GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    HWND window = Window()->window;

    i64 style = GetWindowLongW(window, GWL_STYLE);
    SetWindowLongW(window, GWL_STYLE, style & ~(i32)WS_OVERLAPPEDWINDOW);

    SetMenu(window, 0);
    SetWindowPos(window, HWND_TOPMOST, 0, 0, size.w, size.h, SWP_NOZORDER | SWP_FRAMECHANGED);
    ResizeWindow(window, size, 1);
}

internal v2 Win32GetMouse() {
    POINT pt = {0};
    if (GetCursorPos(&pt) == false) return (v2){0};

    RECT rect = {0};
    // KB global
    if (GetWindowRect(Window()->window, &rect) == false) return (v2){0};

    return (v2){pt.x - rect.left - 10, pt.y - rect.top - 34};
}

v2 Mouse() {
    return Input()->mouse.pos;
}

v2 MouseInWorld(Camera cam) {
    v2 mouse = Mouse();
    return (v2){mouse.x + cam.pos.x, mouse.y + cam.pos.y};
}

void LockCursorToWindow(HWND hWnd) {
    RECT rect = {0};
    GetClientRect(hWnd, &rect);
    POINT screenCoord = (POINT){0};
    ClientToScreen(hWnd, &screenCoord);
    rect.left = screenCoord.x;
    rect.top  = screenCoord.y;
    ClipCursor(&rect);
}

void ProcessKeyboard(ButtonState *keys, bool *running) {
    for (i32 i = 0; i < KEY_COUNT; i++) {
        if (keys[i] == JustReleased) keys[i] = Released;
        if (keys[i] == JustPressed) keys[i] = Pressed;
    }

    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, 1)) {
        if (msg.message == WM_QUIT) *running = 0;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void ProcessGamepads(GamepadState *gamepads) {
    for (i32 i = 0; i < GAMEPAD_MAX; i++) {
        GamepadState new       = {0};
        const GamepadState old = gamepads[i];

        XINPUT_STATE state = {0};
        if (XInputGetState(i, &state) != ERROR_SUCCESS) {
            if (old.connected) new.connected = 0;
            continue;
        }

        if (!old.connected) new.connected = 1;

        XINPUT_GAMEPAD *gamepad = &state.Gamepad;

        for (i32 j = 0; j < PAD_ButtonCount; j++) {
            switch (old.buttons[j]) {
            case Pressed:
            case JustPressed:
                // FIXME ???
                new.buttons[j] =
                    XINPUT_GAMEPAD_DPAD_UP | gamepad->wButtons ? Pressed : JustReleased;
                break;

            case Released:
            case JustReleased:
                new.buttons[j] =
                    XINPUT_GAMEPAD_DPAD_UP | gamepad->wButtons ? Pressed : JustReleased;
                break;
            }
        }

        new.lStart = old.lEnd;
        new.rStart = old.rEnd;

        f32 stickLX = (f32)(gamepad->sThumbLX) / (gamepad->sThumbLX < 0 ? -32768.0f : 32767.0);
        f32 stickLY = (f32)(gamepad->sThumbLY) / (gamepad->sThumbLY < 0 ? -32768.0f : 32767.0);
        f32 stickRX = (f32)(gamepad->sThumbRX) / (gamepad->sThumbRX < 0 ? -32768.0f : 32767.0);
        f32 stickRY = (f32)(gamepad->sThumbRY) / (gamepad->sThumbRY < 0 ? -32768.0f : 32767.0);

        new.lEnd = (v2){stickLX, stickLY};
        new.rEnd = (v2){stickRX, stickRY};

        new.trigLStart = old.trigLEnd;
        new.trigRStart = old.trigREnd;
        new.trigLEnd   = (f32)(gamepad->bLeftTrigger) / 255.0f;
        new.trigREnd   = (f32)(gamepad->bRightTrigger) / 255.0f;

        gamepads[i] = new;
    }
}

void ProcessMouse(MouseState *mouse) {
    // NOTE: La rueda del mouse se maneja en MainWindowCallback
    mouse->pos = Win32GetMouse();

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        mouse->left = mouse->left >= Pressed ? Pressed : JustPressed;
    else
        mouse->left = mouse->left <= Released ? Released : JustReleased;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        mouse->right = mouse->right >= Pressed ? Pressed : JustPressed;
    else
        mouse->right = mouse->right <= Released ? Released : JustReleased;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        mouse->middle = mouse->middle >= Pressed ? Pressed : JustPressed;
    else
        mouse->middle = mouse->middle <= Released ? Released : JustReleased;
}

const f32 CharmapCoords[256] = {
    [' '] = 0,   ['A'] = 1,  ['B'] = 2,  ['C'] = 3,  ['D'] = 4,  ['E'] = 5,  ['F'] = 6,
    ['G'] = 7,   ['H'] = 8,  ['I'] = 9,  ['J'] = 10, ['K'] = 11, ['L'] = 12, ['M'] = 13,
    ['N'] = 14,  ['O'] = 15, ['P'] = 16, ['Q'] = 17, ['R'] = 18, ['S'] = 19, ['T'] = 20,
    ['U'] = 21,  ['V'] = 22, ['W'] = 23, ['X'] = 24, ['Y'] = 25, ['Z'] = 26, ['?'] = 27,
    ['!'] = 28,  ['.'] = 29, [','] = 30, ['-'] = 31, ['+'] = 32, ['a'] = 33, ['b'] = 34,
    ['c'] = 35,  ['d'] = 36, ['e'] = 37, ['f'] = 38, ['g'] = 39, ['h'] = 40, ['i'] = 41,
    ['j'] = 42,  ['k'] = 43, ['l'] = 44, ['m'] = 45, ['n'] = 46, ['o'] = 47, ['p'] = 48,
    ['q'] = 49,  ['r'] = 50, ['s'] = 51, ['t'] = 52, ['u'] = 53, ['v'] = 54, ['w'] = 55,
    ['x'] = 56,  ['y'] = 57, ['z'] = 58, [':'] = 59, [';'] = 60, ['"'] = 61, ['('] = 62,
    [')'] = 63,  ['@'] = 64, ['&'] = 65, ['1'] = 66, ['2'] = 67, ['3'] = 68, ['4'] = 69,
    ['5'] = 70,  ['6'] = 71, ['7'] = 72, ['8'] = 73, ['9'] = 74, ['0'] = 75, ['%'] = 76,
    ['^'] = 77,  ['*'] = 78, ['{'] = 79, ['}'] = 80, ['='] = 81, ['#'] = 82, ['/'] = 83,
    ['\\'] = 84, ['$'] = 85, [163] = 86, ['['] = 87, [']'] = 88, ['<'] = 89, ['>'] = 90,
    ['\''] = 91, ['`'] = 92, ['~'] = 93};

void DrawText(string text, Tilemap *tilemap, v2 pos, f32 scale) {
    i32  iText = 0, iBox = 0, nextSpace = 0;
    bool addWhitespace = false;

    while (iText < text.len && iBox < (TILES_X * TILES_Y)) {
        if (!addWhitespace)
            for (i32 i = iText; i < text.len; i++) {
                if (text.data[i] == ' ' || (1 == text.len) - 1) {
                    nextSpace = i - iText;
                    break;
                }
            }

        addWhitespace          = nextSpace >= (TILES_X - (iBox % TILES_X));
        tilemap->instIdx[iBox] = !addWhitespace ? CharmapCoords[text.data[iText]] : 0;

        iText += !addWhitespace ? 1 : 0;
        iBox += 1;

        addWhitespace = addWhitespace && ((iBox % TILES_X) != 0);
    }

    if (text.len < (TILES_X * TILES_Y))
        for (i32 i = iBox; i < (TILES_X * TILES_Y); i++) {
            tilemap->instIdx[i] = 0;
        }

    DrawTilemap(tilemap, pos, scale, true);
}

inline bool V2InRect(v2 pos, Rect rectangle) {
    f32 left   = fminf(rectangle.x, rectangle.x + rectangle.w);
    f32 right  = fmaxf(rectangle.x, rectangle.x + rectangle.w);
    f32 top    = fminf(rectangle.y, rectangle.y + rectangle.h);
    f32 bottom = fmaxf(rectangle.y, rectangle.y + rectangle.h);

    return (pos.x >= left && pos.x <= right && pos.y >= top && pos.y <= bottom);
}

inline bool CollisionRectRect(Rect a, Rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

GuiState GuiButton(Rect button) {
    bool btn    = Input()->mouse.left == JustPressed;
    bool inRect = V2InRect(Mouse(), button);
    return btn && inRect ? GUI_PRESSED : inRect ? GUI_HOVERED : GUI_RELEASED;
}

void DrawRectangle(Rect rect, v4 color) {
    UseShader(E->Graphics.shaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform2f("res", GetResolution());
    SetUniform4f("color", color);
    DrawMesh(E->Graphics.squareMesh);
}

Dictionary NewDictionary(u32 size) {
    return (Dictionary){
        .Hash = SimpleHash,
        .data = ALLOC(sizeof(KVPair) * 4 * size),
        .size = size,
    };
}

void *DictUpsert(Dictionary *dict, const cstr key, void *data) {
    KVPair *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0 || strcmp(key, match[i].key) == 0) {
            match[i] = (KVPair){key, data};
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s exceeded maximum number of collisions\n", __func__, key);
    return 0;
}
void *DictInsert(Dictionary *dict, const cstr key, void *data) {
    KVPair *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) {
            match[i] = (KVPair){key, data};
            return match[i].data;
        }

        if (strcmp(key, match[i].key) == 0) {
            printf("[Warning] [%s] Key %s is already inserted\n", __func__, key);
            return match[i].data;
        }
    }
    printf("[Warning] [%s] Key %s exceeded maximum number of collisions\n", __func__, key);
    return 0;
}
void *DictUpdate(Dictionary *dict, const cstr key, void *data) {
    KVPair *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) break;
        if (strcmp(key, match[i].key) == 0) {
            match[i].data = data;
            return match[i].data;
        }
    }
    printf("[Warning] [%s] Key %s not found\n", __func__, key);
    return 0;
}
void *DictGet(const Dictionary *dict, const cstr key) {
    LARGE_INTEGER freq, start, end;
    KVPair *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) break;
        if (strcmp(key, match[i].key) == 0) {
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s not found\n", __func__, key);
    return 0;
}