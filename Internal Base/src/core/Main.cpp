#include <Windows.h>
#include <thread>
#include <cstdio>
#include "Hooks.h"
#include "ResourceExtractor.h"
#include "../../src/sdk/entity/EntityManager.h"
#include "../../src/feature/combat/legitbot/Aimbot.h"
#include "../../src/feature/combat/legitbot/Triggerbot.h"
#include "../../src/feature/combat/legitbot/RCS.h"
#include "../../src/feature/skinchanger/SkinChanger.h"
#include "../../src/sdk/interfaces/Interfaces.h"
#include "../../src/sdk/utils/Config.h"

static HANDLE g_MainThread = nullptr;

static void InitDebugConsole()
{
    if (!AllocConsole())
        return;

    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    SetConsoleTitleA("GhostWeave Internal Base");

    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd)
    {
        LONG exStyle = GetWindowLong(consoleWnd, GWL_EXSTYLE);
        SetWindowLong(consoleWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        SetLayeredWindowAttributes(consoleWnd, 0, 220, LWA_ALPHA); // semi-transparent
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut && hOut != INVALID_HANDLE_VALUE)
    {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    std::printf("\x1b[38;2;58;129;246m[GhostWeave]\x1b[0m \x1b[38;2;180;220;255mConsole attached.\x1b[0m Press END to unload DLL.\n");
}

DWORD WINAPI MainThread(LPVOID module)
{

    __try
    {
        ResourceExtractor::ExtractAll(static_cast<HMODULE>(module));
        Config::Refresh();
        Config::LoadFavorite();
        SkinChanger::Setup(); 
        Hooks::Setup();

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    while (!(GetAsyncKeyState(VK_END) & 1))
    {
        __try {
            EntityManager::Get().Update();
            Aimbot::Run();
            Triggerbot::Run();
            RCS::Run();
            SkinChanger::Run();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    __try
    {
        Hooks::Destroy();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    std::printf("\x1b[38;2;58;129;246m[GhostWeave]\x1b[0m \x1b[38;2;255;180;120mUnloading: END key pressed.\x1b[0m\n");
    Sleep(200);

    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        g_MainThread = CreateThread(
            nullptr,
            0,
            MainThread,
            hModule,
            0,
            nullptr
        );
    }

    return TRUE;
}
