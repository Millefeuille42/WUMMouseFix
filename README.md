# Worms Ultimate Mayhem (WormsXHD) - Smooth FPS & Mouse Fix

A `dinput8.dll` wrapper that fixes the FPS cap and mouse jitter/stuttering issues in **Worms Ultimate Mayhem** on modern High-Refresh Rate monitors.

## Credits & Inspiration
This project integrates and refines fixes discovered by the community:

*   **FPS Limit Fix**: Derived from **[heatray/WUMPatch](https://github.com/heatray/WUMPatch)**.
*   **Mouse Stutter Fix**: Derived from **[ermaccer/Worms4UHD.MouseFix](https://github.com/ermaccer/Worms4UHD.MouseFix)**.

For transparency, I'd like to specify that parts of this fix has been made with the help of generative LLMs, 
the idea and part of the implementation of the stair-stepping fix being the major part.

## Features
*   **Higher FPS Support**: Removes the hardcoded FPS cap (sets interval to 1ms for up to 1000 FPS).
*   **Mouse Accumulator Fix**: Implements sub-pixel mouse movement accumulation to limit "stair-stepping" or "snapping" at high framerates.
*   **Menu/Cursor Safety**: Respects game menus to prevent the cursor from escaping or breaking Alt-Tab behavior.
*   **Timer Resolution Fix**: Enforces 1ms Windows Timer Resolution for stable frame pacing.

## Installation
### Method 1: Standalone (Recommended)
1.  Place `dinput8.dll` into your game folder (next to `WormsMayhem.exe`).
2.  Launch the game normally.

### Method 2: Ultimate ASI Loader (For WUMPatch Compatibility)
If you want to use this with **WUMPatch**:
1.  Download **[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)**.
2.  Rename Ultimate ASI Loader to `dinput8.dll` and place it in the game folder.
    *   *Note: If WUMPatch is already installed dinput8.dll, use the provided ASI Loader instead. I cannot guarantee a full compatibility with WUMPatch.*
3.  Place `WUMMouseFix.asi` (this fix) into the root or the `plugins/` folder.

## How it Works
The fix acts as a proxy for `dinput8.dll`, allowing it to load automatically with the game. It then patches the game engine memory in real-time:
*   **FPS Patch**: Changes the frame update interval to 1ms (Credits: *WUMPatch*).
*   **Mouse Hook**: Intercepts the engine's mouse handler to accumulate fractional movement deltas, solving the quantization error caused by high frame rates (Credits: *Worms4UHD.MouseFix*).

## Build Instructions
Requirements:
*   MinGW (GCC) or compatible C++ compiler.

Build manaully via GCC as dinput8.dll:
```bash
g++ -m32 -shared -o dinput8.dll dllmain.cpp mod.def -luser32 -lwinmm
```

Build manaully via GCC as WUMMouseFix.asi:
```bash
g++ -m32 -shared -o WUMMouseFix.asi dllmain.cpp mod.def -luser32 -lwinmm
```

## License
MIT License. See [LICENSE](LICENSE) for details.
