#pragma once
#include "engine.h"

typedef struct {
    v2  pos;
    f32 zoom;
    f32 speed;
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
void   ShaderPrintError(u32 shader);
void   ShaderPrintProgramError(u32 program);
void   SetUniform1i(cstr name, i32 value);
void   SetUniform2i(cstr name, v2i value);
void   SetUniform1f(cstr name, f32 value);
void   SetUniform1fv(cstr name, f32 *value, u64 count);
void   SetUniform2f(cstr name, v2 value);
void   SetUniform3f(cstr name, v3 value);
void   SetUniform4f(cstr name, v4 value);
void   SetUniform1b(cstr name, bool value);

typedef enum {
    ATTRIB_FLOAT = GL_FLOAT,
    ATTRIB_INT   = GL_INT,
} AttribType;

typedef struct {
    AttribType type;
    u32        count;
} Attrib;

typedef enum {
    BUF_STATIC  = GL_STATIC_DRAW,
    BUF_DYNAMIC = GL_DYNAMIC_DRAW,
} DrawType;

typedef enum { TYPE_VBO, TYPE_EBO, TYPE_FBO, TYPE_RBO } BOType;

typedef struct {
    u32      id;
    DrawType drawType;
    bool     ebo;
    bool     perInstance;
    void    *buf;
    u32      bufSize;
    u32      stride;
    Attrib   attribs[4];
    u32      attribCount;
} BO;
void BOInit(BO *obj, Arena *alloc);
void BOUpdate(BO obj);

typedef struct {
    u32 id;
    BO  objs[8];
    u32 count;
} VAO;
VAO  VAOFromShader(cstr path);
void VAOInit(VAO *vao, Arena *alloc);
void VAOUse(VAO vao);

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
void    TextureUse(Texture tex);

typedef enum {
    SHADER_Default,
    SHADER_Tiled,
    SHADER_Text,
    SHADER_Rect,
    SHADER_Line,
    SHADER_COUNT,
} BuiltinShaders;

typedef enum { VAO_SQUARE, VAO_TEXT, VAO_COUNT } BuiltinVAOs;

typedef enum {
    TEX_Mouse,
    TEX_COUNT,
} BuiltinTextures;

struct GraphicsCtx {
    Camera      cam;
    u32         activeShader;
    VAO         builtinVAOs[VAO_COUNT];
    Texture     builtinTextures[TEX_COUNT];
    Shader      builtinShaders[SHADER_COUNT];
    Framebuffer postprocessing;
};
GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings);
void        UpdateGraphics(GraphicsCtx *ctx, void (*draw)());

v2   GetResolution();
v2   Mouse();
v2   MouseInWorld(Camera cam);
void ClearScreen(v4 color);
void DrawInstances(u32 count);
void DrawRectangle(Rect rect, v4 color, f32 radius);
void DrawLine(v2 from, v2 to, v4 color, f32 thickness);
void DrawCircle(v2 pos, v4 color, f32 radius);
void DrawPoly(Poly poly, v4 color, f32 thickness);

#define WHITE (v4){1, 1, 1, 1}