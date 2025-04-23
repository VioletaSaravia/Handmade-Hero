#include "win32.c"

i32 main() {
    const char *ecs_names[50] = {
        "position", "velocity",    "rotation", "scale",     "sprite",  "animation", "health",
        "mana",     "stamina",     "attack",   "defense",   "ai",      "player",    "enemy",
        "npc",      "collider",    "hitbox",   "hurtbox",   "sound",   "camera",    "input",
        "score",    "lifetime",    "timer",    "inventory", "weapon",  "armor",     "quest",
        "dialogue", "pathfinding", "waypoint", "team",      "faction", "xp",        "level",
        "loot",     "drop",        "pickup",   "buff",      "debuff",  "effect",    "particle",
        "light",    "shadow",      "script",   "trigger",   "zone",    "portal",    "checkpoint",
        "spawn"};

    ComponentTable bla = NewComponentTable(128);
    u32        val = 1337;

    for (u64 i = 0; i < 50; i++) {
        cstr name = (cstr)ecs_names[i];
        printf("String %s hashed to %d\n", name, SimpleHash(name) % 256);
        CompInsert(&bla, name, &val);
    }
}