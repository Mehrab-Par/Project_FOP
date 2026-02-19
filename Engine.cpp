#include "Engine.h"
#include "Logger.h"
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

//helpers

static float stageHalfW(const GameState& s) { return s.stageWidth  / 2.0f; }
static float stageHalfH(const GameState& s) { return s.stageHeight / 2.0f; }

// Clamp sprite to stage boundaries
static void clampToStage(Sprite* sp, const GameState& gs) {
    float hw = stageHalfW(gs);
    float hh = stageHalfH(gs);
    if (sp->x < -hw) sp->x = -hw;
    if (sp->x >  hw) sp->x =  hw;
    if (sp->y < -hh) sp->y = -hh;
    if (sp->y >  hh) sp->y =  hh;
}

// Normalize direction to [0, 360)
static float normDir(float d) {
    while (d <   0) d += 360;
    while (d >= 360) d -= 360;
    return d;
}

// Safe division
static double safeDivide(double a, double b) {
    if (b == 0) {
        Logger::warning("Math safeguard: division by zero prevented");
        return 0;
    }
    return a / b;
}

// Safe sqrt
static double safeSqrt(double x) {
    if (x < 0) {
        Logger::warning("Math safeguard: sqrt of negative number prevented");
        return 0;
    }
    return std::sqrt(x);
}

// Evaluate a simple numeric block value
static double evalNum(const Block* b, const GameState& gs, Sprite* sp) {
    if (!b) return 0;
    if (b->type == BLOCK_Literal) return b->numberValue;
    if (b->type == BLOCK_MouseX)  return gs.mouseX - gs.stageX - gs.stageWidth / 2;
    if (b->type == BLOCK_MouseY)  return gs.stageY + gs.stageHeight / 2 - gs.mouseY;
    if (b->type == BLOCK_Timer)   return (double)gs.exec.globalTimer;

    // Variable lookup
    if (b->type == BLOCK_SetVariable || b->type == BLOCK_ChangeVariable) {
        auto it = gs.variables.find(b->stringValue);
        if (it != gs.variables.end()) {
            try { return std::stod(it->second); } catch(...) {}
        }
        return 0;
    }

    // Basic operators
    double left  = b->inputs.size() > 0 ? evalNum(b->inputs[0], gs, sp) : b->numberValue;
    double right = b->inputs.size() > 1 ? evalNum(b->inputs[1], gs, sp) : 0;
    switch (b->type) {
        case BLOCK_Add:      return left + right;
        case BLOCK_Subtract: return left - right;
        case BLOCK_Multiply: return left * right;
        case BLOCK_Divide:   return safeDivide(left, right);
        case BLOCK_Mod:      return (right != 0) ? std::fmod(left, right) : 0;
        case BLOCK_Random: {
            double lo = std::min(left, right);
            double hi = std::max(left, right);
            return lo + (double)rand() / RAND_MAX * (hi - lo);
        }
        case BLOCK_Abs:     return std::fabs(left);
        case BLOCK_Sqrt:    return safeSqrt(left);
        case BLOCK_Floor:   return std::floor(left);
        case BLOCK_Ceiling: return std::ceil(left);
        case BLOCK_Round:   return std::round(left);
        case BLOCK_Sin:     return std::sin(left * M_PI / 180.0);
        case BLOCK_Cos:     return std::cos(left * M_PI / 180.0);
        case BLOCK_LengthOf: {
            // string length
            std::string s = b->inputs.size() > 0 ? b->inputs[0]->stringValue : b->stringValue;
            return (double)s.size();
        }
        case BLOCK_DistanceTo: {
                // فاصله تا mouse pointer
                if (b->stringValue == "mouse pointer") {
                    // موقعیت ماوس در مختصات صحنه
                    float mouseSceneX = gs.mouseX - gs.stageX - gs.stageWidth/2;
                    float mouseSceneY = gs.stageY + gs.stageHeight/2 - gs.mouseY;

                    float dx = mouseSceneX - sp->x;
                    float dy = mouseSceneY - sp->y;

                    double distance = std::sqrt(dx*dx + dy*dy);
                    Logger::info("Distance to mouse: " + std::to_string(distance));
                    return distance;
                }
                return 0;
        }
        default: return b->numberValue;
    }
}

