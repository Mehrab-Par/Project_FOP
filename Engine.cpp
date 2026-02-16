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


// LOOKS BLOCKS
void executeSay(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string text = toString(evaluateExpression(state, block->params[0]));
    sprite->speechText = text;
    sprite->isThinker  = false;
    sprite->speechTimer = -1.0f; // permanent
    Logger::logInfo(state, block->sourceLine, "SAY", "Text", text);
}

void executeSayForTime(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string text = toString(evaluateExpression(state, block->params[0]));
    double secs      = toDouble(evaluateExpression(state, block->params[1]));
    sprite->speechText  = text;
    sprite->isThinker   = false;
    sprite->speechTimer = (float)secs;
    Logger::logInfo(state, block->sourceLine, "SAY_FOR", "Text", text + " for " + std::to_string((int)secs) + "s");
}

void executeThink(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string text = toString(evaluateExpression(state, block->params[0]));
    sprite->speechText = text;
    sprite->isThinker  = true;
    sprite->speechTimer = -1.0f;
    Logger::logInfo(state, block->sourceLine, "THINK", "Text", text);
}

void executeThinkForTime(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string text = toString(evaluateExpression(state, block->params[0]));
    double secs      = toDouble(evaluateExpression(state, block->params[1]));
    sprite->speechText  = text;
    sprite->isThinker   = true;
    sprite->speechTimer = (float)secs;
    Logger::logInfo(state, block->sourceLine, "THINK_FOR", "Text", text + " for " + std::to_string((int)secs) + "s");
}

void executeSwitchCostume(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string name = toString(evaluateExpression(state, block->params[0]));
    for (int i = 0; i < (int)sprite->costumes.size(); i++) {
        if (sprite->costumes[i].name == name) {
            sprite->currentCostume = i;
            Logger::logInfo(state, block->sourceLine, "COSTUME", "Switch", name);
            return;
        }
    }
    Logger::logWarning(state, block->sourceLine, "COSTUME", "Switch", "Costume not found: " + name);
}

void executeNextCostume(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite || sprite->costumes.empty()) return;
    sprite->currentCostume = (sprite->currentCostume + 1) % (int)sprite->costumes.size();
    Logger::logInfo(state, block->sourceLine, "COSTUME", "Next", std::to_string(sprite->currentCostume));
}

void executeSwitchBackdrop(GameState& state, Block* block) {
    std::string name = toString(evaluateExpression(state, block->params[0]));
    for (int i = 0; i < (int)state.backdrops.size(); i++) {
        if (state.backdrops[i].name == name) {
            state.currentBackdrop = i;
            Logger::logInfo(state, block->sourceLine, "BACKDROP", "Switch", name);
            return;
        }
    }
    Logger::logWarning(state, block->sourceLine, "BACKDROP", "Switch", "Backdrop not found: " + name);
}

void executeNextBackdrop(GameState& state, Block* block) {
    if (state.backdrops.empty()) return;
    state.currentBackdrop = (state.currentBackdrop + 1) % (int)state.backdrops.size();
    Logger::logInfo(state, block->sourceLine, "BACKDROP", "Next", std::to_string(state.currentBackdrop));
}

void executeChangeSize(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    float oldSize = sprite->size;
    sprite->size = std::max(1.0f, sprite->size + (float)val);
    Logger::logInfo(state, block->sourceLine, "SIZE", "Change",
                    std::to_string((int)oldSize) + "% -> " + std::to_string((int)sprite->size) + "%");
}

void executeSetSize(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->size = std::max(1.0f, (float)val);
    Logger::logInfo(state, block->sourceLine, "SIZE", "Set", std::to_string((int)sprite->size) + "%");
}

void executeShow(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->visible = true;
    Logger::logInfo(state, block->sourceLine, "SHOW", "Visible", "true");
}

void executeHide(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->visible = false;
    Logger::logInfo(state, block->sourceLine, "HIDE", "Visible", "false");
}

