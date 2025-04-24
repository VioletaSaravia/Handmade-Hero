#pragma once
#include "common.h"

typedef struct {
    v2  pos;
    f32 zoom;
    f32 speed;
} Camera;
void CameraBegin(Camera cam);
void CameraEnd();

v2 GetResolution();
v2 Mouse();
v2 MouseInWorld(Camera cam);

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

void DrawInstances(u32 count);

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

// typedef struct Tileset {
//     Texture tex;
//     v2      tileSize;
// } Tileset;
// Tileset NewTileset(const cstr path, v2 tileSize);

// typedef struct Tilemap {
//     Tileset tileset;
//     u32     vao, ebo;
//     u32     vbo, idVbo, posVbo;
//     u32     colForeVbo, colBackVbo;
//     v2      size;
//     i32    *instIdx;
//     v4     *instForeColor;
//     v4     *instBackColor;
//     u32     width;
// } Tilemap;
// Tilemap NewTilemap(Tileset tileset, v2 size);
// void    TilemapDraw(const Tilemap *tilemap, v2 pos, f32 scale, i32 width);
// void    TilemapLoadCsv(Tilemap *tilemap, cstr csvPath);
// void    DrawText(VAO *vao, Texture tex, v2 tSize, v2 tileSize, const cstr text, v2 pos, i32
// width,
//                  f32 scale);

void ClearScreen(v4 color);

void DrawRectangle(Rect rect, v4 color, f32 radius);
void DrawLine(v2 from, v2 to, v4 color, f32 thickness);
void DrawCircle(v2 pos, v4 color, f32 radius);
void DrawPoly(Poly poly, v4 color, f32 thickness);