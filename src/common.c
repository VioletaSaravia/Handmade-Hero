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

u64 GetLastWriteTime(cstr file) {
    u64         result = 0;
    struct stat fileStat;
    if (stat(file, &fileStat) != 0) {
        perror("Failed to get file attributes");
        return 0;
    }
    result = (u64)fileStat.st_mtime;
    return result;
}

string ReadEntireFile(const cstr filename) {
    string result = {0};
    result.data   = SDL_LoadFile(filename, &result.len);
    return result;
}