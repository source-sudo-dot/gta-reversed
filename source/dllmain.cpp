#include "StdInc.h"
#include "config.h"
#include <cstdlib>

#include "WinPlatform.h"
#include "extensions/CommandLine.h"
#include "extensions/debug.hpp"
#include "extensions/Configuration.hpp"
#include "reversiblehooks/RootHookCategory.h"

void InjectHooksMain(HMODULE hThisDLL);

static constexpr auto DEFAULT_INI_FILENAME = "gta-reversed.ini";

#include "extensions/Configs/FastLoader.hpp"
#include "extensions/Configs/Miscellaneous.hpp"
#include "extensions/Configs/Hooks.hpp"

void LoadConfigurations(HMODULE hThisDLL) {
    // Firstly load the INI into the memory.
    // It ships in `scripts\` next to the ASI, while the working directory is the game folder.
    char dllPath[MAX_PATH]{};
    GetModuleFileNameA(hThisDLL, dllPath, MAX_PATH);
    const auto iniNextToDLL = fs::path{ dllPath }.parent_path() / DEFAULT_INI_FILENAME;
    g_ConfigurationMgr.Load(fs::exists(iniNextToDLL) ? iniNextToDLL.string() : DEFAULT_INI_FILENAME);

    // Then load all specific configurations.
    g_FastLoaderConfig.Load();
    g_MiscConfig.Load();
    g_HooksConfig.Load();
    // ...
}

static void ApplyCommandLineHookSettings() {
    using namespace ReversibleHooks;

    const auto ResultText = [](SetCatOrItemStateResult res) {
        switch (res) {
        case SetCatOrItemStateResult::NotFound: return "not found";
        case SetCatOrItemStateResult::Locked:   return "locked";
        case SetCatOrItemStateResult::Done:     return "done";
        default:                                NOTSA_UNREACHABLE();
        }
    };

    if (CommandLine::s_UnhookAll || !CommandLine::s_UnhookExcept.empty()) {
        GetRootCategory().SetAllItemsEnabled(false);

        NOTSA_LOG_DEBUG("Unhooked all via command-line");
        for (const auto& item : CommandLine::s_UnhookExcept) {
            const auto res = SetCategoryOrItemStateByPath(item, true);

            if (res == SetCatOrItemStateResult::Done) {
                NOTSA_LOG_DEBUG("Rehooked '{}' via command-line.", item);
            } else {
                NOTSA_LOG_WARN("Couldn't rehook '{}' via command-line: {}", item, ResultText(res));
            }
        }
        return;
    }

    if (g_HooksConfig.MinimalMode) {
        GetRootCategory().SetAllItemsEnabled(false);

        NOTSA_LOG_INFO("Minimal mode: every unlocked hook runs the original code");
        for (const auto& item : g_HooksConfig.KeepHooked) {
            const auto res = SetCategoryOrItemStateByPath(item, true);

            if (res == SetCatOrItemStateResult::Done) {
                NOTSA_LOG_INFO("Minimal mode: kept '{}' hooked.", item);
            } else {
                NOTSA_LOG_WARN("Minimal mode: couldn't keep '{}' hooked: {}", item, ResultText(res));
            }
        }
        return;
    }

    if (!CommandLine::s_UnhookSome.empty()) {
        for (const auto& item : CommandLine::s_UnhookSome) {
            const auto res = SetCategoryOrItemStateByPath(item, false);

            if (res == SetCatOrItemStateResult::Done) {
                NOTSA_LOG_DEBUG("Unhooked '{}' via command-line.", item);
            } else {
                NOTSA_LOG_WARN("Couldn't unhook '{}' via command-line: {}", item, ResultText(res));
            }
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        // Fail if RenderWare has already been started
        if (*(RwCamera**)0xC1703C) {
            MessageBox(NULL, "gta_reversed failed to load (RenderWare has already been started)", "Error", MB_ICONERROR | MB_OK);
            return FALSE;
        }

        std::setlocale(LC_ALL, "en_US.UTF-8");

        notsa::debug::DisplayConsole();
        CommandLine::Load(__argc, __argv);
        if (CommandLine::s_WaitForDebugger) {
            notsa::debug::WaitForDebugger();
        }

        LoadConfigurations(hModule);

        InjectHooksMain(hModule);
        ApplyCommandLineHookSettings();
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
