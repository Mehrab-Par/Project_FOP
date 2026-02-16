#pragma once
#include "GameState.h"
#include <string>
#include <vector>
            //================================================//
                        //  Execution Engine    //
            //================================================//
namespace Engine {
    // Core lifecycle
    void engineInit(GameState& state);
    void engineUpdate(GameState& state, float dt);
    void engineReset(GameState& state);
    void startExecution(GameState& state);
    void stopExecution(GameState& state);
    void pauseExecution(GameState& state);
    void resumeExecution(GameState& state);
    // Pre-processing
    void preProcessBlocks(GameState& state, std::vector<Block*>& blocks);
}