void executeGoToFrontLayer(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    int maxLayer = 0;
    for (auto* s : state.sprites) maxLayer = std::max(maxLayer, s->layer);
    sprite->layer = maxLayer + 1;
    Logger::logInfo(state, block->sourceLine, "LAYER", "Front", std::to_string(sprite->layer));
}

void executeGoToBackLayer(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    int minLayer = 0;
    for (auto* s : state.sprites) minLayer = std::min(minLayer, s->layer);
    sprite->layer = minLayer - 1;
    Logger::logInfo(state, block->sourceLine, "LAYER", "Back", std::to_string(sprite->layer));
}

void executeMoveLayerForward(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->layer += (int)val;
    Logger::logInfo(state, block->sourceLine, "LAYER", "Forward", std::to_string(sprite->layer));
}

void executeMoveLayerBackward(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->layer -= (int)val;
    Logger::logInfo(state, block->sourceLine, "LAYER", "Backward", std::to_string(sprite->layer));
}

void executeChangeGraphicEffect(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->colorEffect += (float)val;
    Logger::logInfo(state, block->sourceLine, "EFFECT", "Change", std::to_string((int)sprite->colorEffect));
}

void executeSetGraphicEffect(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->colorEffect = (float)val;
    Logger::logInfo(state, block->sourceLine, "EFFECT", "Set", std::to_string((int)sprite->colorEffect));
}

void executeClearGraphicEffects(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->colorEffect = 0.0f;
    Logger::logInfo(state, block->sourceLine, "EFFECT", "Clear", "0");
}

// SOUND BLOCKS

void executePlaySound(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    std::string name = toString(evaluateExpression(state, block->params[0]));
    for (auto& snd : sprite->sounds) {
        if (snd.name == name && snd.chunk && !snd.muted) {
            Mix_VolumeChunk(snd.chunk, (int)(snd.volume * MIX_MAX_VOLUME / 100));
            snd.channel = Mix_PlayChannel(-1, snd.chunk, 0);
            Logger::logInfo(state, block->sourceLine, "SOUND", "Play", name);
            return;
        }
    }
    Logger::logWarning(state, block->sourceLine, "SOUND", "Play", "Sound not found: " + name);
}

void executePlaySoundUntilDone(GameState& state, Block* block) {
    // Same as PlaySound for now (blocking not implemented for simplicity)
    executePlaySound(state, block);
}

void executeStopAllSounds(GameState& state, Block* block) {
    Mix_HaltChannel(-1);
    Logger::logInfo(state, block->sourceLine, "SOUND", "StopAll", "All sounds stopped");
}

void executeSetVolume(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double vol = toDouble(evaluateExpression(state, block->params[0]));
    vol = std::max(0.0, std::min(100.0, vol));
    for (auto& snd : sprite->sounds) snd.volume = (int)vol;
    Logger::logInfo(state, block->sourceLine, "SOUND", "SetVolume", std::to_string((int)vol) + "%");
}

void executeChangeVolume(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double change = toDouble(evaluateExpression(state, block->params[0]));
    for (auto& snd : sprite->sounds) {
        snd.volume = std::max(0, std::min(100, snd.volume + (int)change));
    }
    Logger::logInfo(state, block->sourceLine, "SOUND", "ChangeVolume", std::to_string((int)change));
}


// EVENTS
void executeBroadcast(GameState& state, Block* block) {
    std::string msg = toString(evaluateExpression(state, block->params[0]));
    state.lastBroadcast    = msg;
    state.broadcastPending = true;
    Logger::logInfo(state, block->sourceLine, "BROADCAST", "Send", msg);
}


// CONTROL FLOW

void executeWait(GameState& state, Block* block, float dt) {
    double secs = toDouble(evaluateExpression(state, block->params[0]));
    state.execCtx.waitTimer = (float)secs;
    state.execCtx.pc--; // stay on this block until timer finishes
    Logger::logInfo(state, block->sourceLine, "WAIT", "Timer", std::to_string((int)secs) + "s");
}

