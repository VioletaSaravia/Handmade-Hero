#pragma once
#include "engine.h"

typedef struct {
    v2  pos;
    f32 zoom;
    f32 speed;
    f32 rotation;
} Camera;
void CameraBegin(Camera cam);
void CameraEnd();

typedef struct {
    u32 id;
#ifdef DEBUG
    cstr vertPath, fragPath;
    u64  vertWrite, fragWrite;
#endif
} Shader;
Shader ShaderFromPath(cstr vertFile, cstr fragFile);
void   ShaderUse(Shader shader);
void   ShaderReload(Shader *shader);
void   ShaderPrintError(u32 shader, char* shaderPath);
void   ShaderPrintProgramError(u32 program);
void   SetUniform1i(cstr name, i32 value);
void   SetUniform2i(cstr name, v2i value);
void   SetUniform1f(cstr name, f32 value);
void   SetUniform1fv(cstr name, f32 *value, u64 count);
void   SetUniform2f(cstr name, v2 value);
void   SetUniform3f(cstr name, v3 value);
void   SetUniform4f(cstr name, v4 value);
void   SetUniform1b(cstr name, bool value);

typedef struct {
    u32    fbo, tex, rbo, vao;
    Shader shader;
} Framebuffer;
Framebuffer NewFramebuffer(const cstr fragPath);
void        Framebufferuse(Framebuffer shader);
void        FramebufferDraw(Framebuffer shader);
void        FramebufferResize(Framebuffer shader);

typedef struct Texture {
    u32 id;
    i32 nChan;
    v2i size;
} Texture;
Texture NewTexture(const cstr path);
Texture TextureFromMemory(void *memory, v2i size);
void    TextureUse(Texture tex, u32 i);

typedef enum {
    SHADER_Default,
    SHADER_Rect,
    SHADER_Circle,
    SHADER_Sdf,
    SHADER_COUNT,
} BuiltinShaders;

typedef enum { VAO_CUBE, VAO_SQUARE, VAO_LINE, VAO_COUNT } BuiltinVAOs;

typedef struct {
    u32 id;
} VAO;

typedef enum {
    TEX_NONE,
    TEX_COUNT,
} BuiltinTextures;

struct GraphicsCtx {
    Camera      cam;
    u32         activeShader;
    Shader      builtinShaders[SHADER_COUNT];
    Texture     builtinTextures[TEX_COUNT];
    VAO         builtinVAOs[VAO_COUNT];
    Framebuffer postprocessing;
};
intern GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings);
intern void        UpdateGraphics(GraphicsCtx *ctx, void (*draw)());

v2   GetResolution();
v2   Mouse();
v2   MouseDir();
v2   MouseInWorld(Camera cam);
void ClearScreen(v4 color);
void DrawInstances(u32 count);
void DrawElement();
void DrawRectangle(Rect rect, f32 rotation, v4 color, f32 rounding);
void DrawLine(v2 from, v2 to, v4 color);
void DrawCircle(v2 center, f32 radius, v4 color, bool line, f32 thickness);
void DrawPoly(Poly poly, v4 color);

#define COLOR_NULL (v4){0, 0, 0, 0}
#define WHITE (v4){1, 1, 1, 1}
#define BLACK (v4){0, 0, 0, 1}
#define RED (v4){1, 0, 0, 1}