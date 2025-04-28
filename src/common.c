#include "common.h"

#undef ALLOC
#define ALLOC(size) Alloc(&E->Memory, size)
#undef FREE
#define FREE(ptr) DeAlloc(&E->Memory, ptr)

Arena NewArena(void *memory, u64 size) {
    return (Arena){.buf = memory, .used = 0, .size = size};
}

void *Alloc(Arena *arena, u64 size) {
    if (arena->used + size >= arena->size) {
        LOG_ERROR("Arena is full");
        return 0;
    }

    void *result = (void *)(&arena->buf[arena->used]);
    arena->used += size;
    return result;
}

void *RingAlloc(Arena *arena, u64 size) {
    if (arena->used + size >= arena->size) {
        arena->used = 0;
    }

    void *result = (void *)(&arena->buf[arena->used]);
    arena->used += size;
    return result;
}

void DeAlloc(Arena *arena, void *ptr) {
    i64 ptrDiff = (u8 *)ptr - arena->buf;

    // TODO is this correct?
    if (ptrDiff < 0 || ptrDiff >= (u64)arena->buf + arena->size) {
        LOG_ERROR("Pointer is not in arena");
        return;
    }

    arena->used = ptrDiff;
}

void Empty(Arena *arena) {
    arena->used = 0;
}

ComponentTable NewComponentTable(u32 buckets, u32 entSize) {
    return (ComponentTable){
        .Hash    = SimpleHash,
        .data    = SDL_malloc(sizeof(Component) * 4 * buckets),
        .size    = buckets,
        .entLen  = 0,
        .entSize = entSize,
    };
}

void *CompUpsert(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0 || strcmp(key, match[i].key) == 0) {
            match[i] = (Component){.key = key, .data = data};
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s exceeded maximum number of collisions\n", __func__, key);
    return 0;
}
void *CompInsert(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) {
            match[i] = (Component){.key = key, .data = data};
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
void *CompUpdate(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
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
void *CompGet(const ComponentTable *dict, const cstr key) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) break;
        if (strcmp(key, match[i].key) == 0) {
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s not found\n", __func__, key);
    return 0;
}

// ===== FILES =====

export u64 GetLastWriteTime(cstr file) {
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

string ReadEntireFile(const char *filename) {
    string result = {0};
    HANDLE file   = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Invalid handle");
        return (string){0};
    }

    u32 size = GetFileSize(file, 0);
    if (size == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        CloseHandle(file);
        LOG_ERROR("Invalid file size");
        return (string){0};
    }

    u64 allocSize = size + 1;

    result.data = SDL_malloc(allocSize);
    if (!result.data) {
        LOG_ERROR("Couldn't allocate data");
        CloseHandle(file);
        return (string){0};
    }

    bool success = ReadFile(file, result.data, size, &result.len, 0);
    CloseHandle(file);

    if (!success || result.len != size) {
        LOG_ERROR("Couldn't read entire file");
        SDL_free(result.data);
        return (string){0};
    }
    result.data[size] = '\0';
    return result;
}