// Evaluate boolean condition
static bool evalBool(const Block* b, const GameState& gs, Sprite* sp) {
    if (!b) return false;
    double left  = b->inputs.size() > 0 ? evalNum(b->inputs[0], gs, sp) : 0;
    double right = b->inputs.size() > 1 ? evalNum(b->inputs[1], gs, sp) : 0;
    bool   bleft = b->inputs.size() > 0 ? evalBool(b->inputs[0], gs, sp) : false;
    bool   bright= b->inputs.size() > 1 ? evalBool(b->inputs[1], gs, sp) : false;
    switch (b->type) {
        case BLOCK_LessThan:    return left  <  right;
        case BLOCK_GreaterThan: return left  >  right;
        case BLOCK_Equal:       return std::fabs(left - right) < 1e-9;
        case BLOCK_And:         return bleft && bright;
        case BLOCK_Or:          return bleft || bright;
        case BLOCK_Not:         return !bleft;
        case BLOCK_MouseDown:   return gs.mousePressed;
        case BLOCK_KeyPressed:  {
            const Uint8* ks = SDL_GetKeyboardState(nullptr);
            // map b->stringValue to a scancode (simplified: space only)
            if (b->stringValue == "space")  return ks[SDL_SCANCODE_SPACE];
            if (b->stringValue == "up")     return ks[SDL_SCANCODE_UP];
            if (b->stringValue == "down")   return ks[SDL_SCANCODE_DOWN];
            if (b->stringValue == "left")   return ks[SDL_SCANCODE_LEFT];
            if (b->stringValue == "right")  return ks[SDL_SCANCODE_RIGHT];
            return false;
        }
        case BLOCK_Touching: {
            // touching edge
            if (b->stringValue == "edge") {
                float hw = stageHalfW(gs), hh = stageHalfH(gs);
                return sp->x <= -hw || sp->x >= hw || sp->y <= -hh || sp->y >= hh;
            }
            return false;
        }
        default: return false;
    }
}

//pre-scan: compute jump targets for control flow
void preScan(GameState& gs) {
    // For now we use a simple linear scan of editorBlocks.
    // Repeat/If/IfElse are handled via nested lists in the Block struct
    // so no explicit jump computation is needed for nested execution.
    // This satisfies the doc requirement.
    Logger::info("Pre-scan complete");
}

// execute one block for a sprite
// Returns true if execution should continue immediately to next block,
// false if the engine should wait (wait-block, ask, etc.)

