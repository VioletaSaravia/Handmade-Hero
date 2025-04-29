#include "graphics.h"

void CameraBegin(Camera cam) {
    Graphics()->cam = cam;
}

void CameraEnd() {
    Graphics()->cam = (Camera){0};
}

v2 GetResolution() {
    v2i result = {0};
    SDL_GetWindowSize(Window()->window, &result.x, &result.y);
    return (v2){(f32)result.x, (f32)result.y};
}

v2 MouseInWorld(Camera cam) {
    v2 mouse = Mouse();
    return (v2){mouse.x + cam.pos.x, mouse.y + cam.pos.y};
}

Shader ShaderFromPath(cstr vertFile, cstr fragFile) {
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
    while (!vertSrc.data) vertSrc = ReadEntireFile(vertFile);
    while (!fragSrc.data) fragSrc = ReadEntireFile(fragFile);

    u32 vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc.data, 0);
    glCompileShader(vertShader);

    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        ShaderPrintError(vertShader);
        return (Shader){0};
    }

    u32 fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc.data, 0);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        ShaderPrintError(fragShader);
        return (Shader){0};
    }

    result.id = glCreateProgram();
    glAttachShader(result.id, vertShader);
    glAttachShader(result.id, fragShader);
    glLinkProgram(result.id);
    glGetProgramiv(result.id, GL_LINK_STATUS, &ok);
    if (!ok) {
        ShaderPrintProgramError(result.id);
        return (Shader){0};
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return result;
}

void ShaderUse(Shader shader) {
    glUseProgram(shader.id);
    Graphics()->activeShader = shader.id;
    SetUniform1i("t", Time());
    SetUniform1f("gScale", 1);  // TODO Implement bottom and remove
    SetUniform1f("camZoom", 0); // Graphics()->cam.scale);
    SetUniform2f("camera", Graphics()->cam.pos);
    SetUniform2f("res", GetResolution());
}

void ShaderReload(Shader *shader) {
#ifdef DEBUG
    u64 vertTime = GetLastWriteTime(shader->vertPath);
    u64 fragTime = GetLastWriteTime(shader->fragPath);
    if (vertTime <= shader->vertWrite && fragTime <= shader->fragWrite) return;

    Shader newShader = ShaderFromPath(shader->vertPath, shader->fragPath);
    if (newShader.id != 0) {
        glDeleteProgram(shader->id);
        *shader = newShader;
    } else {
        shader->vertWrite = vertTime;
        shader->fragWrite = fragTime;
    }
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
    glUniform1fv(glGetUniformLocation(Graphics()->activeShader, name), (i32)count, value);
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

void ShaderPrintError(u32 shader) {
    i32 logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetShaderInfoLog(shader, logLength, 0, infoLog);

    LOG_ERROR(infoLog);

    free(infoLog);
}

void ShaderPrintProgramError(u32 program) {
    i32 logLength = 0;
    glGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetProgramInfoLog(program, logLength, 0, infoLog);

    LOG_ERROR(infoLog);

    free(infoLog);
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

    Shader shader = ShaderFromPath("shaders\\post.vert", fragPath);
    result.shader = shader;

    return result;
}

void Framebufferuse(Framebuffer shader) {
    glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);
}

void FramebufferDraw(Framebuffer shader) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ClearScreen((v4){0});

    ShaderUse(shader.shader);

    glBindVertexArray(shader.vao);
    glBindTexture(GL_TEXTURE_2D, shader.tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void FramebufferResize(Framebuffer shader) {
    v2 res = GetResolution();
    glBindTexture(GL_TEXTURE_2D, shader.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (i32)res.w, (i32)res.h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, shader.tex);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (i32)res.w, (i32)res.h);
}

Texture NewTexture(cstr path) {
    SDL_Surface *img = IMG_Load(path);
    if (!img) {
        LOG_ERROR("Couldn't load texture: %s", SDL_GetError());
        return (Texture){0};
    }

    Texture result = TextureFromMemory((void *)img->pixels, (v2i){img->w, img->h});
    SDL_free(img);
    return result;
}

