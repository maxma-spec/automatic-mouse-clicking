# automatic-mouse-clicking

Windows 10/11 x64 C++ mouse and keyboard auto-clicker.

[Download the latest Windows x64 ZIP](https://github.com/maxma-spec/automatic-mouse-clicking/releases/latest/download/Automatic-mouse-clicking-win-x64.zip)

## Controls

- F8: wait 3 seconds, then start; press again to pause
- F7: change the base interval
- F6: choose the mouse or keyboard target
- Esc: exit; cancel while entering settings

## Features

- Random interval jitter from -3 ms to +5 ms, clamped to at least 1 ms
- Mouse left, right and middle buttons
- Letters, digits, Space, Shift, Ctrl, Alt, Tab, Enter, arrow keys and supported F keys
- Scan-code keyboard input with extended-key handling
- C++ standard monotonic timing without changing the Windows global timer period

The program uses global F6, F7, F8 and Esc key-state checks. After starting or resuming, use the 3-second countdown to switch to the window you want to click.

The release build dynamically links the Microsoft Visual C++ x64 runtime. If Windows reports missing runtime DLLs, install the [official Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).

Source files are provided for learning and inspection. The downloadable ZIP contains only the executable and usage notes.
