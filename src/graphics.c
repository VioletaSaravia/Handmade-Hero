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

    // TODO(violeta): Esto es sólo para evitar errores al hacer reload.
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
    u32 err = glGetError();
    if (err != GL_NO_ERROR) LOG_ERROR("Error using shader: %d", err);

    Graphics()->activeShader = shader.id;
    // TODO Move to game loop beginning
    SetUniform2f("res", GetResolution());
    SetUniform1i("t", Time());
    SetUniform2f("camPos", Graphics()->cam.pos);
    SetUniform1f("camZoom", Graphics()->cam.zoom);
    SetUniform1f("camRotation", Graphics()->cam.rotation);
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

void TextureUse(Texture tex, u32 i) {
    glActiveTexture(0x84C0 + i);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    u32 err = glGetError();
    if (err != GL_NO_ERROR) LOG_ERROR("Error binding texture: %d", err);
}

void VAOInit(VAO *vao, Arena *alloc, u32 maxInstances) {
    glGenVertexArrays(1, &vao->id);
    glBindVertexArray(vao->id);
    u32 attribIdx = 0;

    for (u64 i = 0; i < vao->count; i++) {
        u32 err = BOInit(&vao->objs[i], alloc, maxInstances, &attribIdx);
        if (err != 0) {
            LOG_ERROR("Error initializing BO in VAO %s: %u", vao->source, err);
            return;
        }
    }

    glBindVertexArray(0);
}

u32 BOInit(BO *obj, Arena *alloc, u32 maxInstances, u32 *attribIdx) {
    u32 err = 0;
    glGenBuffers(1, &obj->id);
    u32 target = obj->ebo ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glBindBuffer(target, obj->id);

    u32 sizePerInstance = 0;
    for (size_t i = 0; i < obj->attribCount; i++) {
        u32 size = attribTypeSize(obj->attribs[i].type);
        sizePerInstance += (size * obj->attribs[i].count);
    }

    // Decide how large this BO's buffer should be, if it wasn't manually set previously, based on
    // maxInstances. TODO: What if it's not an instanced buffer AND it wasn't set manually?
    if (alloc && maxInstances > 0 && !obj->bufSize && !obj->buf) {
        obj->bufSize = sizePerInstance * maxInstances;
        obj->buf     = Alloc(alloc, obj->bufSize); // TODO: Align errors?
    }

    glBufferData(target, obj->bufSize, obj->buf, (u32)obj->drawType);
    err = glGetError();
    if (err != GL_NO_ERROR) return err;

    if (obj->ebo) return 0;

    u64 offset = 0;
    for (size_t i = 0; i < obj->attribCount; i++) {
        Attrib attrib = obj->attribs[i];
        u32    idx    = (*attribIdx)++;

        switch (attrib.type) {
        case ATTRIB_FLOAT:
            glVertexAttribPointer(idx, attrib.count, GL_FLOAT, GL_FALSE, obj->stride,
                                  (void *)offset);
            err = glGetError();
            if (err != GL_NO_ERROR) return err;
            offset += attrib.count * sizeof(f32);
            glEnableVertexAttribArray(idx);
            err = glGetError();
            if (err != GL_NO_ERROR) return err;
            if (obj->perInstance) glVertexAttribDivisor(idx, 1);

            break;

        case ATTRIB_INT:
            glVertexAttribIPointer(idx, attrib.count, GL_INT, obj->stride, (void *)offset);
            err = glGetError();
            if (err != GL_NO_ERROR) return err;
            glEnableVertexAttribArray(idx);
            err = glGetError();
            if (err != GL_NO_ERROR) return err;
            if (obj->perInstance) glVertexAttribDivisor(idx, 1);
            break;

        default: LOG_ERROR("Unsupported attribute type: %u", attrib.type); return -1;
        }
    }

    return 0;
}

void VAOUse(VAO vao) {
    glBindVertexArray(vao.id);
    u32 err = glGetError();
    if (err != GL_NO_ERROR) LOG_ERROR("Error binding VAO: %u", err);
}