void executeRepeat(GameState& state, Block* block) {
    double count = toDouble(evaluateExpression(state, block->params[0]));
    int icount = (int)count;
    if (icount <= 0) {
        // Skip to EndRepeat
        if (block->jumpTarget >= 0) jumpTo(state, block->jumpTarget + 1);
        return;
    }
    state.execCtx.loopStack.push_back(icount);
    state.execCtx.loopReturnStack.push_back(state.execCtx.pc - 1); // index of Repeat block
    Logger::logInfo(state, block->sourceLine, "REPEAT", "Start", std::to_string(icount) + " times");
}

void executeForever(GameState& state, Block* block) {
    state.execCtx.loopReturnStack.push_back(state.execCtx.pc - 1);
    Logger::logInfo(state, block->sourceLine, "FOREVER", "Start", "infinite loop");
}

void executeIfThen(GameState& state, Block* block) {
    bool cond = evaluateCondition(state, block->params[0]);
    if (!cond) {
        // Jump to EndIf
        if (block->jumpTarget >= 0) jumpTo(state, block->jumpTarget + 1);
        Logger::logInfo(state, block->sourceLine, "IF", "Branch", "FALSE -> skip");
    } else {
        Logger::logInfo(state, block->sourceLine, "IF", "Branch", "TRUE -> enter");
    }
}

void executeIfThenElse(GameState& state, Block* block) {
    bool cond = evaluateCondition(state, block->params[0]);
    if (!cond) {
        // Jump to Else + 1
        if (block->elseTarget >= 0) jumpTo(state, block->elseTarget + 1);
        Logger::logInfo(state, block->sourceLine, "IF_ELSE", "Branch", "FALSE -> else");
    } else {
        Logger::logInfo(state, block->sourceLine, "IF_ELSE", "Branch", "TRUE -> if body");
    }
}

void executeWaitUntil(GameState& state, Block* block) {
    bool cond = evaluateCondition(state, block->params[0]);
    if (!cond) {
        state.execCtx.waitingUntil = true;
        state.execCtx.pc--; // stay on this block
        Logger::logInfo(state, block->sourceLine, "WAIT_UNTIL", "Waiting", "condition not met");
    } else {
        Logger::logInfo(state, block->sourceLine, "WAIT_UNTIL", "Continue", "condition met");
    }
}

void executeRepeatUntil(GameState& state, Block* block) {
    bool cond = evaluateCondition(state, block->params[0]);
    if (cond) {
        // Exit loop: jump to EndRepeat + 1
        if (block->jumpTarget >= 0) jumpTo(state, block->jumpTarget + 1);
        Logger::logInfo(state, block->sourceLine, "REPEAT_UNTIL", "Exit", "condition met");
    } else {
        // Enter/continue loop body
        state.execCtx.loopReturnStack.push_back(state.execCtx.pc - 1);
        Logger::logInfo(state, block->sourceLine, "REPEAT_UNTIL", "Loop", "condition not met");
    }
}

void executeStopAll(GameState& state, Block* block) {
    state.execCtx.isRunning = false;
    Logger::logInfo(state, block->sourceLine, "STOP_ALL", "Halt", "All stopped");
}

// SENSING

bool isTouchingMouse(GameState& state) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return false;
    int mx = state.mouseX - state.stageX;
    int my = state.mouseY - state.stageY;
    float hw = 32.0f * sprite->size / 100.0f;
    float hh = 32.0f * sprite->size / 100.0f;
    return (mx >= sprite->x - hw && mx <= sprite->x + hw &&
            my >= sprite->y - hh && my <= sprite->y + hh);
}

bool isTouchingEdge(GameState& state) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return false;
    return (sprite->x <= 0.0f || sprite->x >= (float)state.stageWidth ||
            sprite->y <= 0.0f || sprite->y >= (float)state.stageHeight);
}

