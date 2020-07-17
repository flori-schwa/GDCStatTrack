//
// Created by Florian on 17.07.2020.
//

#ifndef TEST_GDC_H
#define TEST_GDC_H

#include <Windows.h>
#include <tlhelp32.h>
#include <stdbool.h>
#include <stdio.h>

#define GD_PROC_NAME "GeometryDash.exe"

typedef struct {
    HANDLE gd_BaseHandle;
    DWORD gd_ProcId;
    void* gd_BaseAddr;
    MODULEENTRY32* gd_Modules;
    int gd_NModules;
} GDHANDLE;

typedef struct {
    void* mod_BaseAddr;
    size_t mod_Size;
    char* mod_ProcName;
} GDMODINFO;

typedef struct {
    const char* baseModuleName;
    int n_offsets;
    size_t* offsets;
} GDPTR;

GDHANDLE* new_handle();

bool is_bound(GDHANDLE* handle);

bool bind_gd(GDHANDLE* handle);

bool is_game_running(GDHANDLE* handle);

bool read(GDHANDLE* handle, GDPTR* ptr, void* buffer, size_t size);

#endif //TEST_GDC_H
