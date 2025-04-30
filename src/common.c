#include "common.h"

#undef ALLOC
#define ALLOC(size) Alloc(&E->Memory, size)
#undef FREE
#define FREE(ptr) DeAlloc(&E->Memory, ptr)

string String(cstr data) {
    string result;
    result.data = data;
    result.len  = (u32)strlen(data);
    return result;
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

u32 SimpleHash(cstr str) {
    u32 hash = 216;
    // for (u32 i = 0; i < 4 && *str; i++) {
    while (*str) {
        hash ^= (char)(*str++);
        hash *= 167;
    }
    return hash;
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

bool V2InRect(v2 pos, Rect rectangle) {
    f32 left   = fminf(rectangle.x, rectangle.x + rectangle.w);
    f32 right  = fmaxf(rectangle.x, rectangle.x + rectangle.w);
    f32 top    = fminf(rectangle.y, rectangle.y + rectangle.h);
    f32 bottom = fmaxf(rectangle.y, rectangle.y + rectangle.h);

    return (pos.x >= left && pos.x <= right && pos.y >= top && pos.y <= bottom);
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