float distanceToMouse(GameState& state) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return 0.0f;
    float mx = (float)(state.mouseX - state.stageX);
    float my = (float)(state.mouseY - state.stageY);
    float dx = sprite->x - mx;
    float dy = sprite->y - my;
    return std::sqrt(dx*dx + dy*dy);
}

float distanceToSprite(GameState& state, const std::string& name) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return 0.0f;
    for (auto* s : state.sprites) {
        if (s->name == name && s != sprite) {
            float dx = sprite->x - s->x;
            float dy = sprite->y - s->y;
            return std::sqrt(dx*dx + dy*dy);
        }
    }
    return 0.0f;
}

bool isKeyPressed(GameState& state, const std::string& key) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (key == "space")     return keys[SDL_SCANCODE_SPACE];
    if (key == "up")        return keys[SDL_SCANCODE_UP];
    if (key == "down")      return keys[SDL_SCANCODE_DOWN];
    if (key == "left")      return keys[SDL_SCANCODE_LEFT];
    if (key == "right")     return keys[SDL_SCANCODE_RIGHT];
    if (key == "enter")     return keys[SDL_SCANCODE_RETURN];
    if (key.size() == 1)    return keys[SDL_SCANCODE_A + (key[0] - 'a')];
    return false;
}

void executeAsk(GameState& state, Block* block) {
    std::string question = toString(evaluateExpression(state, block->params[0]));
    state.askQuestion     = question;
    state.waitingForAnswer = true;
    state.userAnswer       = "";
    Logger::logInfo(state, block->sourceLine, "ASK", "Question", question);
}

// CUSTOM FUNCTIONS


void executeDefineFunction(GameState& state, Block* block) {
    FunctionDef func;
    func.name       = block->name;
    func.paramNames = block->paramNames;
    func.body       = block->body;
    state.functions[func.name] = func;
    Logger::logInfo(state, block->sourceLine, "DEFINE_FN", "Defined", func.name);
}

void executeCallFunction(GameState& state, Block* block) {
    std::string name = block->name;
    if (state.functions.find(name) == state.functions.end()) {
        Logger::logError(state, block->sourceLine, "CALL_FN", "NotFound", name);
        return;
    }
    const FunctionDef& func = state.functions[name];

    // Evaluate arguments
    std::vector<Value> args;
    for (size_t i = 0; i < block->params.size(); i++) {
        args.push_back(evaluateExpression(state, block->params[i]));
    }

    executeFunctionBody(state, func, args);
    Logger::logInfo(state, block->sourceLine, "CALL_FN", "Called", name);
}

void executeFunctionBody(GameState& state, const FunctionDef& func,
                         const std::vector<Value>& args) {
    // Save current variables, set params
    ExecutionContext::CallFrame frame;
    frame.returnPC = state.execCtx.pc;

    // Set parameter variables
    for (size_t i = 0; i < func.paramNames.size() && i < args.size(); i++) {
        if (state.variables.count(func.paramNames[i]))
            frame.localVars[func.paramNames[i]] = state.variables[func.paramNames[i]];
        state.variables[func.paramNames[i]] = args[i];
    }
    state.execCtx.callStack.push_back(frame);

    // Execute body blocks inline
    // We insert them temporarily into editorBlocks
    int insertAt = state.execCtx.pc;
    for (size_t i = 0; i < func.body.size(); i++) {
        state.editorBlocks.insert(state.editorBlocks.begin() + insertAt + (int)i, func.body[i]);
    }
    // Re-preprocess to fix jump targets
    preProcessBlocks(state, state.editorBlocks);

    // After body executes, we'll restore via a special end marker
    // For simplicity, we execute the body directly here
    int savedPC = state.execCtx.pc;
    state.execCtx.pc = insertAt;

    for (size_t i = 0; i < func.body.size(); i++) {
        if (!state.execCtx.isRunning) break;
        Block* b = state.editorBlocks[state.execCtx.pc];
        state.execCtx.pc++;
        if (b) executeBlock(state, b, 0.016f);
    }

    // Restore
    // Remove inserted blocks
    state.editorBlocks.erase(
        state.editorBlocks.begin() + insertAt,
        state.editorBlocks.begin() + insertAt + (int)func.body.size()
    );
    state.execCtx.pc = savedPC;

    // Restore local vars
    if (!state.execCtx.callStack.empty()) {
        auto& cf = state.execCtx.callStack.back();
        for (auto& it : cf.localVars)
        {
            const std::string& k = it.first;
            Value& v = it.second;
            state.variables[k] = v;
        }
        state.execCtx.callStack.pop_back();
    }
}

