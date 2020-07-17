#include <stdio.h>
#include "include/gdc.h"

#define debug_print(fmt, ...) \
        do { fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

GDPTR* _createPtr(const char* baseModName, int n_offsets, size_t* offsets) {
    GDPTR* ptr = (GDPTR*) malloc(sizeof(GDPTR));

    ptr->baseModuleName = baseModName;
    ptr->n_offsets = n_offsets;
    ptr->offsets = offsets;

    return ptr;
}

#define createPtr(ModName, ...) ({                        \
    size_t _x[] = { __VA_ARGS__ };                  \
    _createPtr(ModName, sizeof(_x) / sizeof(_x[0]), _x);  \
})

#define DEF_PTR(PtrName, ModName, ...) GDPTR* PtrName = NULL; \
{ \
    size_t _x[] = { __VA_ARGS__ }; \
    PtrName = _createPtr(ModName, sizeof(_x) / sizeof(size_t), _x); \
}

int main() {
    DEF_PTR(PTR_IS_END_OF_ATTEMPT_SCREEN_SHOWN, GD_PROC_NAME, 0x322290, 0xCC, 0x28, 0x00, 0x8, 0x4BD)

    // Pointers stolen from NeKit's gd.py

    DEF_PTR(PTR_IS_IN_LEVEL, GD_PROC_NAME, 0x3222D0, 0x164, 0x22C, 0x114)
    DEF_PTR(PTR_LEVEL_LENGTH, GD_PROC_NAME, 0x3222D0, 0x164, 0x3B4)
    DEF_PTR(PTR_TOTAL_ATTEMPTS, GD_PROC_NAME, 0x3222D0, 0x164, 0x22C, 0x114, 0x218)
    DEF_PTR(PTR_LEVEL_ID_ACCURATE, GD_PROC_NAME, 0x3222D0, 0x164, 0x22C, 0x114, 0xF8)
    DEF_PTR(PTR_TOTAL_JUMPS, GD_PROC_NAME, 0x3222D0, 0x164, 0x22C, 0x114, 0x224)
    DEF_PTR(PTR_BEST_PERC_NORMAL, GD_PROC_NAME, 0x3222D0, 0x164, 0x22C, 0x114, 0x248)

    DEF_PTR(PTR_IS_PLAYER_DEAD, GD_PROC_NAME, 0x3222D0, 0x164, 0x39C)
    DEF_PTR(PTR_PLAYER_X, GD_PROC_NAME, 0x3222D0, 0x164, 0x224, 0x67C)
    DEF_PTR(PTR_CURRENT_ATTEMPT, GD_PROC_NAME, 0x3222D0, 0x164, 0x4A8)
    DEF_PTR(PTR_IS_PRACTICE_MODE, GD_PROC_NAME, 0x3222D0, 0x164, 0x495)

    GDHANDLE* handle = new_handle();

    printf("Binding to Geometry Dash...\n");

    while (!is_bound(handle)) {
        if (!bind_gd(handle)) {
            Sleep(100);
        }
    }

    printf("Successfully bound to Geometry Dash\n");

    bool capturedDeath = false;

    while (is_game_running(handle)) {
        bool in_level = false;

        if (!read(handle, PTR_IS_IN_LEVEL, &in_level, sizeof(bool))) {
            debug_print("Could not read PTR_IS_IN_LEVEL: %lu\n", GetLastError());
            break;
        }
        if (!in_level) {
            continue;
        }

        bool player_dead = false;

        if (!read(handle, PTR_IS_PLAYER_DEAD, &player_dead, sizeof(bool))) {
            debug_print("Could not read PTR_IS_PLAYER_DEAD: %lu\n", GetLastError());
            break;
        }

        if (!player_dead) {
            capturedDeath = false;
            goto nextIter;
        } else {
            if (capturedDeath) {
                goto nextIter;
            }

            capturedDeath = true;

            int lvlId;
            float lvlLen;
            float playerX;

            if (!read(handle, PTR_LEVEL_ID_ACCURATE, &lvlId, sizeof(int))) {
                debug_print("Could not read PTR_LEVEL_ID_ACCURATE: %lu\n", GetLastError());
                break;
            }

            if (!read(handle, PTR_LEVEL_LENGTH, &lvlLen, sizeof(float))) {
                debug_print("Could not read PTR_LEVEL_LENGTH: %lu\n", GetLastError());
                break;
            }

            if (!read(handle, PTR_PLAYER_X, &playerX, sizeof(float))) {
                debug_print("Could not read PTR_PLAYER_X: %lu\n", GetLastError());
                break;
            }

            float percentage = (playerX / lvlLen) * 100;

            printf("Died at %f%% on Level with ID %d\n", percentage, lvlId);
            goto nextIter;
        }

        nextIter:
        Sleep(1000 / 60);
    }

    printf("GeometryDash closed");

    return 0;
}