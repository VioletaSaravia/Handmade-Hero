#include "win32.h"

void CameraBegin(Camera cam) {
    Graphics()->cam = cam;
}

void CameraEnd() {
    Graphics()->cam = (Camera){0};
}

v2 GetResolution() {
    RECT clientRect = {0};
    GetClientRect(Window()->window, &clientRect);

    return (v2){clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
}

inline v2 Mouse() {
    return Input()->mouse.pos;
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

    printf("[Error] [%s] Shader compilation failed: %s\n", __func__, infoLog);

    free(infoLog);
}

void ShaderPrintProgramError(u32 program) {
    i32 logLength = 0;
    glGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetProgramInfoLog(program, logLength, 0, infoLog);

    printf("[Error] [%s] Program linking failed: %s\n", __func__, infoLog);

    free(infoLog);
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

void DrawMesh(Mesh mesh) {}

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
    v2i size;
    i32 nChan;

    string file = ReadEntireFile(path);

    u8 *img = stbi_load_from_memory(file.data, file.len, &size.x, &size.y, &nChan, 0);

    if (!img) {
        LOG_ERROR("Couldn't load texture");
        return (Texture){0};
    }

    Texture result = TextureFromMemory((void *)img, size);
    stbi_image_free(img);
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

Tileset NewTileset(const cstr path, v2 tileSize) {
    return (Tileset){.tex = NewTexture(path), .tileSize = tileSize};
}

Tilemap NewTilemap(Tileset tileset, v2 size) {
    Tilemap result       = {.size = size};
    result.instIdx       = ALLOC(sizeof(i32) * size.x * size.y);
    result.instForeColor = ALLOC(sizeof(v4) * size.x * size.y);
    result.instBackColor = ALLOC(sizeof(v4) * size.x * size.y);

    persist f32 quadVertices[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1};
    persist i32 quadIndices[]  = {0, 1, 2, 2, 3, 0};

    result.tileset = tileset;

    result.width = size.x * tileset.tileSize.x;

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

    // INSTANCE ID VBO
    glBindBuffer(GL_ARRAY_BUFFER, result.idVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(i32) * size.x * size.y, result.instIdx, GL_DYNAMIC_DRAW);

    glVertexAttribIPointer(2, 1, GL_INT, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    return result;
}

void VAOInit(VAO *vao, Arena *alloc) {
    glGenVertexArrays(1, &vao->id);
    glBindVertexArray(vao->id);

    for (u64 i = 0; i < vao->count; i++) {
        BOInit(&vao->objs[i], alloc);
    }
}

void BOInit(BO *obj, Arena *alloc) {
    glGenBuffers(1, &obj->id);
    u32 target = obj->ebo ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glBindBuffer(target, obj->id);

    obj->buf = ArenaAlloc(alloc, 128); // TODO: How do we handle the allocation?
    glBufferData(target, obj->bufSize, obj->buf, (u32)obj->drawType);

    if (obj->ebo) return;

    u64 offset = 0;
    for (size_t i = 0; i < obj->attribCount; i++) {
        Attrib attrib = obj->attribs[i];
        switch (attrib.type) {
        case ATTRIB_FLOAT:
            glVertexAttribPointer(i, attrib.count, GL_FLOAT, GL_FALSE, obj->stride, (void *)offset);
            offset += attrib.count * sizeof(f32);
            glEnableVertexAttribArray(i);
            if (obj->perInstance) glVertexAttribDivisor(i, 1);

            break;

        case ATTRIB_INT:
            glVertexAttribIPointer(i, attrib.count, GL_INT, obj->stride, (void *)offset);
            glEnableVertexAttribArray(i);
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
                       .ebo         = strstr(line, "+INDEXED") != 0,
                       .perInstance = strstr(line, "+INSTANCED") != 0,
                       .buf         = 0,
                       .bufSize     = 0,
                       .stride      = 0,
                       .attribCount = 0};

            if (bo->ebo) indexed = true;
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

void TilemapDraw(const Tilemap *tilemap, v2 pos, f32 scale, i32 width) {
    ShaderUse(Graphics()->shaders[SHADER_Tiled]);
    SetUniform2f("tile_size", tilemap->tileset.tileSize);
    v2i tilesetSize = (v2i){
        tilemap->tileset.tex.size.x / tilemap->tileset.tileSize.x,
        tilemap->tileset.tex.size.y / tilemap->tileset.tileSize.y,
    };

    SetUniform2i("tileset_size", tilesetSize);
    SetUniform1i("width", width ? width : INT32_MAX);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", pos);
    SetUniform1f("scale", scale ? scale * 2 : 2);
    SetUniform1i("tex0", 0);

    TextureUse(tilemap->tileset.tex);

    VAOUse((VAO){tilemap->vao});

    f32 size = tilemap->size.x * tilemap->size.y;
    // BOUpdate((BO){
    //     .ebo     = false,
    //     .id      = tilemap->idVbo,
    //     .buf     = tilemap->instIdx,
    //     .bufSize = sizeof(i32) * size,
    // });

    DrawInstances(size);
}

void TilemapLoadCsv(Tilemap *tilemap, cstr csvPath) {
    u8 *data = 0;
    // TODO Load Tiled CSVs
    for (int i = 0; i < tilemap->size.x * tilemap->size.y; i++) {
        i32 num             = 0;
        tilemap->instIdx[i] = num;
    }
}

void ClearScreen(v4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}