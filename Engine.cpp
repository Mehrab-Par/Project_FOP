#include "Engine.h"
#include "Logger.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────
static Sprite* getActiveSprite(GameState& state) {
    if (state.activeSpriteIndex >= 0 &&
        state.activeSpriteIndex < (int)state.sprites.size())
        return state.sprites[state.activeSpriteIndex];
    return nullptr;
}

static const float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

static float wrapAngle(float angle) {
    while (angle < 0.0f)   angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

namespace Engine {
    // CORE LIFECYCLE

void engineInit(GameState& state) {
    state.execCtx.pc          = 0;
    state.execCtx.isRunning   = false;
    state.execCtx.isPaused    = false;
    state.execCtx.isDebugMode = false;
    state.execCtx.loopStack.clear();
    state.execCtx.loopReturnStack.clear();
    state.execCtx.callStack.clear();
    state.execCtx.watchdogCounter = 0;
    state.globalCycle = 0;
    Logger::logInfo(state, 0, "ENGINE", "Init", "Engine initialized");
}

void startExecution(GameState& state) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite || sprite->script.empty()) {
        Logger::logWarning(state, 0, "ENGINE", "Start", "No script to run");
        return;
    }
    // Flatten script into editorBlocks for indexed execution
    state.editorBlocks = sprite->script;
    preProcessBlocks(state, state.editorBlocks);

    state.execCtx.pc        = 0;
    state.execCtx.isRunning = true;
    state.execCtx.isPaused  = false;
    state.execCtx.loopStack.clear();
    state.execCtx.loopReturnStack.clear();
    state.execCtx.callStack.clear();
    state.execCtx.watchdogCounter = 0;
    Logger::clearLogs(state);
    Logger::logInfo(state, 0, "ENGINE", "Start", "Execution started");
}

void stopExecution(GameState& state) {
    state.execCtx.isRunning = false;
    state.execCtx.pc        = 0;
    state.execCtx.loopStack.clear();
    state.execCtx.loopReturnStack.clear();
    state.execCtx.callStack.clear();
    Logger::logInfo(state, state.execCtx.pc, "ENGINE", "Stop", "Execution stopped");
}

void pauseExecution(GameState& state) {
    state.execCtx.isPaused = true;
    Logger::logInfo(state, state.execCtx.pc, "ENGINE", "Pause", "Execution paused at PC=" + std::to_string(state.execCtx.pc));
}

void resumeExecution(GameState& state) {
    state.execCtx.isPaused = false;
    Logger::logInfo(state, state.execCtx.pc, "ENGINE", "Resume", "Execution resumed from PC=" + std::to_string(state.execCtx.pc));
}

void engineReset(GameState& state) {
    stopExecution(state);
    // Reset all sprites to initial state
    for (auto* sprite : state.sprites) {
        sprite->x              = sprite->initX;
        sprite->y              = sprite->initY;
        sprite->direction      = sprite->initDirection;
        sprite->size           = sprite->initSize;
        sprite->currentCostume = sprite->initCostume;
        sprite->visible        = true;
        sprite->speechText     = "";
        sprite->speechTimer    = -1.0f;
    }
    state.penStrokes.clear();
    state.penStamps.clear();
    state.globalTimer = 0.0f;
    Logger::logInfo(state, 0, "ENGINE", "Reset", "Full reset complete");
}

void engineUpdate(GameState& state, float dt) {
    if (!state.execCtx.isRunning || state.execCtx.isPaused) return;

    state.globalCycle++;
    state.globalTimer += dt;

    // Debug step mode
    checkDebugMode(state);
    if (state.execCtx.waitingForStep) return;

    // Wait timer handling
    if (state.execCtx.waitTimer > 0.0f) {
        state.execCtx.waitTimer -= dt;
        if (state.execCtx.waitTimer <= 0.0f) {
            state.execCtx.waitTimer = 0.0f;
            state.execCtx.pc++;
        }
        return;
    }

    // WaitUntil handling
    if (state.execCtx.waitingUntil) {
        if (state.execCtx.pc < (int)state.editorBlocks.size()) {
            Block* block = state.editorBlocks[state.execCtx.pc];
            if (block && !block->params.empty()) {
                bool cond = evaluateCondition(state, block->params[0]);
                if (cond) {
                    state.execCtx.waitingUntil = false;
                    state.execCtx.pc++;
                }
            }
        }
        return;
    }

    // Waiting for user answer
    if (state.waitingForAnswer) return;

    // Bounds check
    if (state.execCtx.pc >= (int)state.editorBlocks.size()) {
        stopExecution(state);
        return;
    }

    // Watchdog
    state.execCtx.watchdogCounter++;
    if (watchdogCheck(state)) {
        Logger::logError(state, state.execCtx.pc, "WATCHDOG",
                         "InfiniteLoop", "Infinite loop detected! Stopping.");
        stopExecution(state);
        return;
    }

    // Fetch & execute
    Block* block = fetchNextBlock(state);
    if (block) {
        executeBlock(state, block, dt);
    } else {
        stopExecution(state);
    }
}


    // PRE-PROCESSING
    void preProcessBlocks(GameState& state, std::vector<Block*>& blocks) {
        // Stack-based matching of control structures
        std::vector<int> stack; // indices of unmatched starts

        for (int i = 0; i < (int)blocks.size(); i++) {
            Block* b = blocks[i];
            if (!b) continue;

            switch (b->type) {
                case BlockType::IfThen:
                case BlockType::IfThenElse:
                case BlockType::Repeat:
                case BlockType::RepeatUntil:
                case BlockType::Forever:
                    stack.push_back(i);
                    break;

                case BlockType::Else:
                    // Find matching IfThenElse
                    if (!stack.empty()) {
                        int startIdx = stack.back();
                        blocks[startIdx]->elseTarget = i;
                    }
                    break;

                case BlockType::EndIf:
                    if (!stack.empty()) {
                        int startIdx = stack.back();
                        stack.pop_back();
                        blocks[startIdx]->jumpTarget = i;
                        b->jumpTarget = startIdx; // back-pointer
                    }
                    break;

                case BlockType::EndRepeat:
                    if (!stack.empty()) {
                        int startIdx = stack.back();
                        stack.pop_back();
                        blocks[startIdx]->jumpTarget = i;
                        b->jumpTarget = startIdx; // jump back target
                    }
                    break;

                case BlockType::EndForever:
                    if (!stack.empty()) {
                        int startIdx = stack.back();
                        stack.pop_back();
                        blocks[startIdx]->jumpTarget = i;
                        b->jumpTarget = startIdx;
                    }
                    break;

                default:
                    break;
            }
        }
        Logger::logInfo(state, 0, "PREPROCESS", "Done",
                        "Pre-processed " + std::to_string(blocks.size()) + " blocks");
    }








}