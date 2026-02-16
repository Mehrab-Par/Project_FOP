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

    // PC / FETCHING

    Block* fetchNextBlock(GameState& state) {
    if (state.execCtx.pc < 0 || state.execCtx.pc >= (int)state.editorBlocks.size())
        return nullptr;
    Block* block = state.editorBlocks[state.execCtx.pc];
    state.execCtx.pc++;
    return block;
}

    void jumpTo(GameState& state, int targetLine) {
    state.execCtx.pc = targetLine;
}


    // MAIN DISPATCHER
void executeBlock(GameState& state, Block* block, float dt) {
    if (!block) return;

    // Reset watchdog on each real instruction
    state.execCtx.watchdogCounter = 0;

    switch (block->type) {
        // ── Motion ──
        case BlockType::Move:               executeMove(state, block);               break;
        case BlockType::TurnLeft:           executeTurnLeft(state, block);            break;
        case BlockType::TurnRight:          executeTurnRight(state, block);           break;
        case BlockType::GoTo:               executeGoTo(state, block);               break;
        case BlockType::ChangeX:            executeChangeX(state, block);            break;
        case BlockType::ChangeY:            executeChangeY(state, block);            break;
        case BlockType::SetX:               executeSetX(state, block);               break;
        case BlockType::SetY:               executeSetY(state, block);               break;
        case BlockType::PointInDirection:   executePointInDirection(state, block);   break;
        case BlockType::GoToRandom:         executeGoToRandom(state, block);         break;
        case BlockType::GoToMouse:          executeGoToMouse(state, block);          break;
        case BlockType::BounceOffEdge:      executeBounceOffEdge(state, block);      break;

        // ── Looks ──
        case BlockType::Say:                executeSay(state, block);                break;
        case BlockType::SayForTime:         executeSayForTime(state, block);         break;
        case BlockType::Think:              executeThink(state, block);              break;
        case BlockType::ThinkForTime:       executeThinkForTime(state, block);       break;
        case BlockType::SwitchCostume:      executeSwitchCostume(state, block);      break;
        case BlockType::NextCostume:        executeNextCostume(state, block);        break;
        case BlockType::SwitchBackdrop:     executeSwitchBackdrop(state, block);     break;
        case BlockType::NextBackdrop:       executeNextBackdrop(state, block);       break;
        case BlockType::ChangeSize:         executeChangeSize(state, block);         break;
        case BlockType::SetSize:            executeSetSize(state, block);            break;
        case BlockType::Show:               executeShow(state, block);               break;
        case BlockType::Hide:               executeHide(state, block);               break;
        case BlockType::GoToFrontLayer:     executeGoToFrontLayer(state, block);     break;
        case BlockType::GoToBackLayer:      executeGoToBackLayer(state, block);      break;
        case BlockType::MoveLayerForward:   executeMoveLayerForward(state, block);   break;
        case BlockType::MoveLayerBackward:  executeMoveLayerBackward(state, block);  break;
        case BlockType::ChangeGraphicEffect:executeChangeGraphicEffect(state, block);break;
        case BlockType::SetGraphicEffect:   executeSetGraphicEffect(state, block);   break;
        case BlockType::ClearGraphicEffects:executeClearGraphicEffects(state, block);break;

        // ── Sound ──
        case BlockType::PlaySound:          executePlaySound(state, block);          break;
        case BlockType::PlaySoundUntilDone: executePlaySoundUntilDone(state, block); break;
        case BlockType::StopAllSounds:      executeStopAllSounds(state, block);      break;
        case BlockType::SetVolume:          executeSetVolume(state, block);          break;
        case BlockType::ChangeVolume:       executeChangeVolume(state, block);       break;

        // ── Events ──
        case BlockType::WhenGreenFlagClicked:
        case BlockType::WhenKeyPressed:
        case BlockType::WhenSpriteClicked:
        case BlockType::WhenBroadcast:
            // Event hat blocks: just skip (they're entry points)
            break;
        case BlockType::Broadcast:          executeBroadcast(state, block);          break;

        // ── Control ──
        case BlockType::Wait:               executeWait(state, block, dt);           break;
        case BlockType::Repeat:             executeRepeat(state, block);             break;
        case BlockType::Forever:            executeForever(state, block);            break;
        case BlockType::IfThen:             executeIfThen(state, block);             break;
        case BlockType::IfThenElse:         executeIfThenElse(state, block);         break;
        case BlockType::WaitUntil:          executeWaitUntil(state, block);          break;
        case BlockType::RepeatUntil:        executeRepeatUntil(state, block);        break;
        case BlockType::StopAll:            executeStopAll(state, block);            break;

        // End markers — handled by their parent control blocks
        case BlockType::EndRepeat: {
            // Pop loop stack
            if (!state.execCtx.loopStack.empty()) {
                int count  = state.execCtx.loopStack.back();
                int target = state.execCtx.loopReturnStack.back();
                count--;
                if (count > 0) {
                    state.execCtx.loopStack.back() = count;
                    jumpTo(state, target + 1); // jump back to body start (after Repeat block)
                } else {
                    state.execCtx.loopStack.pop_back();
                    state.execCtx.loopReturnStack.pop_back();
                    // Continue past EndRepeat (pc already incremented)
                }
            }
            break;
        }
        case BlockType::EndForever: {
            // Unconditional jump back
            if (!state.execCtx.loopReturnStack.empty()) {
                int target = state.execCtx.loopReturnStack.back();
                jumpTo(state, target + 1);
            }
            break;
        }
        case BlockType::EndIf:
            // Just fall through
            break;
        case BlockType::Else:
            // If we reach Else while executing (True branch finished), jump to EndIf
            {
                // Find the parent IfThenElse — its jumpTarget is EndIf
                // We stored back-pointer: search backwards
                for (int i = state.execCtx.pc - 2; i >= 0; i--) {
                    Block* b = state.editorBlocks[i];
                    if (b && b->type == BlockType::IfThenElse && b->elseTarget == (state.execCtx.pc - 1)) {
                        jumpTo(state, b->jumpTarget + 1);
                        break;
                    }
                }
            }
            break;

        // ── Sensing ──
        case BlockType::Ask:                executeAsk(state, block);                break;

        // ── Variables ──
        case BlockType::SetVariable:        executeSetVariable(state, block);        break;
        case BlockType::ChangeVariable:     executeChangeVariable(state, block);     break;

        // ── Custom Functions ──
        case BlockType::DefineFunction:     executeDefineFunction(state, block);     break;
        case BlockType::CallFunction:       executeCallFunction(state, block);       break;

        // ── Pen ──
        case BlockType::PenDown:            executePenDown(state, block);            break;
        case BlockType::PenUp:              executePenUp(state, block);              break;
        case BlockType::Stamp:              executeStamp(state, block);              break;
        case BlockType::EraseAll:           executeEraseAll(state, block);           break;
        case BlockType::SetPenColor:        executeSetPenColor(state, block);        break;
        case BlockType::SetPenSize:         executeSetPenSize(state, block);         break;
        case BlockType::ChangePenSize:      executeChangePenSize(state, block);      break;

        // ── Timer reset ──
        case BlockType::ResetTimer:
            state.globalTimer = 0.0f;
            Logger::logInfo(state, block->sourceLine, "TIMER", "Reset", "Timer reset to 0");
            break;

        default:
            break;
    }
}