void BOUpdate(BO obj) {
    u32 target = obj.ebo ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glBindBuffer(target, obj.id);
    // TODO: Reemplaza todo el buffer actualmente, pero podría tener offset y size
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

        if (bo && strstr(line, "layout(") && !(strstr(line, "//layout"))) {
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

    vao.count  = boIndex;
    vao.source = path;
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
    TextureUse(mouseTex, 0);
    VAOUse(Graphics()->builtinVAOs[VAO_SQUARE]);
    DrawInstances(1);
}

bool CollisionRectRect(Rect a, Rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

#define LOG_GL_ERROR(msg)                                                                          \
    do {                                                                                           \
        u32 __err = glGetError();                                                                  \
        if (__err != GL_NO_ERROR) LOG_ERROR(msg ": %u", __err);                                    \
    } while (0);

typedef enum { SHAPE_RECT, SHAPE_LINE, SHAPE_CIRCLE, SHAPE_POLYGON, SHAPE_COUNT } Shapes;

void DrawRectangle(Rect rect, f32 rotation, v4 color, f32 rounding, bool line, f32 thickness) {
    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform1f("rotation", rotation);

    SetUniform1i("shape", SHAPE_RECT);
    SetUniform1b("line", line);
    SetUniform1f("thickness", thickness);
    SetUniform1f("rounding", rounding);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawLine(v2 from, v2 to, v4 color, f32 thickness) {
    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", from);
    v2 size = v2Sub(to, from);
    // TODO Horrible.
    size = (v2){size.x == 0 ? 1 : size.x, size.y == 0 ? 1 : size.y};
    SetUniform2f("size", size);
    SetUniform1f("rotation", 0);

    SetUniform1i("shape", SHAPE_LINE);
    SetUniform1f("thickness", thickness);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawHex(v2 center, f32 radius, f32 rotation, v4 color, f32 rounding, bool line,
             f32 thickness) {
    Rect rect = {.x = center.x - radius, .y = center.y - radius, .w = radius * 2, .h = radius * 2};
    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform1f("rotation", rotation);

    SetUniform1i("shape", SHAPE_POLYGON);
    SetUniform1b("line", line);
    SetUniform1f("thickness", thickness);
    SetUniform1f("rounding", rounding);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawCircle(v2 center, f32 radius, v4 color, bool line, f32 thickness) {
    Rect rect = {.x = center.x - radius, .y = center.y - radius, .w = radius * 2, .h = radius * 2};

    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform1f("rotation", 0);

    SetUniform1i("shape", SHAPE_CIRCLE);
    SetUniform1b("line", line);
    SetUniform1f("thickness", thickness);
    SetUniform1f("rounding", 0);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawPoly(Poly poly, v4 color, f32 thickness) {
    for (u32 i = 0; i < poly.count - 1; i++) {
        DrawLine(poly.verts[i], poly.verts[i + 1], color, thickness);
    }
}

global f32 sqVerts[] = {1, 1, 0, 1, 0, 1, -1, 0, 1, 1, -1, -1, 1, 0, 1, -1, 1, 0, 0, 0};
global u32 sqIds[]   = {0, 1, 3, 1, 2, 3};

void VAOLoadMesh(VAO *vao, f32 *verts, u32 vertSize, u32 *ids, u32 idSize) {
    vao->objs[0].buf     = verts;
    vao->objs[0].bufSize = vertSize;

    for (u32 i = 0; i < vao->count; i++) {
        if (!vao->objs[i].ebo) continue;
        vao->objs[i].buf     = ids;
        vao->objs[i].bufSize = idSize;
    }
}

GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings) {
    GraphicsCtx result = {0};

    result.builtinShaders[SHADER_Default] = ShaderFromPath(0, 0);
    result.builtinShaders[SHADER_Rect] = ShaderFromPath("shaders\\rect.vert", "shaders\\rect.frag");

    result.builtinTextures[TEX_Mouse] = NewTexture("data\\pointer.png");

    result.builtinVAOs[VAO_SQUARE] = VAOFromShader("shaders\\default.vert");
    VAOLoadMesh(&result.builtinVAOs[VAO_SQUARE], sqVerts, sizeof(sqVerts), sqIds, sizeof(sqIds));
    VAOInit(&result.builtinVAOs[VAO_SQUARE], 0, 0);

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
        // if (!Settings()->disableMouse) DrawMouse();
    }
    FramebufferDraw(ctx->postprocessing);
}