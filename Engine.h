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
    // ── Looks ──
    void executeSay(GameState& state, Block* block);
    void executeSayForTime(GameState& state, Block* block);
    void executeThink(GameState& state, Block* block);
    void executeThinkForTime(GameState& state, Block* block);
    void executeSwitchCostume(GameState& state, Block* block);
    void executeNextCostume(GameState& state, Block* block);
    void executeSwitchBackdrop(GameState& state, Block* block);
    void executeNextBackdrop(GameState& state, Block* block);
    void executeChangeSize(GameState& state, Block* block);
    void executeSetSize(GameState& state, Block* block);
    void executeShow(GameState& state, Block* block);
    void executeHide(GameState& state, Block* block);
    void executeGoToFrontLayer(GameState& state, Block* block);
    void executeGoToBackLayer(GameState& state, Block* block);
    void executeMoveLayerForward(GameState& state, Block* block);
    void executeMoveLayerBackward(GameState& state, Block* block);
    void executeChangeGraphicEffect(GameState& state, Block* block);
    void executeSetGraphicEffect(GameState& state, Block* block);
    void executeClearGraphicEffects(GameState& state, Block* block);
    // ── Sound ──
    void executePlaySound(GameState& state, Block* block);
    void executePlaySoundUntilDone(GameState& state, Block* block);
    void executeStopAllSounds(GameState& state, Block* block);
    void executeSetVolume(GameState& state, Block* block);
    void executeChangeVolume(GameState& state, Block* block);

    // ── Events ──
    void executeBroadcast(GameState& state, Block* block);
    // ── Control Flow ──
    void executeWait(GameState& state, Block* block, float dt);
    void executeRepeat(GameState& state, Block* block);
    void executeForever(GameState& state, Block* block);
    void executeIfThen(GameState& state, Block* block);
    void executeIfThenElse(GameState& state, Block* block);
    void executeWaitUntil(GameState& state, Block* block);
    void executeRepeatUntil(GameState& state, Block* block);
    void executeStopAll(GameState& state, Block* block);
    // ── Sensing ──
    bool isTouchingMouse(GameState& state);
    bool isTouchingEdge(GameState& state);
    float distanceToMouse(GameState& state);
    float distanceToSprite(GameState& state, const std::string& spriteName);
    bool isKeyPressed(GameState& state, const std::string& key);
    void executeAsk(GameState& state, Block* block);


}