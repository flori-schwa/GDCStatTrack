//
// Created by Florian on 17.07.2020.
//
#include "include/gdc.h"

bool find_gd_proc(DWORD* ptrPid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(snapshot, &entry)) {
        return false;
    }

    do {
        if (strncmp(GD_PROC_NAME, entry.szExeFile, strlen(GD_PROC_NAME)) == 0) {
            *ptrPid = entry.th32ProcessID;
            return true;
        }
    } while (Process32Next(snapshot, &entry));

    return false;
}

bool find_modules(DWORD procId, int* ptrSizeTo, MODULEENTRY32** ptrModulesTo) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procId);

    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 entry;
    entry.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(snapshot, &entry)) {
        return false;
    }

    int count = 1;
    MODULEENTRY32* buf = (MODULEENTRY32*) malloc(sizeof(MODULEENTRY32) * count);

#define CPY(fName) buf[idx].fName = entry.fName
    do {
        int idx = count - 1;

        CPY(dwSize);
        CPY(th32ModuleID);
        CPY(th32ProcessID);
        CPY(GlblcntUsage);
        CPY(ProccntUsage);
        CPY(modBaseAddr);
        CPY(modBaseSize);
        CPY(hModule);

        memcpy(
                buf[idx].szModule,
                entry.szModule,
                strlen(entry.szModule) + 1 // +1 for \0 byte
        );

        memcpy(
                buf[idx].szExePath,
                entry.szExePath,
                strlen(entry.szExePath) + 1 // +1 for \0 byte
        );

        buf = realloc(buf, (sizeof(MODULEENTRY32) * (++count)));
    } while (Module32Next(snapshot, &entry));
#undef CPY

    *ptrModulesTo = buf;
    *ptrSizeTo = count;

    return true;
}

bool get_handle(DWORD procId, HANDLE* handle) {
    if (handle) {
        CloseHandle(*handle);
    }

    *handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

    return *handle != NULL;
}

void* get_module_base_address(GDHANDLE* handle, const char* modName) {
    for (int i = 0; i < handle->gd_NModules; i++) {
        if (strncmp(modName, strrchr(handle->gd_Modules[i].szExePath, '\\') + 1, strlen(modName)) == 0) {
            return handle->gd_Modules[i].modBaseAddr;
        }
    }

    return NULL;
}

void* get_ptr_address(GDHANDLE* handle, GDPTR* ptr) {
    if (handle->gd_BaseHandle == NULL) {
        return NULL; // Not bound
    }

    void* mod_base_addr = get_module_base_address(handle, ptr->baseModuleName);

    if (mod_base_addr == NULL) {
        return NULL;
    }

    size_t addr = (size_t) mod_base_addr;

    if (ptr->n_offsets > 0) {
        addr += ptr->offsets[0];
    }

    for (int i = 1; i < ptr->n_offsets; i++) {
        size_t buffer = 0;

        ReadProcessMemory(
                handle->gd_BaseHandle,
                (void*) addr,
                &buffer,
                sizeof(size_t),
                NULL
        );

        addr = buffer + ptr->offsets[i];
    }

    return (void*) addr;
}

GDHANDLE* new_handle() {
    GDHANDLE* handle = (GDHANDLE*) malloc(sizeof(GDHANDLE));

    handle->gd_BaseHandle = NULL;
    handle->gd_BaseAddr = NULL;
    handle->gd_Modules = NULL;
    handle->gd_NModules = -1;
    handle->gd_ProcId = -1;

    return handle;
}

bool is_bound(GDHANDLE* handle) {
    return handle->gd_BaseHandle != NULL;
}

bool bind_gd(GDHANDLE* handle) {
    if (handle == NULL) {
        return false;
    }

    if (!find_gd_proc(&(handle->gd_ProcId))) {
        return false;
    }

    if (!get_handle(handle->gd_ProcId, &handle->gd_BaseHandle)) {
        return false;
    }

    if (!find_modules(handle->gd_ProcId, &(handle->gd_NModules), &(handle->gd_Modules))) {
        return false;
    }

    for (int i = 0; i < handle->gd_NModules; i++) {
        if (strncmp(GD_PROC_NAME, handle->gd_Modules[i].szExePath, strlen(GD_PROC_NAME)) == 0) {
            handle->gd_BaseAddr = (void*) handle->gd_Modules[i].modBaseAddr;
            handle->gd_BaseHandle = handle->gd_Modules[i].hModule;
            break;
        }
    }

    return true;
}

bool is_game_running(GDHANDLE* handle) {
    if (handle == NULL || handle->gd_BaseHandle == NULL) {
        return false;
    }

    DWORD exitCode = 0;

    if (!GetExitCodeProcess(handle->gd_BaseHandle, &exitCode)) {
        return false;
    }

    return exitCode == STILL_ACTIVE;
}

bool read(GDHANDLE* handle, GDPTR* ptr, void* buffer, size_t size) {
    void* addr = get_ptr_address(handle, ptr);

    if (addr == NULL) {
        return false;
    }

    ReadProcessMemory(
            handle->gd_BaseHandle,
            addr,
            buffer,
            size,
            NULL
    );

    return true;
}
