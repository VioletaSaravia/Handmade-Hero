#pragma once

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ==== BASE TYPES =====

#define internal static
#define global static
#define persist static

typedef float_t  f32;
typedef double_t f64;

#define PI32 3.14159265f
#define PI64 3.141592653589793
#define PI PI32
#define TAU32 6.2831853f
#define TAU64 6.283185307179586
#define TAU TAU32

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef char *cstr;

typedef struct {
    cstr data;
    u32  len;
} Array;

typedef Array string;
string        String(cstr data) {
    string result;
    result.data = data;
    result.len  = (u32)strlen(data);
    return result;
}

typedef union {
    struct {
        f32 x, y;
    };
    struct {
        f32 w, h;
    };
} v2;

inline v2 v2Add(v2 a, v2 b) {
    return (v2){a.x + b.x, a.y + b.y};
}

inline v2 v2Sub(v2 a, v2 b) {
    return (v2){a.x - b.x, a.y - b.y};
}

inline v2 v2Scale(v2 a, f32 s) {
    return (v2){a.x * s, a.y * s};
}

typedef union {
    struct {
        i32 x, y;
    };
    struct {
        i32 w, h;
    };
} v2i;

typedef union {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
} v3;

typedef union {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
} v4;

typedef union {
    struct {
        v2 pos, size;
    };
    struct {
        f32 x, y, w, h;
    };
} Rect;
inline bool V2InRect(v2 pos, Rect rectangle);

typedef struct {
    v2  pos;
    f32 radius;
} Circle;

typedef struct {
    v2 *verts;
    u32 count;
} Poly;

// ===== MEMORY =====

// u8 *Alloc(MemRegion *buf, u32 size);
// u8 *RingAlloc(MemRegion *buf, u32 size);

typedef struct {
    u32 count, size;
    u8 *data;
} MemRegion;
MemRegion NewMemRegion(u32 size);

#if defined(_MSC_VER)
#include <intrin.h>
#define PopCnt64(x) __popcnt64(x)
#elif defined(__EMSCRIPTEN__) || defined(__GNUC__) || defined(__clang__)
#define PopCnt64(x) __builtin_popcountll(x)
#else
#error "Unsupported compiler"
#endif

#define ALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

// ===== MATH =====

f32 Rand() {
    return (f32)rand() / INT16_MAX;
}

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a <= b ? a : b)

inline v2 Normalize(v2 val) {
    f32 len = sqrtf(val.x * val.x + val.y * val.y);
    if (len > 0.0f) {
        val.x /= len;
        val.y /= len;
    }
    return val;
}

inline v2 Scale(v2 vec, f32 s) {
    return (v2){
        .x = vec.x * s,
        .y = vec.y * s,
    };
}

inline f32 Distance(v2 a, v2 b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    return (sqrtf(dx * dx + dy * dy));
}

inline v2 Direction(v2 a, v2 b) {
    return Normalize((v2){b.x - a.x, b.y - a.y});
}

inline f32 Angle(v2 vec) {
    return atan2f(vec.y, vec.x);
}

inline f32 Length(v2 vec) {
    return sqrtf(vec.x * vec.x + vec.y * vec.y);
}

v2 CollisionNormal(Rect rectA, Rect rectB) {
    float dx = (rectA.x + rectA.w / 2) - (rectB.x + rectB.w / 2);
    float dy = (rectA.y + rectA.h / 2) - (rectB.y + rectB.h / 2);
    float px = (rectA.w + rectB.w) / 2 - fabsf(dx);
    float py = (rectA.h + rectB.h) / 2 - fabsf(dy);

    v2 normal = {0, 0};

    if (px < py) {
        // Collision is horizontal
        if (dx > 0)
            normal.x = 1; // From left
        else
            normal.x = -1; // From right
    } else {
        // Collision is vertical
        if (dy > 0)
            normal.y = 1; // From top
        else
            normal.y = -1; // From bottom
    }

    return normal;
}