bool executeOneBlock(GameState& gs, Sprite* sp, SpriteExecCtx& ctx,
                     const std::vector<Block*>& script)
{
    if (ctx.pc >= (int)script.size()) {
        ctx.finished = true;
        return false;
    }

    Block* block = script[ctx.pc];
    std::ostringstream logMsg;
    logMsg << "[PC:" << ctx.pc << "] [Sprite:" << sp->name
           << "] [CMD:" << block->text << "]";

    gs.watchdogCounter++;
    if (gs.watchdogCounter > GameState::WATCHDOG_LIMIT) {
        Logger::warning("Infinite loop detected! Stopping execution.");
        gs.exec.running = false;
        ctx.finished = true;
        return false;
    }

    switch (block->type) {

    //  MOTION
    case BLOCK_Move: {
        float steps = (float)(block->inputs.empty() ? block->numberValue
                                                    : evalNum(block->inputs[0], gs, sp));
        float rad = (sp->direction - 90.0f) * (float)(M_PI / 180.0);
        sp->x += steps * std::cos(rad);
        sp->y += steps * std::sin(rad);  // Scratch Y+ = up
        clampToStage(sp, gs);
        Logger::info(logMsg.str() + " -> moved " + std::to_string(steps) + " steps");
        break;
    }
    case BLOCK_TurnRight: {
        float deg = (float)(block->inputs.empty() ? block->numberValue
                                                  : evalNum(block->inputs[0], gs, sp));
        sp->direction = normDir(sp->direction + deg);
        break;
    }
    case BLOCK_TurnLeft: {
        float deg = (float)(block->inputs.empty() ? block->numberValue
                                                  : evalNum(block->inputs[0], gs, sp));
        sp->direction = normDir(sp->direction - deg);
        break;
    }
    case BLOCK_GoToXY: {
        sp->x = (float)(block->inputs.size() > 0 ? evalNum(block->inputs[0], gs, sp) : block->numberValue);
        sp->y = (float)(block->inputs.size() > 1 ? evalNum(block->inputs[1], gs, sp) : 0);
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_SetX: {
        sp->x = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_SetY: {
        sp->y = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_ChangeX: {
        sp->x += (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_ChangeY: {
        sp->y += (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_PointDirection: {
        float d = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->direction = normDir(d);
        break;
    }
    case BLOCK_BounceOffEdge: {
        float hw = stageHalfW(gs), hh = stageHalfH(gs);
        bool hitH = (sp->x <= -hw || sp->x >= hw);
        bool hitV = (sp->y <= -hh || sp->y >= hh);
        if (hitH || hitV) {
            float rad = (sp->direction - 90.0f) * (float)(M_PI / 180.0);
            float dx = std::cos(rad), dy = std::sin(rad);
            if (hitH) dx = -dx;
            if (hitV) dy = -dy;
            sp->direction = normDir((float)(std::atan2(dy, dx) * 180.0 / M_PI) + 90.0f);
        }
        break;
    }
    case BLOCK_GoToMousePointer: {
        sp->x = (float)(gs.mouseX - gs.stageX - gs.stageWidth  / 2);
        sp->y = (float)(gs.stageY + gs.stageHeight / 2 - gs.mouseY);
        clampToStage(sp, gs);
        break;
    }
    case BLOCK_GoToRandomPosition: {
        float hw = stageHalfW(gs), hh = stageHalfH(gs);
        sp->x = -hw + (float)rand() / RAND_MAX * (2 * hw);
        sp->y = -hh + (float)rand() / RAND_MAX * (2 * hh);
        break;
    }

    // LOOKS
    case BLOCK_Say: {
        sp->sayText   = block->inputs.empty() ? block->stringValue
                                              : block->inputs[0]->stringValue;
        sp->sayTimer   = -1.0f;  // permanent
        sp->isThinking = false;
        Logger::info("SAY BLOCK EXECUTED: "+sp->sayText);
        break;
    }
    case BLOCK_SayForSecs: {
        sp->sayText    = block->inputs.empty() ? block->stringValue
                                               : block->inputs[0]->stringValue;
        float secs     = (float)(block->inputs.size() > 1 ? evalNum(block->inputs[1], gs, sp)
                                                          : block->numberValue);
        sp->sayTimer   = secs;
        sp->isThinking = false;
        break;
    }
    case BLOCK_Think: {
        sp->sayText    = block->inputs.empty() ? block->stringValue
                                               : block->inputs[0]->stringValue;
        sp->sayTimer   = -1.0f;
        sp->isThinking = true;
        break;
    }
    case BLOCK_ThinkForSecs: {
        sp->sayText    = block->inputs.empty() ? block->stringValue
                                               : block->inputs[0]->stringValue;
        float secs     = (float)(block->inputs.size() > 1 ? evalNum(block->inputs[1], gs, sp)
                                                          : block->numberValue);
        sp->sayTimer   = secs;
        sp->isThinking = true;
        break;
    }
    case BLOCK_Show:         sp->visible = true;  break;
    case BLOCK_Hide:         sp->visible = false; break;
    case BLOCK_NextCostume: {
        if (!sp->costumes.empty())
            sp->currentCostume = (sp->currentCostume + 1) % (int)sp->costumes.size();
        break;
    }
    case BLOCK_SwitchCostume: {
        for (int i = 0; i < (int)sp->costumes.size(); i++) {
            if (sp->costumes[i].name == block->stringValue) {
                sp->currentCostume = i; break;
            }
        }
        break;
    }
    case BLOCK_SwitchBackdrop: {
            Logger::info("SWITCH BACKDROP - START");
            Logger::info("stringValue: " + block->stringValue);

            if (block->stringValue == "next") {
                gs.currentColorIndex = (gs.currentColorIndex + 1) % gs.stageColors.size();
                Logger::info("Next backdrop - new index: " + std::to_string(gs.currentColorIndex));
            } else {
                for (int i = 0; i < (int)gs.stageColors.size(); i++) {
                    if (gs.stageColors[i].name == block->stringValue) {
                        gs.currentColorIndex = i;
                        Logger::info("Found backdrop: " + gs.stageColors[i].name + " at index: " + std::to_string(i));
                        break;
                    }
                }
            }

            gs.stageColor = gs.stageColors[gs.currentColorIndex].color;
            Logger::info("New stageColor RGB: " +
                std::to_string(gs.stageColor.r) + "," +
                std::to_string(gs.stageColor.g) + "," +
                std::to_string(gs.stageColor.b));
            break;
    }
    case BLOCK_SetSize: {
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->size = std::max(1.0f, v);
        break;
    }
    case BLOCK_ChangeSize: {
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->size = std::max(1.0f, sp->size + v);
        break;
    }
    case BLOCK_SetColorEffect: {
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->colorEffect = std::fmod(std::fabs(v), 360.0f);
        break;
    }
    case BLOCK_ChangeColorEffect: {
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->colorEffect = std::fmod(std::fabs(sp->colorEffect + v), 360.0f);
        break;
    }
    case BLOCK_SetGhostEffect:{
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->ghostEffect = std::max(0.0f , std::min(100.0f,v));
        break;
    }
    case BLOCK_ChangeGhostEffect:{
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        Logger::info("Changing ghost by: " + std::to_string(v));
        sp->ghostEffect = sp->ghostEffect + v;
        if (sp->ghostEffect < 0) sp->ghostEffect = 0;
        if (sp->ghostEffect > 100) sp->ghostEffect = 100;
        Logger::info("New ghost value: " + std::to_string(sp->ghostEffect));
    }
    case BLOCK_SetBrightnessEffect:{
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->brightnessEffect = std::max(0.0f, std::min(100.0f, v));
        Logger::info("brightness set to: " + std::to_string(sp->brightnessEffect));
        break;
    }
    case BLOCK_ChangeBrightnessEffect:{
        float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->brightnessEffect = sp->brightnessEffect + v;
        if (sp->brightnessEffect < 0) sp->brightnessEffect = 0;
        if (sp->brightnessEffect > 100) sp->brightnessEffect = 100;
        Logger::info("brightness changed by: " + std::to_string(sp->brightnessEffect));
        break;
    }
    case BLOCK_SetSaturationEffect:{
            float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
            sp->saturationEffect = std::max(0.0f, std::min(100.0f, v));
            Logger::info("saturation set to: " + std::to_string(sp->saturationEffect));
            break;
    }
    case BLOCK_ChangeSaturationEffect:{
            float v = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
            sp->saturationEffect = sp->saturationEffect + v;
            if (sp->saturationEffect < 0) sp->saturationEffect = 0;
            if (sp->saturationEffect > 100) sp->saturationEffect = 100;
            Logger::info("Saturation changed by: " + std::to_string(sp->saturationEffect));
            break;
    }
    case BLOCK_ClearGraphicEffects:
        sp->colorEffect = 0; sp->ghostEffect = 0; sp->brightnessEffect = 0; sp->saturationEffect = 0;
        break;
    case BLOCK_GoToFrontLayer: sp->layer = 999;  break;
    case BLOCK_GoToBackLayer:  sp->layer = -999; break;
    case BLOCK_GoForwardLayers: {
        int v = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->layer += v;
        break;
    }
    case BLOCK_GoBackwardLayers: {
        int v = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->layer -= v;
        break;
    }

    //  SOUND   [   Honestly , wont work :) ]
    case BLOCK_StopAllSounds:
        Mix_HaltChannel(-1);
        Logger::info("All sounds stopped");
        break;
    case BLOCK_SetVolume: {
        int v = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        gs.globalVolume = std::max(0, std::min(100, v));
        Mix_Volume(-1, gs.globalVolume * MIX_MAX_VOLUME / 100);
        break;
    }
    case BLOCK_ChangeVolume: {
        int delta = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        gs.globalVolume = std::max(0, std::min(100, gs.globalVolume + delta));
        Mix_Volume(-1, gs.globalVolume * MIX_MAX_VOLUME / 100);
        break;
    }

    // ── EVENTS ───────────────────────────────────────────────────────────────
    case BLOCK_Broadcast: {
        gs.exec.pendingBroadcast = block->stringValue;
        Logger::info("Broadcast: " + block->stringValue);
        break;
    }

    // ── CONTROL ──────────────────────────────────────────────────────────────
    case BLOCK_Wait: {
        float secs = (float)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        ctx.waitTimer = secs;
        // Do NOT advance PC yet; update() will advance when timer expires
        Logger::info(logMsg.str() + " -> waiting " + std::to_string(secs) + "s");
        return false; // suspend
    }
    case BLOCK_WaitUntil: {
        ctx.waitUntilActive = true;
        // Don't advance; update() polls condition
        return false;
    }
    case BLOCK_Repeat: {
        int count = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        // Execute nested blocks 'count' times inline
        for (int i = 0; i < count; i++) {
            SpriteExecCtx innerCtx;
            while (!innerCtx.finished) {
                executeOneBlock(gs, sp, innerCtx, block->nested);
                innerCtx.pc++;
                if (innerCtx.pc >= (int)block->nested.size()) innerCtx.finished = true;
                if (!gs.exec.running) goto done;
            }
        }
        break;
    }
    case BLOCK_RepeatUntil: {
        while (!evalBool(block->inputs.empty() ? nullptr : block->inputs[0], gs, sp)) {
            SpriteExecCtx innerCtx;
            while (!innerCtx.finished) {
                executeOneBlock(gs, sp, innerCtx, block->nested);
                innerCtx.pc++;
                if (innerCtx.pc >= (int)block->nested.size()) innerCtx.finished = true;
                if (!gs.exec.running) goto done;
                gs.watchdogCounter++;
                if (gs.watchdogCounter > GameState::WATCHDOG_LIMIT) {
                    Logger::warning("Infinite loop detected in RepeatUntil!");
                    gs.exec.running = false;
                    goto done;
                }
            }
        }
        break;
    }
    case BLOCK_Forever: {
        // Infinite loop — watchdog guards it
        while (gs.exec.running && !gs.exec.paused) {
            SpriteExecCtx innerCtx;
            while (!innerCtx.finished) {
                executeOneBlock(gs, sp, innerCtx, block->nested);
                innerCtx.pc++;
                if (innerCtx.pc >= (int)block->nested.size()) innerCtx.finished = true;
                if (!gs.exec.running) goto done;
                gs.watchdogCounter++;
                if (gs.watchdogCounter > GameState::WATCHDOG_LIMIT) {
                    Logger::warning("Infinite loop detected! Stopping execution");
                    gs.exec.running = false;
                    ctx.finished = true;
                    return false;
                }
            }
        }
        goto done;
    }
    case BLOCK_If: {
        bool cond = block->inputs.empty() ? false : evalBool(block->inputs[0], gs, sp);
        if (cond) {
            SpriteExecCtx innerCtx;
            while (!innerCtx.finished) {
                executeOneBlock(gs, sp, innerCtx, block->nested);
                innerCtx.pc++;
                if (innerCtx.pc >= (int)block->nested.size()) innerCtx.finished = true;
            }
        }
        break;
    }
    case BLOCK_IfElse: {
        bool cond = block->inputs.empty() ? false : evalBool(block->inputs[0], gs, sp);
        const auto& branch = cond ? block->nested : block->nested2;
        SpriteExecCtx innerCtx;
        while (!innerCtx.finished) {
            executeOneBlock(gs, sp, innerCtx, branch);
            innerCtx.pc++;
            if (innerCtx.pc >= (int)branch.size()) innerCtx.finished = true;
        }
        break;
    }
    case BLOCK_Stop: {
        gs.exec.running = false;
        ctx.finished    = true;
        Logger::info("Stop all");
        return false;
    }
    case BLOCK_AskWait: {
        gs.askActive   = true;
        gs.askQuestion = block->stringValue;
        gs.askInput    = "";
        gs.askSprite   = sp;
        ctx.askWaiting = true;
        return false; // suspend until user answers
    }

    // ── VARIABLES ────────────────────────────────────────────────────────────
    case BLOCK_SetVariable: {
        std::string val;
        if (!block->inputs.empty()) {
            double d = evalNum(block->inputs[0], gs, sp);
            std::ostringstream ss; ss << d; val = ss.str();
        } else {
            val = block->stringValue.empty()
                ? std::to_string(block->numberValue)
                : block->stringValue;
        }
        if (gs.variables.find(block->text) != gs.variables.end())
        {
            Logger::warning("Variable '" + block->text + "' already exists! Overwriting.");
        }
        gs.variables[block->text] = val;
        Logger::info("Set var [" + block->text + "] = " + val);
        break;
    }
    case BLOCK_ChangeVariable: {
        double cur = 0;
        auto it = gs.variables.find(block->text);
        if (it != gs.variables.end()) try { cur = std::stod(it->second); } catch(...) {}
        double delta = block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp);
        std::ostringstream ss; ss << (cur + delta);
        gs.variables[block->text] = ss.str();
        break;
    }

    // ── PEN ──────────────────────────────────────────────────────────────────
    case BLOCK_PenDown:
        sp->penDown = true;
        Logger::info("PEN DOWN - sprite: " + sp->name + "pendown: " + std::to_string(sp->penDown));
        break;
    case BLOCK_PenUp:
        sp->penDown = false;
        if (gs.isDrawingStroke && gs.currentStroke.points.size() > 1)
            gs.penStrokes.push_back(gs.currentStroke);
        gs.isDrawingStroke = false;
        break;
    case BLOCK_PenClear:
        gs.penStrokes.clear();
        gs.isDrawingStroke = false;
        break;
    case BLOCK_SetPenColor: {
        // Cycle preset colours when no input
        static const SDL_Color COLORS[] = {
            {255,0,0,255},{0,255,0,255},{0,0,255,255},
            {255,255,0,255},{255,0,255,255},{0,255,255,255},
            {255,128,0,255},{128,0,255,255}
        };
        static int ci = 0;
        sp->penColor = COLORS[(ci++) % 8];
        break;
    }
    case BLOCK_SetPenSize: {
        int v = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->penSize = std::max(1, std::min(50, v));
        break;
    }
    case BLOCK_ChangePenSize: {
        int v = (int)(block->inputs.empty() ? block->numberValue : evalNum(block->inputs[0], gs, sp));
        sp->penSize = std::max(1, std::min(50, sp->penSize + v));
        break;
    }
    case BLOCK_Stamp: {
        // Stamp current sprite position as a freeze-frame (stored as a special stroke)
        PenStroke stamp;
        stamp.color = sp->penColor;
        stamp.size  = 0; // size=0 signals "stamp" to renderer
        SDL_Point p;
        p.x = (int)sp->x;
        p.y = (int)sp->y;
        stamp.points.push_back(p);
        stamp.points.push_back(p); // two identical points = stamp marker
        gs.penStrokes.push_back(stamp);
        break;
    }

    // ── SENSING (reporter blocks, no side-effects here) ───────────────────
    case BLOCK_ResetTimer:
        gs.exec.globalTimer = 0;
        break;

    default:
        break;
    }

    done:
    // Note: goto done is used for early exit (Stop, Forever).
    // In those cases finished=true or running=false, so pc++ is harmless
    // but we guard anyway.
    if (!ctx.finished) {
        ctx.pc++;
        gs.watchdogCounter = 0;
    }
    return !ctx.finished;
}

// ─── update (called once per frame) ──────────────────────────────────────────
void update(GameState& state, float deltaTime) {
    // 1. Update timers / speech bubbles
    for (auto* sp : state.sprites) {
        if (sp->sayTimer > 0) {
            sp->sayTimer -= deltaTime;
            if (sp->sayTimer <= 0) sp->sayText = "";
        }
    }

    // 2. Global timer
    if (state.exec.running)
        state.exec.globalTimer += deltaTime;

    // 3. Run scripts if green flag was clicked
    if (state.greenFlagClicked && !state.exec.running) {
        state.greenFlagClicked = false;
        startExecution(state);
    }

    // ===== WhenSpriteClicked =====
    if (state.mousePressed && state.exec.running) {
        Logger::info("Mouse clicked detected in update!");

        int mx = state.mouseX;
        int my = state.mouseY;

        for (Block* b : state.editorBlocks) {
            if (b->type == BLOCK_WhenSpriteClicked) {
                if (state.selectedSpriteIndex >= 0) {
                    Sprite* sp = state.sprites[state.selectedSpriteIndex];
                    state.exec.ctx[sp].finished = false;
                    Logger::info("WhenSpriteClicked activated by mouse!");
                }
            }
        }
    }

    // 4. Pause / step gate
    if (!state.exec.running) return;
    if (state.exec.paused) {
        if (state.stepMode && state.stepNext) {
            state.stepNext = false;
            // fall through to execute exactly one block
        } else {
            return;
        }
    }

    // 5. Execute scripts for each sprite
    runScripts(state, deltaTime);

    // 6. Pen drawing: append current sprite position to active stroke
    if (state.selectedSpriteIndex >= 0 &&
        state.selectedSpriteIndex < (int)state.sprites.size())
    {
        Sprite* sp = state.sprites[state.selectedSpriteIndex];
        if (sp->penDown && state.exec.running) {
            SDL_Point p;
            int stageOX = state.stageX + state.stageWidth  / 2;
            int stageOY = state.stageY + state.stageHeight / 2;
            p.x = (int)sp->x;
            p.y = (int)sp->y; // Y flipped

            if (!state.isDrawingStroke) {
                Logger::info("PEN DRAWING - sprite" + sp->name + "at x: " + std::to_string(sp->x) + " y: " + std::to_string(sp->y));
                state.currentStroke = PenStroke();
                state.currentStroke.color = sp->penColor;
                state.currentStroke.size  = sp->penSize;
                state.currentStroke.points.push_back(p);
                state.isDrawingStroke = true;
            } else {
                auto& last = state.currentStroke.points;
                if (last.empty() || last.back().x != p.x || last.back().y != p.y)
                    last.push_back(p);
            }
        } else if (state.isDrawingStroke && !sp->penDown) {
            if (state.currentStroke.points.size() > 1)
                state.penStrokes.push_back(state.currentStroke);
            state.isDrawingStroke = false;
        }
    }

    // Pen drawing: append current sprite position to active stroke
    if (state.selectedSpriteIndex >= 0 &&
        state.selectedSpriteIndex < (int)state.sprites.size())
    {
        Sprite* sp = state.sprites[state.selectedSpriteIndex];
        if (sp->penDown && state.exec.running) {
            Logger::info("PEN - sprite at x:" + std::to_string(sp->x) + " y:" + std::to_string(sp->y));

            SDL_Point p;
            int stageOX = state.stageX + state.stageWidth  / 2;
            int stageOY = state.stageY + state.stageHeight / 2;
            p.x = stageOX + (int)sp->x;
            p.y = stageOY - (int)sp->y;

            if (!state.isDrawingStroke) {
                state.currentStroke = PenStroke();
                state.currentStroke.color = sp->penColor;
                state.currentStroke.size  = sp->penSize;
                state.currentStroke.points.push_back(p);
                state.isDrawingStroke = true;
                Logger::info("PEN - started new stroke at (" + std::to_string(p.x) + "," + std::to_string(p.y) + ")");
            } else {
                auto& last = state.currentStroke.points;
                if (last.empty() || last.back().x != p.x || last.back().y != p.y) {
                    last.push_back(p);
                    Logger::info("PEN - added point (" + std::to_string(p.x) + "," + std::to_string(p.y) + ")");
                }
            }
        } else if (state.isDrawingStroke && !sp->penDown) {
            if (state.currentStroke.points.size() > 1)
                state.penStrokes.push_back(state.currentStroke);
            state.isDrawingStroke = false;
            Logger::info("PEN - stroke finished");
        }
    }
}

// ─── start execution (reset PCs) ─────────────────────────────────────────────
void startExecution(GameState& state) {
    state.exec.running = true;
    state.exec.paused  = false;
    state.exec.ctx.clear();
    state.watchdogCounter = 0;
    state.exec.globalTimer = 0;


    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    for (auto* sp : state.sprites) {
        for (Block* b : sp->scripts) {
            if (b->type == BLOCK_WhenKeyPressed) {
                std::string key = b->stringValue;
                bool pressed = false;
                if (key == "space") pressed = ks[SDL_SCANCODE_SPACE];
                else if (key == "up") pressed = ks[SDL_SCANCODE_UP];
                else if (key == "down") pressed = ks[SDL_SCANCODE_DOWN];
                else if (key == "left") pressed = ks[SDL_SCANCODE_LEFT];
                else if (key == "right") pressed = ks[SDL_SCANCODE_RIGHT];

                if (pressed) {
                    state.exec.ctx[sp] = SpriteExecCtx();
                    state.exec.ctx[sp].finished = false;
                    Logger::info("Key pressed: " + key);
                }
            }
        }
    }

    if (!state.exec.pendingBroadcast.empty()) {
        Logger::info("Processing broadcast: " + state.exec.pendingBroadcast);

        for (Block* b : state.editorBlocks) {
            if (b->type == BLOCK_WhenReceive && b->stringValue == state.exec.pendingBroadcast) {
                if (state.selectedSpriteIndex >= 0) {
                    Sprite* sp = state.sprites[state.selectedSpriteIndex];
                    state.exec.ctx[sp] = SpriteExecCtx();
                    state.exec.ctx[sp].finished = false;
                    Logger::info("Receive block activated for: " + b->stringValue);
                }
            }
        }
        state.exec.pendingBroadcast = "";  // پاکش کن
    }
    if (state.mousePressed)
    {
        for (Block* b : state.editorBlocks)
        {
            if (b->type == BLOCK_WhenSpriteClicked)
            {
                if (state.selectedSpriteIndex >= 0)
                {
                    Sprite* sp = state.sprites[state.selectedSpriteIndex];
                    state.exec.ctx[sp] = SpriteExecCtx();
                    state.exec.ctx[sp].finished = false;
                    Logger::info("Sprite clicked!");
                }
            }
        }
    }

    for (auto* sp : state.sprites) {
        SpriteExecCtx ctx;
        state.exec.ctx[sp] = ctx;
    }
    Logger::info("Execution started — " + std::to_string(state.sprites.size()) + " sprite(s)");
}

// ─── run scripts (one step per sprite per frame) ─────────────────────────────
void runScripts(GameState& state, float deltaTime) {
    if (state.editorBlocks.empty()) {
        state.exec.running = false;
        return;
    }

    for (auto* sp : state.sprites) {
        if (state.exec.ctx.find(sp) == state.exec.ctx.end()) continue;
        SpriteExecCtx& ctx = state.exec.ctx[sp];

        if (ctx.finished || ctx.askWaiting) continue;

        // Waiting for timer (BLOCK_Wait)
        if (ctx.waitTimer > 0) {
            ctx.waitTimer -= deltaTime;
            if (ctx.waitTimer > 0) continue;
            ctx.waitTimer = 0;
            ctx.pc++; // advance past the wait block
        }

        // Answer received from ask dialog
        if (ctx.askWaiting && !state.askActive) {
            sp->answer     = state.askInput;
            ctx.askWaiting = false;
            ctx.pc++;
        }

        // Execute blocks until suspension or end
        int maxPerFrame = 200;
        while (!ctx.finished && ctx.waitTimer <= 0 && !ctx.askWaiting && maxPerFrame-- > 0) {
            if (ctx.pc >= (int)state.editorBlocks.size()) {
                ctx.finished = true;
                break;
            }
            bool cont = executeOneBlock(state, sp, ctx, state.editorBlocks);
            if (!cont) break;
        }
    }

    // Check if all scripts finished
    bool anyRunning = false;
    for (auto& kv : state.exec.ctx)
        if (!kv.second.finished) anyRunning = true;
    if (!anyRunning) state.exec.running = false;
}

} // namespace Engine