Texture TextureFromMemory(void *memory, v2i size) {
    Texture result = {.size = size};

    glGenTextures(1, &result.id);
    glBindTexture(GL_TEXTURE_2D, result.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    u32 format = GL_RGBA; // RGB for BMP

    glTexImage2D(GL_TEXTURE_2D, 0, format, result.size.x, result.size.y, 0, format,
                 GL_UNSIGNED_BYTE, memory);
    // GL_GenerateMipmap(GL_TEXTURE_2D)

    return result;
}

void TextureUse(Texture tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
}

void VAOInit(VAO *vao, Arena *alloc) {
    glGenVertexArrays(1, &vao->id);
    glBindVertexArray(vao->id);

    for (u64 i = 0; i < vao->count; i++) {
        BOInit(&vao->objs[i], alloc);
    }
}

void BOInit(BO *obj, Arena *alloc) {
    u32 err = 0;
    glGenBuffers(1, &obj->id);
    u32 target = obj->ebo ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glBindBuffer(target, obj->id);

    if (alloc) obj->buf = Alloc(alloc, 128); // TODO: How do we handle the allocation?
    glBufferData(target, obj->bufSize, obj->buf, (u32)obj->drawType);
    err = glGetError();
    if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);

    if (obj->ebo) return;

    u64 offset = 0;
    for (size_t i = 0; i < obj->attribCount; i++) {
        Attrib attrib = obj->attribs[i];
        switch (attrib.type) {
        case ATTRIB_FLOAT:
            glVertexAttribPointer(i, attrib.count, GL_FLOAT, GL_FALSE, obj->stride, (void *)offset);
            err = glGetError();
            if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
            offset += attrib.count * sizeof(f32);
            glEnableVertexAttribArray(i);
            err = glGetError();
            if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
            if (obj->perInstance) glVertexAttribDivisor(i, 1);

            break;

        case ATTRIB_INT:
            glVertexAttribIPointer(i, attrib.count, GL_INT, obj->stride, (void *)offset);
            err = glGetError();
            if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
            glEnableVertexAttribArray(i);
            err = glGetError();
            if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
            if (obj->perInstance) glVertexAttribDivisor(i, 1);
            break;

        default: LOG_ERROR("Unsupported attribute type"); return;
        }
    }
}

void VAOUse(VAO vao) {
    glBindVertexArray(vao.id);
}

void BOUpdate(BO obj) {
    u32 target = obj.ebo ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glBindBuffer(target, obj.id);
    // NOTE: Reemplaza todo el buffer actualmente, pero podría tener offset y size!
    glBufferSubData(target, 0, obj.bufSize, obj.buf);
}

static AttribType attribTypeFromGLSL(cstr typeName) {
    if (strcmp(typeName, "float") == 0 || strncmp(typeName, "vec", 3) == 0) {
        return ATTRIB_FLOAT;
    } else if (strcmp(typeName, "int") == 0 || strncmp(typeName, "ivec", 4) == 0) {
        return ATTRIB_INT;
    }
    LOG_ERROR("Attrib type not found");
    return 0;
}

static u32 attribCountFromGLSL(cstr typeName) {
    if (strcmp(typeName, "float") == 0 || strcmp(typeName, "int") == 0) return 1;
    if (strncmp(typeName, "vec", 3) == 0 || strncmp(typeName, "ivec", 4) == 0)
        return (u32)(typeName[3] - '0');

    return 1;
}

static u32 attribTypeSize(AttribType type) {
    switch (type) {
    case ATTRIB_FLOAT: return 4;
    case ATTRIB_INT: return 4;
    default: return 0;
    }
}

VAO VAOFromShader(cstr path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Failed to open shader file");
        exit(1);
    }

    VAO  vao     = {0};
    u32  boIndex = 0;
    BO  *bo      = NULL;
    bool indexed = false;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "+BUFFER")) {
            // Start new BO
            if (boIndex >= 8) {
                fprintf(stderr, "Too many buffers in shader\n");
                exit(1);
            }
            bo  = &vao.objs[boIndex++];
            *bo = (BO){.drawType    = strstr(line, "+DYNAMIC") != 0 ? BUF_DYNAMIC : BUF_STATIC,
                       .ebo         = false,
                       .perInstance = strstr(line, "+INSTANCED") != 0,
                       .buf         = 0,
                       .bufSize     = 0,
                       .stride      = 0,
                       .attribCount = 0};

            if (strstr(line, "+INDEXED") != 0) indexed = true;
            continue;
        }

        if (bo && strstr(line, "layout(")) {
            cstr typeStart = strstr(line, "in ");
            if (!typeStart) continue;
            typeStart += 3;

            char typeName[32];
            sscanf(typeStart, "%31s", typeName);

            Attrib attrib;
            attrib.type  = attribTypeFromGLSL(typeName);
            attrib.count = attribCountFromGLSL(typeName);

            if (bo->attribCount >= 4) {
                fprintf(stderr, "Too many attributes in buffer\n");
                exit(1);
            }

            bo->attribs[bo->attribCount++] = attrib;
            bo->stride += attrib.count * attribTypeSize(attrib.type);
        }
    }

    fclose(file);

    // NOTE(viole): If any of the buffers where tagged +INDEXED, we add an EBO.
    // Only one though! Would two ever be necessary? Also, should this be attached
    // to the shader or the mesh loading?
    if (indexed && vao.count < 8 && boIndex > 0) {
        BO *ebo = &vao.objs[boIndex++];
        *ebo    = (BO){.drawType    = BUF_STATIC,
                       .ebo         = true,
                       .perInstance = false,
                       .buf         = 0,
                       .bufSize     = 0,
                       .stride      = 0,
                       .attribCount = 0};
    }

    vao.count = boIndex;
    return vao;
}

