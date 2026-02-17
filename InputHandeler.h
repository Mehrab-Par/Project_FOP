#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <iostream>
#include "GameState.h"


// Input Handler — processes SDL events

namespace Input {

 
    void handleEvent(GameState& state, SDL_Event& event);


    void handleMouseMotion(GameState& state, const SDL_Event& event);
    void handleMouseDown(GameState& state, const SDL_Event& event);
    void handleMouseUp(GameState& state, const SDL_Event& event);
    void handleKeyDown(GameState& state, const SDL_Event& event);
    void handleTextInput(GameState& state, const SDL_Event& event);
    

    
    bool pointInRect(int px, int py, float rx, float ry, float rw, float rh);

   
    bool isOverStage(const GameState& state, int mx, int my);

    
    int  getSpriteAtPoint(const GameState& state, int mx, int my);

    
    int getToolboxBlockIndex(const GameState& state, int mx, int my);

    
    enum class Button {
        NONE,
        PLAY,
        PAUSE,
        STOP,
        DEBUG,
        LOG,
        PEN_TOGGLE,
        ADD_SPRITE
    };


    Button getButtonAt(const GameState& state, int mx, int my);
}
