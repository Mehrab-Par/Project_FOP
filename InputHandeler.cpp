#include "InputHandler.h"
#include "Engine.h"
#include "Renderer.h"
#include <iostream>
#include <algorithm>

namespace Input {


void handleEvent(GameState& state, SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            handleMouseMotion(state, event);
            break;
        case SDL_MOUSEBUTTONDOWN:
            handleMouseDown(state, event);
            break;
        case SDL_MOUSEBUTTONUP:
            handleMouseUp(state, event);
            break;
        case SDL_KEYDOWN:
            handleKeyDown(state, event);
            break;
        case SDL_TEXTINPUT:
            handleTextInput(state, event);
            break;
        default:
            break;
    }
}


//MOTION


void handleMouseMotion(GameState& state, SDL_Event& event) {
    state.mouseX = event.motion.x;
    state.mouseY = event.motion.y;


// Drag sprite on stage
    if (state.mouseDown && isOverStage(state, state.mouseX, state.mouseY)) {
        Sprite* sprite = nullptr;
        if (state.activeSpriteIndex >= 0 &&
            state.activeSpriteIndex < (int)state.sprites.size())
            sprite = state.sprites[state.activeSpriteIndex];

        if (sprite && sprite->draggable) {
            sprite->x = (float)(state.mouseX - state.stageX);
            sprite->y = (float)(state.mouseY - state.stageY);
            // Clamp
            sprite->x = std::max(0.0f, std::min((float)state.stageWidth, sprite->x));
            sprite->y = std::max(0.0f, std::min((float)state.stageHeight, sprite->y));
        }
    }
}


// DOWN


void handleMouseDown(GameState& state, SDL_Event& event) {
    int mx = event.button.x;
    int my = event.button.y;
    state.mouseX = mx;
    state.mouseY = my;
    state.mouseDown = true;

// buttons
    Button btn = getButtonAt(state, mx, my);
    switch (btn) {
        case Button::PLAY:
            if (state.execCtx.isRunning && state.execCtx.isPaused) {
                Engine::resumeExecution(state);
            } else if (!state.execCtx.isRunning) {
                Engine::startExecution(state);
            }
            return;

        case Button::PAUSE:
            if (state.execCtx.isRunning && !state.execCtx.isPaused) {
                Engine::pauseExecution(state);
            } else if (state.execCtx.isPaused) {
                Engine::resumeExecution(state);
            }
            return;

        case Button::STOP:
            Engine::stopExecution(state);
            Engine::engineReset(state);
            return;

        case Button::DEBUG:
            state.execCtx.isDebugMode = !state.execCtx.isDebugMode;
            return;

        case Button::LOG:
            state.showLogs = !state.showLogs;
            return;

        case Button::PEN_TOGGLE:
            state.penExtensionEnabled = !state.penExtensionEnabled;
            return;

        case Button::ADD_SPRITE: {
            // Add a default sprite
            Sprite* newSprite = new Sprite();
            newSprite->name = "Sprite" + std::to_string(state.sprites.size() + 1);
            newSprite->x = (float)state.stageWidth / 2;
            newSprite->y = (float)state.stageHeight / 2;
            newSprite->initX = newSprite->x;
            newSprite->initY = newSprite->y;
            newSprite->draggable = true;
            newSprite->layer = (int)state.sprites.size();
            state.sprites.push_back(newSprite);
            state.activeSpriteIndex = (int)state.sprites.size() - 1;
            return;
        }

        default:
            break;
    }


    int spriteIdx = getSpriteIconAt(state, mx, my);
    if (spriteIdx >= 0) {
        state.activeSpriteIndex = spriteIdx;
        return;
    }


    if (isOverStage(state, mx, my)) {
        int idx = getSpriteAtPoint(state, mx, my);
        if (idx >= 0) {
            state.activeSpriteIndex = idx;
        }
    }
}