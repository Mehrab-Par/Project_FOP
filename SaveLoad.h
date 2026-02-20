#pragma once
#include "GameState.h"
#include <string>

namespace SaveLoad {
    bool        saveProject     (const GameState& state, const std::string& filename);
    bool        loadProject     (GameState& state,       const std::string& filename);
    std::string getDefaultSavePath();
}