void DrawInstances(u32 count) {
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);
}

void ClearScreen(v4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void DrawMouse() {
    ShaderUse(Graphics()->builtinShaders[SHADER_Default]);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", Mouse());
    SetUniform1f("scale", 1);
    Texture mouseTex = Graphics()->builtinTextures[TEX_Mouse];
    SetUniform2f("size", (v2){mouseTex.size.x, mouseTex.size.y});
    SetUniform4f("color", WHITE);
    TextureUse(mouseTex);
    VAOUse(Graphics()->builtinVAOs[VAO_SQUARE]);
    DrawInstances(1);
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

const i32 MonogramCoords[256] = {
    [' '] = 0,  ['!'] = 1,  ['"'] = 2,  ['#'] = 3,  ['$'] = 4,   ['%'] = 5,  ['&'] = 6,  ['\''] = 7,
    ['('] = 8,  [')'] = 9,  ['*'] = 10, ['+'] = 11, [','] = 12,  ['-'] = 13, ['.'] = 14, ['/'] = 15,
    ['0'] = 16, ['1'] = 17, ['2'] = 18, ['3'] = 19, ['4'] = 20,  ['5'] = 21, ['6'] = 22, ['7'] = 23,
    ['8'] = 24, ['9'] = 25, [':'] = 26, [';'] = 27, ['<'] = 28,  ['='] = 29, ['>'] = 30, ['?'] = 31,
    ['@'] = 32, ['A'] = 33, ['B'] = 34, ['C'] = 35, ['D'] = 36,  ['E'] = 37, ['F'] = 38, ['G'] = 39,
    ['H'] = 40, ['I'] = 41, ['J'] = 42, ['K'] = 43, ['L'] = 44,  ['M'] = 45, ['N'] = 46, ['O'] = 47,
    ['P'] = 48, ['Q'] = 49, ['R'] = 50, ['S'] = 51, ['T'] = 52,  ['U'] = 53, ['V'] = 54, ['W'] = 55,
    ['X'] = 56, ['Y'] = 57, ['Z'] = 58, ['['] = 59, ['\\'] = 60, [']'] = 61, ['^'] = 62, ['_'] = 63,
    ['`'] = 64, ['a'] = 65, ['b'] = 66, ['c'] = 67, ['d'] = 68,  ['e'] = 69, ['f'] = 70, ['g'] = 71,
    ['h'] = 72, ['i'] = 73, ['j'] = 74, ['k'] = 75, ['l'] = 76,  ['m'] = 77, ['n'] = 78, ['o'] = 79,
    ['p'] = 80, ['q'] = 81, ['r'] = 82, ['s'] = 83, ['t'] = 84,  ['u'] = 85, ['v'] = 86, ['w'] = 87,
    ['x'] = 88, ['y'] = 89, ['z'] = 90, ['{'] = 91, ['|'] = 92,  ['}'] = 93, ['~'] = 94};

u32 MapStringToTextBox(cstr text, i32 width, v2 tSize, void *buf) {
    if (!width) width = INT32_MAX;

    u64 textLen = strlen(text);
    u32 iText = 0, iBox = 0, nextSpace = 0;

    bool bold = false, italics = false;
    bool addWhitespace = false;
    f32  size          = tSize.x * tSize.y;
    while (iText < textLen && iBox < size) {
        if (!addWhitespace)
            for (u32 i = iText; i < textLen; i++) {
                if (text[i] == ' ' || (i == (textLen - 1))) {
                    nextSpace = i - iText;
                    break;
                }
            }

        addWhitespace = nextSpace >= width - (iBox % width);

        if (text[iText] == '\b') {
            bold ^= true;
            iText++;
            continue;
        }
        if (text[iText] == '\t') {
            italics ^= true;
            iText++;
            continue;
        }

        ((i32 *)buf)[iBox] = !addWhitespace ? MonogramCoords[text[iText]] : 0;
        // if (bold) tilemap->fontVbo[iBox] = 1;
        // if (italics) tilemap->fontVbo[iBox] = 2;
        // if (bold && italics) tilemap->fontVbo[iBox] = 3;

        if (!addWhitespace) iText++;
        iBox++;

        addWhitespace = addWhitespace && ((iBox % width) != 0);
    }

    if (textLen < size)
        for (i32 i = iBox; i < size; i++) {
            ((i32 *)buf)[i] = 0;
        }

    return iBox;
}

void DrawText(VAO *vao, Texture tex, v2 tSize, v2 tileSize, const cstr text, v2 pos, i32 width,
              f32 scale) {
    u32 iBox = MapStringToTextBox(text, width, tSize, vao->objs[1].buf);

    ShaderUse(Graphics()->builtinShaders[SHADER_Tiled]);

    SetUniform2f("tile_size", tSize);
    v2i tilesetSize = (v2i){
        tex.size.x / tileSize.x,
        tex.size.y / tileSize.y,
    };

    SetUniform2i("tileset_size", tilesetSize);
    SetUniform1i("width", width ? width : INT32_MAX);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", pos);
    SetUniform1f("scale", scale ? scale * 2 : 2);
    SetUniform1i("tex0", 0);

    TextureUse(tex);

    VAOUse(*vao);

    // f32 tileSize = tilemap->size.x * tilemap->size.y;
    BOUpdate(vao->objs[1]);

    DrawInstances(iBox);
}

bool V2InRect(v2 pos, Rect rectangle) {
    f32 left   = fminf(rectangle.x, rectangle.x + rectangle.w);
    f32 right  = fmaxf(rectangle.x, rectangle.x + rectangle.w);
    f32 top    = fminf(rectangle.y, rectangle.y + rectangle.h);
    f32 bottom = fmaxf(rectangle.y, rectangle.y + rectangle.h);

    return (pos.x >= left && pos.x <= right && pos.y >= top && pos.y <= bottom);
}

bool CollisionRectRect(Rect a, Rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

void DrawRectangle(Rect rect, v4 color, f32 radius) {
    u32 err = 0;
    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform2f("res", GetResolution());
    SetUniform4f("color", color);
    SetUniform1f("radius", radius);
    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    err = glGetError();
    if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    err = glGetError();
    if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
    glBindVertexArray(0);
}

void DrawLine(v2 from, v2 to, v4 color, f32 thickness) {
    ShaderUse(Graphics()->builtinShaders[SHADER_Line]);
    SetUniform2f("pos", from);
    SetUniform1f("thickness", thickness ? thickness : 1);
    SetUniform2f("size", v2Sub(to, from));
    SetUniform2f("res", GetResolution());
    SetUniform4f("color", color);
    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    glBindVertexArray(0);
}

void DrawPoly(Poly poly, v4 color, f32 thickness) {
    // TODO Inefficient
    for (size_t i = 0; i < poly.count - 1; i++) {
        DrawLine(poly.verts[i], poly.verts[i + 1], color, thickness);
    }
}

void DrawCircle(v2 pos, v4 color, f32 radius) {}

global f32 squareVerts[] = {1, 1, 0, 1, 0, 1, -1, 0, 1, 1, -1, -1, 1, 0, 1, -1, 1, 0, 0, 0};
global u32 squareIds[]   = {0, 1, 3, 1, 2, 3};

GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings) {
    GraphicsCtx result = {0};

    result.builtinShaders[SHADER_Default] = ShaderFromPath(0, 0);
    result.builtinShaders[SHADER_Tiled]   = ShaderFromPath("shaders\\tiled.vert", 0);
    result.builtinShaders[SHADER_Text] = ShaderFromPath("shaders\\text.vert", "shaders\\text.frag");
    result.builtinShaders[SHADER_Rect] = ShaderFromPath("shaders\\rect.vert", "shaders\\rect.frag");
    result.builtinShaders[SHADER_Line] = ShaderFromPath("shaders\\line.vert", "shaders\\line.frag");

    result.builtinTextures[TEX_Mouse] = NewTexture("data\\pointer.png");

    result.builtinVAOs[VAO_SQUARE]                 = VAOFromShader("shaders\\default.vert");
    result.builtinVAOs[VAO_SQUARE].objs[0].buf     = squareVerts;
    result.builtinVAOs[VAO_SQUARE].objs[0].bufSize = 20 * sizeof(f32);
    result.builtinVAOs[VAO_SQUARE].objs[1].buf     = squareIds;
    result.builtinVAOs[VAO_SQUARE].objs[1].bufSize = 6 * sizeof(i32);
    VAOInit(&result.builtinVAOs[VAO_SQUARE], 0);

    result.builtinVAOs[VAO_TEXT] = VAOFromShader("shaders\\text.vert");
    // TODO Allocs
    VAOInit(&result.builtinVAOs[VAO_TEXT], 0);

    result.postprocessing = NewFramebuffer("shaders\\post.frag");

    return result;
}

void UpdateGraphics(GraphicsCtx *ctx, void (*draw)()) {
    ShaderReload(&ctx->postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ShaderReload(&ctx->builtinShaders[i]);
    Framebufferuse(ctx->postprocessing);
    {
        ClearScreen((v4){0.3f, 0.4f, 0.4f, 1.0f});
        draw();
        CameraEnd();
        if (!Settings()->disableMouse) DrawMouse();
    }
    FramebufferDraw(ctx->postprocessing);
}