// PEN BLOCKS

void executePenDown(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->penDown = true;
    Logger::logInfo(state, block->sourceLine, "PEN", "Down", "Pen down");
}

void executePenUp(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    sprite->penDown = false;
    Logger::logInfo(state, block->sourceLine, "PEN", "Up", "Pen up");
}

void executeStamp(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    PenStamp stamp;
    stamp.x    = sprite->x;
    stamp.y    = sprite->y;
    stamp.size = sprite->size;
    if (!sprite->costumes.empty())
        stamp.texture = sprite->costumes[sprite->currentCostume].texture;
    state.penStamps.push_back(stamp);
    Logger::logInfo(state, block->sourceLine, "PEN", "Stamp", "Stamped at (" +
                    std::to_string((int)sprite->x) + "," + std::to_string((int)sprite->y) + ")");
}

void executeEraseAll(GameState& state, Block* block) {
    state.penStrokes.clear();
    state.penStamps.clear();
    Logger::logInfo(state, block->sourceLine, "PEN", "EraseAll", "All pen drawings cleared");
}

void executeSetPenColor(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double r = toDouble(evaluateExpression(state, block->params[0]));
    double g = toDouble(evaluateExpression(state, block->params[1]));
    double b = toDouble(evaluateExpression(state, block->params[2]));
    sprite->penColor = {(Uint8)std::max(0.0, std::min(255.0, r)),
                        (Uint8)std::max(0.0, std::min(255.0, g)),
                        (Uint8)std::max(0.0, std::min(255.0, b)), 255};
    Logger::logInfo(state, block->sourceLine, "PEN", "Color",
                    "(" + std::to_string((int)r) + "," + std::to_string((int)g) + "," + std::to_string((int)b) + ")");
}

void executeSetPenSize(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->penSize = std::max(1, (int)val);
    Logger::logInfo(state, block->sourceLine, "PEN", "SetSize", std::to_string(sprite->penSize));
}

void executeChangePenSize(GameState& state, Block* block) {
    Sprite* sprite = getActiveSprite(state);
    if (!sprite) return;
    double val = toDouble(evaluateExpression(state, block->params[0]));
    sprite->penSize = std::max(1, sprite->penSize + (int)val);
    Logger::logInfo(state, block->sourceLine, "PEN", "ChangeSize", std::to_string(sprite->penSize));
}
    // Expression Evaluator
    Value evaluateExpression(GameState& state, Block* exprBlock) {}
    bool  evaluateCondition(GameState& state, Block* condBlock){}
    Value resolveVariable(GameState& state, const std::string& name){}

    // Safety
    bool  watchdogCheck(GameState& state){}
    void  clampPosition(GameState& state, Sprite& sprite){}
    Value safeDivide(double a, double b, GameState& state, int line){}
    Value safeSqrt(double x, GameState& state, int line){}

    //  Debug
    void checkDebugMode(GameState& state){}


    // SPEECH BUBBLE TIMER
    void updateSpeechBubbles(GameState& state, float dt) {
    for (auto* sprite : state.sprites) {
        if (sprite->speechTimer > 0.0f) {
            sprite->speechTimer -= dt;
            if (sprite->speechTimer <= 0.0f) {
                sprite->speechText  = "";
                sprite->speechTimer = -1.0f;
            }
        }
    }
}

}