inline v2 MoveBy(v2 a, v2 b, f32 amount) {
    v2  to   = (v2){b.x - a.x, b.y - a.y};
    f32 dist = Distance(a, b);

    if (dist == 0.0f) return (v2){0};

    return dist < amount ? to : Scale(Scale(to, 1.0f / dist), amount);
}

inline f32 f32Abs(f32 x) {
    return x < 0.0f ? -x : x;
}

#define EPSILON 0.01f
inline bool IsEq(f32 a, f32 b) {
    return f32Abs(a - b) < EPSILON;
}

inline bool IsEqV2(v2 a, v2 b) {
    return IsEq(a.x, b.x) && IsEq(a.y, b.y);
}

inline f32 CrossV2(v2 a, v2 b, v2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

inline f32 DistV2(v2 a, v2 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    return dx * dx + dy * dy;
}

typedef struct {
    v2 p0, p1, p2, p3;
} Bezier;

v2 bezierPoint(Bezier b, float t) {
    float u   = 1.0f - t;
    float tt  = t * t;
    float uu  = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    v2 p = {0};
    p.x  = uuu * b.p0.x + 3 * uu * t * b.p1.x + 3 * u * tt * b.p2.x + ttt * b.p3.x;

    p.y = uuu * b.p0.y + 3 * uu * t * b.p1.y + 3 * u * tt * b.p2.y + ttt * b.p3.y;

    return p;
}

// ===== ALGORITHMS =====

#define MAX_COLLISIONS 4
typedef struct {
    cstr  key;
    void *data;
} KVPair;

typedef struct {
    u32 (*Hash)(const cstr);
    KVPair (*data)[MAX_COLLISIONS];
    u32 size;
} Dictionary;
Dictionary NewDictionary(u32 size);
void      *DictUpsert(Dictionary *dict, const cstr key, void *data);
void      *DictInsert(Dictionary *dict, const cstr key, void *data);
void      *DictUpdate(Dictionary *dict, const cstr key, void *data);
void      *DictGet(const Dictionary *dict, const cstr key);

u32 SimpleHash(cstr str) {
    u32 hash = 216;
    while (*str) {
        hash ^= (char)(*str++);
        hash *= 167;
    }
    return hash;
}

typedef struct {
    f32 _k1, _k2, _k3;
    f32 freq, damp, resp;
} Damper;

void DamperSetK(Damper *val) {
    val->_k1 = val->damp / (PI * val->freq);
    val->_k2 = 1 / ((TAU * val->freq) * (TAU * val->freq));
    val->_k3 = val->resp * val->damp / (TAU * val->freq);
}

void DamperSet(Damper *val, f32 f, f32 z, f32 r) {
    val->freq = f;
    val->damp = z;
    val->resp = r;

    DamperSetK(val);
}

Damper NewDamper(f32 f, f32 z, f32 r) {
    Damper result = (Damper){
        .freq = f,
        .damp = z,
        .resp = r,
    };
    DamperSetK(&result);
    return result;
}

typedef struct {
    f32  y, _xp, _yd;
    bool started, enabled;
} f32d;

f32d Newf32d(f32 x0) {
    return (f32d){
        .y       = x0,
        ._xp     = x0,
        .started = true,
        .enabled = true,
    };
}

void f32dUpdate(f32d *val, const Damper *damper, f32 delta, f32 x) {
    // if (!val->started) {}

    if (!val->enabled) {
        val->y = x;
        return;
    }

    f32 xd   = (x - val->_xp) / delta;
    val->_xp = x;
    val->y   = val->y + delta * val->_yd;

    f32 k2Stable = MAX(damper->_k2, 1.1 * (delta * delta / 4 + delta * damper->_k1 / 2));
    val->_yd =
        val->_yd + delta * (x + damper->_k3 * xd - val->y - damper->_k1 * val->_yd) / k2Stable;
}

Rect BoundingBoxOfSelection(const v2 *vects, const u32 count, const b64 selMap) {
    f32 minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX;
    for (u64 i = 0; i < count; i++) {
        if (!(selMap & (1ull << i))) continue;

        v2 vec = vects[i];
        if (vec.x < minX) minX = vec.x;
        if (vec.y < minY) minY = vec.y;
        if (vec.x > maxX) maxX = vec.x;
        if (vec.y > maxY) maxY = vec.y;
    }
    return (Rect){minX, minY, maxX - minX, maxY - minY};
}

global v2 pivot;
i32       ConvexHullCmp(const void *a, const void *b) {
    v2  p = *(v2 *)a;
    v2  q = *(v2 *)b;
    f32 c = CrossV2(pivot, p, q);
    if (c < 0) return 1;
    if (c > 0) return -1;
    // If collinear, nearest first
    return (DistV2(pivot, p) < DistV2(pivot, q)) ? -1 : 1;
}

// TODO No sorting in place
Poly ConvexHullOfSelection(v2 *points, u32 count, b32 bitmap) {
    Poly result = {0};
    if (count < 3) {
        MemRegion region = NewMemRegion(sizeof(v2) * count);
        result.verts     = (v2 *)region.data;
        result.count     = count;
        for (u32 i = 0; i < count; ++i) {
            result.verts[i] = points[i];
        }
        return result;
    }

    // Find pivot (lowest y, then lowest x)
    u32 pi = 0;
    for (u32 i = 1; i < count; ++i) {
        if (!(bitmap & (1 << i))) continue;

        if (points[i].y < points[pi].y ||
            (points[i].y == points[pi].y && points[i].x < points[pi].x)) {
            pi = i;
        }
    }
    // Swap pivot to first position
    v2 temp    = points[0];
    points[0]  = points[pi];
    points[pi] = temp;
    pivot      = points[0];

    // Sort remaining points by polar angle
    qsort(points + 1, count - 1, sizeof(v2), ConvexHullCmp);

    // Build hull
    v2 *hull  = (v2 *)malloc(sizeof(v2) * count);
    u32 h     = 0;
    hull[h++] = points[0];
    hull[h++] = points[1];

    for (u32 i = 2; i < count; ++i) {
        if (!(bitmap & (1 << i))) continue;

        while (h >= 2 && CrossV2(hull[h - 2], hull[h - 1], points[i]) <= 0) {
            --h;
        }
        hull[h++] = points[i];
    }

    result.verts = hull;
    result.count = h;
    return result;
}

// ===== IMAGE =====

#define WHITE (v4){1, 1, 1, 1}
#define BLACK (v4){0, 0, 0, 1}

typedef struct {
    u8 b, g, r, a;
} BMPColor;

typedef enum {
    RGB,
    RLE8,
    RLE4,
    BITFIELDS,
    JPEG,
    PNG,
    ALPHABITFIELDS,
    CMYK,
    CMYKRLE8,
    CMYKRLE4,
} BMPCompression;

typedef struct {
    // BMFH
    u16 bf_type;
    u32 bf_size;
    u16 bf_reserved1;
    u16 bf_reserved2;
    u32 bf_off_bits;

    // BMIH
    u32            size;
    i32            width;
    i32            height;
    u16            planes;
    u16            bit_count;
    BMPCompression compression;
    u32            size_image;
    v2i            pels_per_meter;
    u32            clr_used;
    u32            clr_important;

    // DATA
    BMPColor rgbq[256];
    u8       line_data[1024];
} AseBMP32x32;

typedef struct {
    u8 *data;
    u64 size;
} Image;

Image LoadBMP32x32Image(cstr path);

// ===== INPUT =====

typedef enum {
    PAD_Up,
    PAD_Down,
    PAD_Left,
    PAD_Right,
    PAD_A,
    PAD_B,
    PAD_X,
    PAD_Y,
    PAD_ShL,
    PAD_ShR,
    PAD_Start,
    PAD_Back,
    PAD_ThumbL,
    PAD_ThumbR,
    PAD_ButtonCount,
} GamepadButton;

typedef enum { JustReleased, Released, Pressed, JustPressed } ButtonState;

typedef struct {
    bool        connected, notAnalog;
    ButtonState buttons[PAD_ButtonCount];
    f32         trigLStart, trigLEnd, trigRStart, trigREnd;
    v2          lStart, lEnd, rStart, rEnd;
    f32         minX, minY, maxX, maxY;
} GamepadState;

typedef enum {
    KEY_Ret      = 0x0D,
    KEY_Shift    = 0x10,
    KEY_Ctrl     = 0x11,
    KEY_Esc      = 0x1B,
    KEY_Space    = 0x20,
    KEY_KeyLeft  = 0x25,
    KEY_KeyUp    = 0x26,
    KEY_KeyRight = 0x27,
    KEY_KeyDown  = 0x28,
    KEY_F1       = 0x70,
    KEY_F2       = 0x71,
    KEY_F3       = 0x72,
    KEY_F4       = 0x73,
    KEY_F5       = 0x74,
    KEY_F6       = 0x75,
    KEY_F7       = 0x76,
    KEY_F8       = 0x77,
    KEY_F9       = 0x78,
    KEY_F10      = 0x79,
    KEY_F11      = 0x7A,
    KEY_F12      = 0x7B,
    KEY_COUNT    = 0xFF, // 255?
} Key;

typedef struct {
    ButtonState left, right, middle;
    i16         wheel;
    v2          pos;
} MouseState;

typedef enum {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_ACCEPT,
    ACTION_CANCEL,
    ACTION_COUNT,
} Action;

typedef enum {
    INPUT_Keyboard,
    INPUT_Mouse,
    INPUT_Gamepad1,
    INPUT_Gamepad2,
    INPUT_Gamepad3,
    INPUT_Gamepad4 // etc.
} InputSource;

#define MAX_MODS 4

typedef struct {
    InputSource source;
    u32         key;
    u32         mods[MAX_MODS];
} Keybind;

#define MAX_KEYBINDS 3

internal void ProcessKeyboard(ButtonState *keys, bool *running);
internal void ProcessGamepads(GamepadState *gamepads);
internal void ProcessMouse(MouseState *mouse);

// ===== AUDIO =====

typedef enum { ONESHOT, LOOPING, HELD } PlaybackType;

typedef struct SoundBuffer SoundBuffer;
typedef struct Sound       Sound;

Sound LoadSound(cstr path, PlaybackType type);
void  PlaySound(Sound sound);
void  PauseSound(Sound sound);
void  StopSound(Sound sound);
void  ResumeSound(Sound sound);

// ===== GRAPHICS =====

typedef struct {
    v2  pos;
    f32 scale;
    f32 speed;
} Camera;
void CameraBegin(Camera cam);
void CameraEnd();
v2   GetResolution();
v2   Mouse();
v2   MouseInWorld(Camera cam);

typedef struct {
    u32 id;
#ifdef DEBUG
    cstr vertPath, fragPath;
    u64  vertWrite, fragWrite;
#endif
} Shader;
Shader NewShader(cstr vertFile, cstr fragFile);
void   UseShader(Shader shader);
void   ReloadShader(Shader *shader);
void   SetUniform1i(cstr name, i32 value);
void   SetUniform2i(cstr name, v2i value);
void   SetUniform1f(cstr name, f32 value);
void   SetUniform1fv(cstr name, f32 *value, u64 count);
void   SetUniform2f(cstr name, v2 value);
void   SetUniform3f(cstr name, v3 value);
void   SetUniform4f(cstr name, v4 value);
void   SetUniform1b(cstr name, bool value);
void   PrintShaderError(u32 shader);
void   PrintProgramError(u32 program);

typedef struct {
    u32 vao;
} Mesh;
Mesh NewMesh(f32 *verts, u64 vertCount, u32 *indices, u64 idxCount);
void DrawMesh(Mesh mesh);

typedef enum {
    SHADER_Default,
    SHADER_Tiled,
    SHADER_Rect,
    SHADER_Line,
    SHADER_COUNT,
} Shaders;

typedef struct {
    u32    fbo, tex, rbo, vao;
    Shader shader;
} Framebuffer;
Framebuffer NewFramebuffer(const cstr fragPath);
void        UseFramebuffer(Framebuffer shader);
void        DrawFramebuffer(Framebuffer shader);
void        ResizeFramebuffer(Framebuffer shader);

typedef struct Texture {
    u32 id;
    i32 nChan;
    v2i size;
} Texture;
Texture NewTexture(const cstr path);
Texture NewTextureFromMemory(void *memory, v2i size);
void    UseTexture(Texture tex);
void    DrawTexture(Texture tex, v2 pos, f32 scale);

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
VAO  InitVao(VAO result);
void InitVaoFromShader(const char *shaderFile, VAO *vao);
void UseVAO(const VAO *vao);

typedef struct Tileset {
    Texture tex;
    v2      tileSize;
} Tileset;
Tileset NewTileset(const cstr path, v2 tileSize);

typedef struct Tilemap {
    Tileset tileset;
    u32     vao, ebo;
    u32     vbo, idVbo, posVbo;
    u32     colForeVbo, colBackVbo;
    v2      size;
    i32    *instIdx;
    v4     *instForeColor;
    v4     *instBackColor;
    u32     width;
} Tilemap;
Tilemap NewTilemap(Tileset tileset, v2 size);
void    DrawTilemap(const Tilemap *tilemap, v2 pos, f32 scale, bool twoColor, i32 width);
void    TilemapLoadCsv(Tilemap *tilemap, cstr csvPath);
void    DrawText(const cstr text, v2 pos, i32 width, f32 scale);

// TODO Shader attrib parser
typedef struct {
    u32  location;
    char type[32];
    char name[64];
} ShaderAttrib;
void ParseShaderAttribs(const char *filename, ShaderAttrib *attribs, int *attribCount);
u32  GlslTypeToEnum(const char *type);

void ClearScreen(v4 color);

// ===== ENGINE =====

typedef struct GameSettings GameSettings;
GameSettings               *Settings();

typedef struct InputCtx InputCtx;
InputCtx               *Input();

typedef struct AudioCtx AudioCtx;
internal void           InitAudio(AudioCtx *ctx);
internal void           ShutdownAudio(AudioCtx *audio);
AudioCtx               *Audio();

typedef struct WindowCtx WindowCtx;
WindowCtx               *Window();
internal WindowCtx       InitWindow();
void                     ToggleFullscreen();

typedef struct GraphicsCtx GraphicsCtx;
GraphicsCtx               *Graphics();
GraphicsCtx                InitGraphics(const WindowCtx *ctx, const GameSettings *settings);

typedef struct TimingCtx TimingCtx;
internal TimingCtx       InitTiming(f32 refreshRate);

typedef struct EngineCtx EngineCtx;

typedef struct {
    void (*Setup)();
    void (*Init)();
    void (*Update)();
    void (*Draw)();
} GameProcs;
extern void  GameReloadMemory(void *memory);
extern void  GameLoad(void (*setup)(), void (*init)(), void (*update)(), void (*draw)());
extern void  GameEngineInit();
extern void  GameEngineUpdate();
extern void  GameEngineShutdown();
extern void *GameGetMemory();
extern bool  GameIsRunning();

f32 Delta();

// ===== FILES =====

u64    GetLastWriteTime(cstr file);
string ReadEntireFile(const char *filename);

typedef struct {
    u8  *data;
    u64 *map; // TODO
    u64  count;
} DataPak;
DataPak NewDataPak(cstr folder);
DataPak LoadDataPak(cstr pakFile);
string  DataPakRead(cstr name);

// ===== GUI =====

typedef enum {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER_CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
} Anchor;

typedef enum {
    GUI_RELEASED,
    GUI_HOVERED,
    GUI_PRESSED,
} GuiState;
GuiState GuiButton(Rect button, cstr text);
GuiState GuiSlider(f32 *val, Rect coords, f32 from, f32 to);

// ===== DEBUG =====

void DrawRectangle(Rect rect, v4 color, f32 radius);
void DrawLine(v2 from, v2 to, v4 color, f32 thickness);
void DrawPoly(Poly poly, v4 color, f32 thickness);