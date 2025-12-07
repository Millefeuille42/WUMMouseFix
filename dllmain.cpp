#include <windows.h>
#include <stdio.h>
#include <string>

// Link against winmm.lib for timeBeginPeriod/timeEndPeriod
#pragma comment(lib, "winmm.lib") 

// --- Constants ---
// FPS Limit Patch Addresses (Credits: WUMPatch)
const DWORD ADDR_FPS_INTERVAL_1 = 0x4D919B;
const DWORD ADDR_FPS_INTERVAL_2 = 0x4D919F;

// Mouse Fix Addresses (Credits: Worms4UHD.MouseFix)
const DWORD ADDR_CURSOR_VISIBLE = 0x97AC96;
const DWORD ADDR_MOUSE_HANDLER  = 0x701D92;
const DWORD ADDR_MOUSE_VTABLE   = 0x89AE5C;

// Config
// 1ms = ~1000 FPS Cap. Setting to 0 (Uncapped) can cause physics issues.
// Stock game uses 33ms (30 FPS) or 16ms (60 FPS).
const BYTE TARGET_FRAME_INTERVAL = 1; 

// --- Proxy Definition ---
// Standard dinput8.dll proxy to ensure the game loads DLL automatically.
typedef HRESULT(WINAPI* pDirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
pDirectInput8Create OriginalDirectInput8Create = nullptr;
HMODULE hOriginalDinput = nullptr;

void SetupProxy() {
    char path[MAX_PATH];
    GetSystemDirectory(path, MAX_PATH);
    strcat_s(path, "\\dinput8.dll");
    hOriginalDinput = LoadLibrary(path);
    if (hOriginalDinput) {
        OriginalDirectInput8Create = (pDirectInput8Create)GetProcAddress(hOriginalDinput, "DirectInput8Create");
    }
}

extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    if (!OriginalDirectInput8Create) SetupProxy();
    if (OriginalDirectInput8Create) return OriginalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    return E_FAIL;
}

// --- Memory Utils ---
void PatchByte(DWORD address, BYTE value) {
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)address = value;
    VirtualProtect((LPVOID)address, 1, oldProtect, &oldProtect);
}

void PatchPointer(DWORD address, void* value) {
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)address = (DWORD)value;
    VirtualProtect((LPVOID)address, 4, oldProtect, &oldProtect);
}

// --- Fix Implementations ---

// Mouse Accumulator State
// stores sub-pixel movement that would otherwise be discarded by integer truncation.
float AccX = 0.0f;
float AccY = 0.0f;

// Replacement Mouse Handler
// Fixes "stair-stepping" quantization at high FPS by accumulating sub-pixel deltas.
void WINAPI SetMousePosDirectly(int ptr, int X, int Y) {
    // 1. Check if cursor is visible (Menus, Alt-Tab, etc).
    // If cursor is hidden, don't skip this, the cursor will get trapped or input will glitch in menus.
    bool* pCursorVisible = (bool*)ADDR_CURSOR_VISIBLE;
    if (pCursorVisible && *pCursorVisible) {
        return; 
    }

    // 2. Accumulate fractional movement
    // High FPS means smaller X/Y deltas per frame.
    // The engine likely truncates these to integers, losing precision (stair-stepping).
    // Accumulate them until they form a whole pixel.
    AccX += (float)X;
    AccY += (float)Y;

    int SendX = (int)AccX;
    int SendY = (int)AccY;

    // Remove the part sent from the accumulator, keeping the fraction.
    AccX -= SendX;
    AccY -= SendY;
    
    // 3. Forward to original game handler
    typedef void(__stdcall* pOriginalFunc)(int, int, int);
    ((pOriginalFunc)ADDR_MOUSE_HANDLER)(ptr, SendX, SendY);
}

void ApplyPatches() {
    // 1. FPS Limit Patch
    // Forces the game to update inputs/rendering more frequently.
    PatchByte(ADDR_FPS_INTERVAL_1, TARGET_FRAME_INTERVAL);
    PatchByte(ADDR_FPS_INTERVAL_2, TARGET_FRAME_INTERVAL);

    // 2. Mouse Hook
    // Redirects the mouse position handler to the Accumulator version.
    PatchPointer(ADDR_MOUSE_VTABLE, (void*)&SetMousePosDirectly);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        
        // Critical: Force Windows Timer Resolution to 1ms.
        // Without this, Sleep(1) can sleep for 15ms, causing massive jitter/shakiness at high FPS.
        timeBeginPeriod(1); 
        
        SetupProxy();
        ApplyPatches();
        break;
    case DLL_PROCESS_DETACH:
        timeEndPeriod(1);
        if (hOriginalDinput) FreeLibrary(hOriginalDinput);
        break;
    }
    return TRUE;
}
