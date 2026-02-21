#pragma once
#include "GameState.h"

namespace Engine {
    void update(GameState& state, float deltaTime);
    void startExecution(GameState& state);
    void runScripts(GameState& state, float deltaTime);
    bool executeOneBlock(GameState& gs, Sprite* sp, SpriteExecCtx& ctx,
                         const std::vector<Block*>& script);
    void preScan(GameState& gs);
}