// MOTION BLOCKS


void executeMove(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;

    double steps = toDouble(evaluateExpression(state, block->params[0]));
    float oldX = sprite->x, oldY = sprite->y;

    // direction: 90=right, 0=up, 180=down, 270=left
    float rad = (sprite->direction - 90.0f) * DEG_TO_RAD;
    sprite->x += (float)(steps * std::cos(rad));
    sprite->y += (float)(steps * std::sin(rad));

    // Pen trail
    if (sprite->penDown) {
        PenStroke stroke;
        stroke.x1 = oldX; stroke.y1 = oldY;
        stroke.x2 = sprite->x; stroke.y2 = sprite->y;
        stroke.color = sprite->penColor;
        stroke.size  = sprite->penSize;
        state.penStrokes.push_back(stroke);
    }

    clampPosition(state, *sprite);

    Logger::logInfo(state, block->sourceLine, "MOVE", "Position",
                    "(" + std::to_string((int)oldX) + "," + std::to_string((int)oldY) + ") -> (" +
                    std::to_string((int)sprite->x) + "," + std::to_string((int)sprite->y) + ")");
}

void executeTurnLeft(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double degrees = toDouble(evaluateExpression(state, block->params[0]));
    float old = sprite->direction;
    sprite->direction = wrapAngle(sprite->direction - (float)degrees);
    Logger::logInfo(state, block->sourceLine, "TURN_LEFT", "Direction",
                    std::to_string((int)old) + " -> " + std::to_string((int)sprite->direction));
}

