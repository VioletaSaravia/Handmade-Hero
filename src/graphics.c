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

    // TODO(viole): Esto es sólo para evitar errores al hacer reload.
    // Cambiar por algo mejor.
    while (!vertSrc.data) vertSrc = ReadEntireFile(vertFile);
    while (!fragSrc.data) fragSrc = ReadEntireFile(fragFile);

    u32 vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc.data, 0);
    glCompileShader(vertShader);

    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        ShaderPrintError(vertShader, vertFile);
        return (Shader){0};
    }

    u32 fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc.data, 0);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        ShaderPrintError(fragShader, fragFile);
        return (Shader){0};
    }

    result.id = glCreateProgram();
    glAttachShader(result.id, vertShader);
    glAttachShader(result.id, fragShader);
    glLinkProgram(result.id);
    glGetProgramiv(result.id, GL_LINK_STATUS, &ok);
    if (!ok) {
        ShaderPrintProgramError(result.id);
        exit(-1);
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return result;
}

void ShaderUse(Shader shader) {
    glUseProgram(shader.id);
    LOG_GL_ERROR("Couldn't use shader program");

    Graphics()->activeShader = shader.id;
    SetUniform1i("t", Time());
    SetUniform2f("res", GetResolution());
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

void ShaderPrintError(u32 shader, char *shaderPath) {
    i32 logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    char *infoLog = malloc(logLength);
    glGetShaderInfoLog(shader, logLength, 0, infoLog);

    LOG_ERROR("%s - %s", shaderPath, infoLog);

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

void TextureEnd(u32 i) {
    glActiveTexture(GL_TEXTURE0 + i); // Same unit used before
    glBindTexture(GL_TEXTURE_2D, 0);
    u32 err = glGetError();
    if (err != GL_NO_ERROR) LOG_ERROR("Error binding texture: %d", err);
}

void DrawInstances(u32 count) {
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);
}

void DrawElement() {
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void ClearScreen(v4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool CollisionRectRect(Rect a, Rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

typedef enum { SHAPE_RECT, SHAPE_LINE, SHAPE_CIRCLE, SHAPE_HEXAGON, SHAPE_COUNT } Shapes;

void DrawRectangle(Rect rect, f32 rotation, v4 color, f32 radius) {
    ShaderUse(Graphics()->builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", v2Add(rect.pos, v2Scale(rect.size, 0.5)));
    SetUniform2f("size", rect.size);
    SetUniform1f("rotation", rotation);
    SetUniform1f("radius", radius);
    SetUniform1f("border", 100);

    SetUniform1i("tex0", 0);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        DrawElement();
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawTexture(Texture tex, v2 pos, f32 rotation) {
    TextureUse(tex, 0);
    DrawRectangle((Rect){pos.x, pos.y, tex.size.w, tex.size.h}, rotation, COLOR_NULL, 0);
    TextureEnd(0);
}

void DrawLine(v2 from, v2 to, v4 color) {
    ShaderUse(Graphics()->builtinShaders[SHADER_Default]);
    f32 swapY = from.y;
    from.y    = to.y;
    to.y      = swapY;
    SetUniform2f("pos", from);
    SetUniform2f("size", v2Sub(to, from));
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_LINE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        glDrawArrays(GL_LINES, 0, 2);
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawCircle(v2 center, f32 radius, v4 color, bool line, f32 thickness) {
    Rect rect = {.x = center.x, .y = center.y, .w = radius * 2, .h = radius * 2};

    ShaderUse(Graphics()->builtinShaders[SHADER_Circle]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform1f("rotation", 0);

    SetUniform1i("shape", SHAPE_CIRCLE);
    SetUniform1b("line", line);
    SetUniform1f("thickness", thickness);
    SetUniform1f("radius", 0);
    SetUniform4f("color", color);

    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    LOG_GL_ERROR("VAO binding failed");
    {
        DrawElement();
        LOG_GL_ERROR("Drawing failed");
    }
    glBindVertexArray(0);
}

void DrawPoly(Poly poly, v4 color) {
    for (u32 i = 0; i < poly.count - 1; i++) {
        DrawLine(poly.verts[i], poly.verts[i + 1], color);
    }
}

VAO LoadSquareMesh() {
    persist f32 sqVerts[] = {
        //  x     y     u     v
        -0.5f, -0.5f, 0.0f, 0.0f, // bottom left
        0.5f,  -0.5f, 1.0f, 0.0f, // bottom right
        0.5f,  0.5f,  1.0f, 1.0f, // top right
        -0.5f, 0.5f,  0.0f, 1.0f  // top left
    };
    persist u32 sqIds[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    VAO result = {0};

    glGenVertexArrays(1, &result.id);
    u32 vbo = 0, ebo = 0;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(result.id);
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sqVerts), sqVerts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sqIds), sqIds, GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(0));

        // UV attribute (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    }
    glBindVertexArray(0);

    return result;
}

VAO LoadLineMesh() {
    persist float lineVertices[] = {-1, -1, 1, 1};

    VAO result = {0};
    u32 vbo    = 0;
    glGenVertexArrays(1, &result.id);
    glGenBuffers(1, &vbo);

    glBindVertexArray(result.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    return result;
}

GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings) {
    GraphicsCtx result = {0};

    result.builtinShaders[SHADER_Default] = ShaderFromPath(0, 0);
    result.builtinShaders[SHADER_Rect] =
        ShaderFromPath("shaders\\default2d.vert", "shaders\\rect.frag");
    result.builtinShaders[SHADER_Circle] =
        ShaderFromPath("shaders\\default2d.vert", "shaders\\circle.frag");
    result.builtinShaders[SHADER_Sdf] =
        ShaderFromPath("shaders\\shapes.vert", "shaders\\shapes.frag");
    result.builtinShaders[SHADER_Tiles] =
        ShaderFromPath("shaders\\default2d.vert", "shaders\\tiles.frag");

    result.builtinVAOs[VAO_CUBE]   = LoadSquareMesh();
    result.builtinVAOs[VAO_SQUARE] = LoadSquareMesh();
    result.builtinVAOs[VAO_LINE]   = LoadLineMesh();

    result.postprocessing = NewFramebuffer("shaders\\post.frag");

    SDL_CHECK(TTF_Init(), "Failed to initialize SDL_TTF");

    return result;
}

void UpdateGraphics(GraphicsCtx *ctx, void (*draw)()) {
    ShaderUse(Graphics()->builtinShaders[0]);

    ShaderReload(&ctx->postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ShaderReload(&ctx->builtinShaders[i]);
    Framebufferuse(ctx->postprocessing);
    {
        ClearScreen((v4){0.3f, 0.4f, 0.4f, 1.0f});
        draw();
        CameraEnd();
    }
    FramebufferDraw(ctx->postprocessing);
}

typedef struct {
    u64       length;
    i32       wrap;
    TTF_Font *font;
    Texture   tex;
} Text;

Text NewText(cstr text, cstr fontPath, f32 size, i32 wrapChars) {
    Text result = {.length = 0,
                   .wrap   = wrapChars * size,
                   .font   = TTF_OpenFont(fontPath, size),
                   .tex    = (Texture){0}};
    SDL_CHECK(result.font, "Failed to open font");

    SDL_Surface *rendered = TTF_RenderText_Blended_Wrapped( // TODO Remove allocation
        result.font, text, result.length, (SDL_Color){255, 255, 255, 255}, result.wrap);
    SDL_CHECK(rendered, "Failed to render text");
    rendered = SDL_ConvertSurface(
        rendered, SDL_PIXELFORMAT_ABGR8888); // TODO Leaks memory from the first rendered
    SDL_CHECK(rendered, "Failed to convert surface format");

    glGenTextures(1, &result.tex.id);
    glBindTexture(GL_TEXTURE_2D, result.tex.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rendered->w, rendered->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rendered->pixels);
    result.tex.size = (v2i){rendered->w, rendered->h};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_DestroySurface(rendered);
    return result;
}

void DrawText(Text text, v2 pos) {
    DrawTexture(text.tex, pos, 0);
}