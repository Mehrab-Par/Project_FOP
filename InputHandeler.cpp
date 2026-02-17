#include "InputHandler.h"
#include <iostream>
#include <algorithm>

namespace Input {


    bool pointInRect(int px, int py, float rx, float ry, float rw, float rh) {
        return (px >= rx && px <= (rx + rw) && py >= ry && py <= (ry + rh));
    }


    int getToolboxBlockIndex(const GameState& state, int mx, int my) {
        for (size_t i = 0; i < state.toolboxBlocks.size(); ++i) {
            Block* b = state.toolboxBlocks[i];
            if (pointInRect(mx, my, b->x, b->y, b->width, b->height)) {
                return (int)i;
            }
        }
        return -1;
    }


    //  MOUSE DOWN

    void handleMouseDown(GameState& state, const SDL_Event& event) {
        int mx = event.button.x;
        int my = event.button.y;
        state.mouseDown = true;

        std::cout << "Click at: " << mx << ", " << my << std::endl;

        //  Control Buttons
        Button clickedBtn = getButtonAt(state, mx, my);
        if (clickedBtn != Button::NONE) {
            std::cout << "Button action triggered." << std::endl;
            switch (clickedBtn) {
                case Button::PLAY:
                    state.isRunningVM = true;
                    std::cout << "VM Started" << std::endl;
                    return;
                case Button::STOP:
                    state.isRunningVM = false;
                    std::cout << "VM Stopped" << std::endl;
                    return;
                case Button::ADD_SPRITE:
                    {

                        Sprite* newSp = new Sprite();
                        newSp->name = "Sprite " + std::to_string(state.sprites.size());
                        newSp->x = 400;
                        newSp->y = 300;
                        state.sprites.push_back(newSp);
                        std::cout << "New Sprite Added" << std::endl;
                    }
                    return;
                default:
                    break;
            }
        }


        int tbIndex = getToolboxBlockIndex(state, mx, my);
        if (tbIndex != -1) {
            Block* tmpl = state.toolboxBlocks[tbIndex];


            Block* newBlock = new Block();
            newBlock->type = tmpl->type;
            newBlock->text = tmpl->text;
            newBlock->command = tmpl->command;
            newBlock->color = tmpl->color;
            newBlock->width = tmpl->width;
            newBlock->height = tmpl->height;


            newBlock->x = (float)mx - (tmpl->width / 2);
            newBlock->y = (float)my - (tmpl->height / 2);

            state.editorBlocks.push_back(newBlock);
            state.draggedBlock = newBlock;
            state.isDraggingBlock = true;
            std::cout << "Block cloned from toolbox" << std::endl;
            return;
        }

        for (auto it = state.editorBlocks.rbegin(); it != state.editorBlocks.rend(); ++it) {
            Block* b = *it;
            if (pointInRect(mx, my, b->x, b->y, b->width, b->height)) {
                state.draggedBlock = b;
                state.isDraggingBlock = true;
                std::cout << "Dragging existing block" << std::endl;
                return;
            }
        }
    }


    //  MOUSE UP

    void handleMouseUp(GameState& state, const SDL_Event& event) {
        state.mouseDown = false;

        if (state.isDraggingBlock && state.draggedBlock != nullptr) {

            if (state.draggedBlock->x < 250) {
                auto it = std::find(state.editorBlocks.begin(), state.editorBlocks.end(), state.draggedBlock);
                if (it != state.editorBlocks.end()) {
                    state.editorBlocks.erase(it);
                    delete state.draggedBlock; // آزادسازی حافظه
                    std::cout << "Block deleted" << std::endl;
                }
            } else {
                std::cout << "Block placed" << std::endl;
            }

            state.draggedBlock = nullptr;
            state.isDraggingBlock = false;
        }
    }


    // MOUSE MOTION

    void handleMouseMotion(GameState& state, const SDL_Event& event) {
        state.mouseX = event.motion.x;
        state.mouseY = event.motion.y;

        if (state.isDraggingBlock && state.draggedBlock != nullptr) {
            state.draggedBlock->x = (float)state.mouseX - (state.draggedBlock->width / 2);
            state.draggedBlock->y = (float)state.mouseY - (state.draggedBlock->height / 2);
        }
    }



    void handleKeyDown(GameState& state, const SDL_Event& event) {

    }

    void handleTextInput(GameState& state, const SDL_Event& event) {

    }


    //  HANDLE EVENT

    void handleEvent(GameState& state, SDL_Event& event) {
        switch (event.type) {
            case SDL_QUIT:
                state.running = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseDown(state, event);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseUp(state, event);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotion(state, event);
                break;
            case SDL_KEYDOWN:
                handleKeyDown(state, event);
                break;
            case SDL_TEXTINPUT:
                handleTextInput(state, event);
                break;
        }
    }



    Button getButtonAt(const GameState& state, int mx, int my) {

        if (pointInRect(mx, my, 800, 10, 50, 50)) return Button::PLAY;
        if (pointInRect(mx, my, 860, 10, 50, 50)) return Button::STOP;
        if (pointInRect(mx, my, 920, 10, 50, 50)) return Button::ADD_SPRITE;

        return Button::NONE;
    }

    bool isOverStage(const GameState& state, int mx, int my) {
        return pointInRect(mx, my, state.stageX, 0, state.stageWidth, 600);
    }

    int getSpriteAtPoint(const GameState& state, int mx, int my) {

        return -1;
    }

} // end namespace Input