void executeTurnRight(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double degrees = toDouble(evaluateExpression(state, block->params[0]));
    float old = sprite->direction;
    sprite->direction = wrapAngle(sprite->direction + (float)degrees);
    Logger::logInfo(state, block->sourceLine, "TURN_RIGHT", "Direction",
                    std::to_string((int)old) + " -> " + std::to_string((int)sprite->direction));
}

void executeGoTo(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double newX = toDouble(evaluateExpression(state, block->params[0]));
    double newY = toDouble(evaluateExpression(state, block->params[1]));
    sprite->x = (float)newX;
    sprite->y = (float)newY;
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "GOTO", "Position",
                    "(" + std::to_string((int)sprite->x) + "," + std::to_string((int)sprite->y) + ")");
}

void executeChangeX(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    float oldX = sprite->x;
    sprite->x += (float)val;
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "CHANGE_X", "X",
                    std::to_string((int)oldX) + " -> " + std::to_string((int)sprite->x));
}

void executeChangeY(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    float oldY = sprite->y;
    sprite->y += (float)val;
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "CHANGE_Y", "Y",
                    std::to_string((int)oldY) + " -> " + std::to_string((int)sprite->y));
}

void executeSetX(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->x = (float)val;
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "SET_X", "X", std::to_string((int)sprite->x));
}

void executeSetY(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->y = (float)val;
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "SET_Y", "Y", std::to_string((int)sprite->y));
}

void executePointInDirection(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double angle = toDouble(evaluateExpression(state, block->params[0]));
    sprite->direction = wrapAngle((float)angle);
    Logger::logInfo(state, block->sourceLine, "POINT_DIR", "Direction", std::to_string((int)sprite->direction));
}

void executeGoToRandom(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->x = (float)(rand() % state.stageWidth);
    sprite->y = (float)(rand() % state.stageHeight);
    Logger::logInfo(state, block->sourceLine, "GOTO_RANDOM", "Position",
                    "(" + std::to_string((int)sprite->x) + "," + std::to_string((int)sprite->y) + ")");
}

void executeGoToMouse(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->x = (float)(state.mouseX - state.stageX);
    sprite->y = (float)(state.mouseY - state.stageY);
    clampPosition(state, *sprite);
    Logger::logInfo(state, block->sourceLine, "GOTO_MOUSE", "Position",
                    "(" + std::to_string((int)sprite->x) + "," + std::to_string((int)sprite->y) + ")");
}

void executeBounceOffEdge(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    bool bounced = false;
    if (sprite->x <= 0.0f || sprite->x >= (float)state.stageWidth) {
        sprite->direction = wrapAngle(180.0f - sprite->direction);
        bounced = true;
    }
    if (sprite->y <= 0.0f || sprite->y >= (float)state.stageHeight) {
        sprite->direction = wrapAngle(-sprite->direction);
        bounced = true;
    }
    clampPosition(state, *sprite);
    if (bounced)
        Logger::logInfo(state, block->sourceLine, "BOUNCE", "Direction", std::to_string((int)sprite->direction));
}



}