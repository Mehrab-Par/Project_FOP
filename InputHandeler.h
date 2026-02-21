#pragma once
#include "GameState.h"
#include <algorithm>

namespace Input {
    void handleEvent(GameState& state, SDL_Event& event);
    void handleMouseDown  (GameState& state, int x, int y);
    void handleMouseUp    (GameState& state, int x, int y);
    void handleMouseMotion(GameState& state, int x, int y);
    void handleKeyPress   (GameState& state, SDL_Keycode key);

    Block* findBlockAt   (const std::vector<Block*>& blocks, int x, int y);
    Block* findSnapTarget(GameState& state);
}
