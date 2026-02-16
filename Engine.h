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
    // ── PC / Fetching ──
    Block* fetchNextBlock(GameState& state);
    void jumpTo(GameState& state, int targetLine);
    // ── Main Dispatcher ──
    void executeBlock(GameState& state, Block* block, float dt);
    // ── Motion ──
    void executeMove(GameState& state, Block* block);
    void executeTurnLeft(GameState& state, Block* block);
    void executeTurnRight(GameState& state, Block* block);
    void executeGoTo(GameState& state, Block* block);
    void executeChangeX(GameState& state, Block* block);
    void executeChangeY(GameState& state, Block* block);
    void executeSetX(GameState& state, Block* block);
    void executeSetY(GameState& state, Block* block);
    void executePointInDirection(GameState& state, Block* block);
    void executeGoToRandom(GameState& state, Block* block);
    void executeGoToMouse(GameState& state, Block* block);
    void executeBounceOffEdge(GameState& state, Block* block);



}