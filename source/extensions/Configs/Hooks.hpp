#pragma once
#include <string>
#include <vector>
#include <Utility.h>
#include "app_debug.h"

#include "extensions/Configuration.hpp"

//! Which reversed functions run instead of the game's own code.
//! In minimal mode every hook that isn't locked is switched back to the original code,
//! except the categories/functions listed in `KeepHooked` (our modified code).
inline struct HooksConfig {
    INI_CONFIG_SECTION("Hooks");

    bool                     MinimalMode{};
    std::vector<std::string> KeepHooked{};

    void Load() {
        STORE_INI_CONFIG_VALUE(MinimalMode, false);

        KeepHooked.clear();
        const auto list = GET_INI_CONFIG_VALUE("KeepHooked", std::string{});
        for (auto item : SplitStringView(list, ",")) {
            item.remove_prefix(std::min(item.find_first_not_of(" \t"), item.size()));
            item.remove_suffix(item.size() - std::min(item.find_last_not_of(" \t") + 1, item.size()));
            if (!item.empty()) {
                KeepHooked.emplace_back(item);
            }
        }
    }
} g_HooksConfig{};
