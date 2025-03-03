#pragma once

#include <strsafe.h>  // StringCbPrintfA()
#include <Windows.h>
#include <xaudio2.h>
#include <Xinput.h>

#include "glad/glad.h"
#include "handmade.h"

// <Debug>
#ifndef DEBUG
#define WIN32_CHECK(func) (func)
#else
#define WIN32_CHECK(func)                                                        \
    if (FAILED(func)) {                                                          \
        char buf[32];                                                            \
        StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %d\n", GetLastError()); \
        OutputDebugStringA(buf);                                                 \
    }
#endif

#ifndef DEBUG
#define WIN32_LOG(str)
#else
#define WIN32_LOG(str)                                            \
    char buf[128];                                                \
    StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %s\n", str); \
    OutputDebugStringA(buf);
#endif

void *PlatformReadEntireFile(const char *filename) {
    HANDLE handle =
        CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
    Assert(handle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER size     = {};
    LARGE_INTEGER sizeRead = {};

    bool ok = GetFileSizeEx(handle, &size);
    Assert(ok);
    Assert(size.HighPart == 0);  // 4GB+ reads not supported >:(

    void *result = VirtualAlloc(0, size.LowPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    ok = ReadFile(handle, result, size.LowPart, &sizeRead.LowPart, 0);
    Assert(ok);

    CloseHandle(handle);

    Assert(sizeRead.QuadPart == size.QuadPart);

    return result;
};

// TODO(violeta): Untested
bool PlatformWriteEntireFile(char *filename, u32 size, void *memory) {
    HANDLE handle =
        CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (handle == INVALID_HANDLE_VALUE) {
        WIN32_LOG("Error open file for writing");
        return false;
    }

    DWORD sizeWritten = 0;

    bool ok = WriteFile(handle, memory, size, &sizeWritten, 0);

    CloseHandle(handle);

    if (!ok || sizeWritten != size) {
        WIN32_LOG("Error writing file");
        return false;
    }

    return true;
};

void PlatformFreeFileMemory(void *memory) { VirtualFree(memory, 0, MEM_RELEASE); };
// </Debug>

// global const f32 TriangleVertices[] = {
//     // positions        // colors
//     0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom right
//     -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom left
//     0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f   // top
// };

global const f32 RectVertices[] = {
    // positions        // colors
    0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f,  // top right
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom left
    -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f,  // top left
};
global const u32 RectIndices[] = {
    // note that we start from 0!
    0, 1, 3,  // first triangle
    1, 2, 3   // second triangle
};

struct Shader {
    u32 ID;
    // TODO(violeta): Paths, shader reloading
};
global Shader DefaultShader;

struct Object3D {
    u32    VAO;
    Shader shader;
};

global Object3D Objects[8];
global u32      ObjCount;

void ShaderSetInt(Shader shader, const char *name, i32 value) {
    glUniform1i(glGetUniformLocation(shader.ID, name), value);
}
void ShaderSetFloat(Shader shader, const char *name, f32 value) {
    glUniform1f(glGetUniformLocation(shader.ID, name), value);
}

internal Shader InitShader(const char *vertFile, const char *fragFile) {
    const char *vertSource =
        (const char *)PlatformReadEntireFile(vertFile ? vertFile : "../shaders/default.vert");
    const char *fragSource =
        (const char *)PlatformReadEntireFile(fragFile ? fragFile : "../shaders/default.frag");

    u32 vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSource, 0);
    glCompileShader(vertShader);

    i32 ok;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char infoLog[512];
        glGetShaderInfoLog(vertShader, 512, 0, infoLog);
        char buf[512];
        WIN32_CHECK(StringCbPrintfA(buf, sizeof(buf), "[SHADER ERROR] %s\n", infoLog));
        OutputDebugStringA(buf);
    }

    u32 fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSource, 0);
    glCompileShader(fragShader);

    Shader result = {.ID = glCreateProgram()};
    glAttachShader(result.ID, vertShader);
    glAttachShader(result.ID, fragShader);
    glLinkProgram(result.ID);

    glGetProgramiv(result.ID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char infoLog[512];
        glGetProgramInfoLog(result.ID, 512, 0, infoLog);
        char buf[512];
        WIN32_CHECK(StringCbPrintfA(buf, sizeof(buf), "[SHADER ERROR] %s\n", infoLog));
        OutputDebugStringA(buf);
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return result;
}

// TODO(violeta): Objects are assumed to have vertex paint and indeces. Is any other case necessary?
internal Object3D InitObject3D(Shader shader, const f32 *vertices, u32 vSize, const u32 *indices,
                               u32 iSize) {
    Object3D result = {.shader = shader};
    glGenVertexArrays(1, &result.VAO);
    glBindVertexArray(result.VAO);

    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // learnopengl p. 29:
    // The fourth parameter specifies how we want the graphics card to manage the given data:
    // - GL_STREAM_DRAW: the data is set only once and used by the GPU at most a few times.
    // - GL_STATIC_DRAW: the data is set only once and used many times.
    // - GL_DYNAMIC_DRAW: the data is changed a lot and used many times.
    glBufferData(GL_ARRAY_BUFFER, vSize, vertices, GL_STATIC_DRAW);

    u32 ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iSize, indices, GL_STATIC_DRAW);

    // learnopengl p. 34:
    //  - The first parameter specifies which vertex attribute we want to configure. Remember that
    //  we specified the location of the position vertex attribute in the vertex shader with layout
    //  (location = 0).
    //  - The next argument specifies the size of the vertex attribute. The vertex attribute is a
    //  vec3 so it is composed of 3 values.
    //  - The third argument specifies the type of the data which is GL_FLOAT (a vec* in GLSL
    //  consists of floating point values).
    //  - The next argument specifies if we want the data to be normalized. If we’re inputting
    //  integer data types (int, byte) and we’ve set this to GL_TRUE, the integer data is normalized
    //  to 0 (or-1 for signed data) and 1 when converted to float. This is not relevant for us so
    //  we’ll leave this at GL_FALSE.
    //  - The fifth argument is known as the stride and tells us the space between consecutive
    //  vertex attributes. Since the next set of position data is located exactly 3 times the size
    //  of a float away we specify that value as the stride. Note that since we know that the array
    //  is tightly packed (there is no space between the next vertex attribute value) we could’ve
    //  also specified the stride as 0 to let OpenGL determine the stride (this only works when
    //  values are tightly packed). Whenever we have more vertex attributes we have to carefully
    //  define the spacing between each vertex attribute but we’ll get to see more examples of that
    //  later on.
    //  - The last parameter is of type void* and thus requires that weird cast. This is the offset
    //  of where the position data begins in the buffer. Since the position data is at the start of
    //  the data array this value is just 0.

    u32 stride = 6 * sizeof(f32);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    // Enable the vertex attribute with glEnableVertexAttribArray giving the vertex attribute
    // location as its argument; vertex attributes are disabled by default.
    glBindVertexArray(0);

    return result;
}

internal void DrawObject3D(Object3D obj) {
    glUseProgram(obj.shader.ID);
    glBindVertexArray(obj.VAO);

    ShaderSetFloat(obj.shader, "time", 0);
    ShaderSetFloat(obj.shader, "delta", 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}