#include "InputHandler.h"
#include "Engine.h"
#include "Logger.h"
#include "UIManager.h"
#include <iostream>
#include <cmath>

namespace Input {

void handleEvent(GameState& state, SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            handleMouseDown(state, event.button.x, event.button.y);
            break;
        case SDL_MOUSEBUTTONUP:
            handleMouseUp(state, event.button.x, event.button.y);
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotion(state, event.motion.x, event.motion.y);
            break;
        case SDL_KEYDOWN:
            handleKeyPress(state, event.key.keysym.sym);
            break;
        case SDL_TEXTINPUT:
            if (state.askActive) {
                state.askInput += event.text.text;
            }
            break;
        default: break;
    }
}

void handleMouseDown(GameState& state, int x, int y) {
    state.mouseX    = x;
    state.mouseY    = y;
    state.mousePressed = true;

    // If ask dialog active, ignore all other interaction
    if (state.askActive) return;

    // ── Palette (left panel) ──────────────────────────────────────────────
    const int CAT_TAB_H = 22;
    const int palClickMinY = UILayout::MENU_H + CAT_TAB_H + 2;
    if (x < state.paletteWidth && y > palClickMinY) {
        // Find block accounting for scroll offset
        // Each block's stored .y is its base position; rendered as y - paletteScrollY
        Block* clicked = nullptr;
        for (int i = (int)state.paletteBlocks.size()-1; i >= 0; i--) {
            Block* b = state.paletteBlocks[i];
            // category filter
            if (state.paletteCategory >= 0 && (int)b->category != state.paletteCategory)
                continue;
            int drawY = b->y - state.paletteScrollY;
            if (x >= b->x && x < b->x + b->width &&
                y >= drawY  && y < drawY + b->height) {
                clicked = b;
                break;
            }
        }

        if (clicked) {
            int drawY = clicked->y - state.paletteScrollY;
            // Clone the palette block into a new editor block
            Block* nb = new Block();
            nb->type        = clicked->type;
            nb->category    = clicked->category;
            nb->text        = clicked->text;
            nb->numberValue = clicked->numberValue;
            nb->stringValue = clicked->stringValue;
            nb->width       = clicked->width;
            nb->height      = clicked->height;
            nb->x           = x;
            nb->y           = y;

            state.draggedBlock        = nb;
            state.draggingFromPalette = true;
            state.dragOffsetX         = x - clicked->x;
            state.dragOffsetY         = y - drawY;
            Logger::info("Dragging new block: " + nb->text);
        }
        return;
    }

    // ── Editor (centre panel) ─────────────────────────────────────────────
    if (x >= state.editorX && x < state.stageX && y > 35) {
        Block* clicked = findBlockAt(state.editorBlocks, x, y);
        if (clicked) {
            state.draggedBlock        = clicked;
            state.draggingFromPalette = false;
            state.dragOffsetX         = x - clicked->x;
            state.dragOffsetY         = y - clicked->y;
        }
    }
}

void handleMouseUp(GameState& state, int x, int y) {
    state.mousePressed = false;
    if (!state.draggedBlock) return;

    if (state.draggingFromPalette) {
        // Only add to editor if dropped in editor zone
        if (x >= state.editorX && x < state.stageX && y > 35) {
            // Apply snap
            Block* target = findSnapTarget(state);
            if (target) {
                if (state.snapAbove) {
                    state.draggedBlock->y      = target->y - state.draggedBlock->height - 4;
                    state.draggedBlock->nextBlock = target;
                } else {
                    state.draggedBlock->y      = target->y + target->height + 4;
                    target->nextBlock          = state.draggedBlock;
                }
                state.draggedBlock->x = target->x;
            }
            state.editorBlocks.push_back(state.draggedBlock);
            Logger::info("Block added to editor: " + state.draggedBlock->text);
        } else {
            // Dropped outside editor — discard
            delete state.draggedBlock;
        }
    } else {
        // Moving an existing editor block: it stays in editorBlocks,
        // just update position.
        Block* target = findSnapTarget(state);
        if (target && target != state.draggedBlock) {
            if (state.snapAbove) {
                state.draggedBlock->y      = target->y - state.draggedBlock->height - 4;
                state.draggedBlock->nextBlock = target;
            } else {
                state.draggedBlock->y      = target->y + target->height + 4;
                target->nextBlock          = state.draggedBlock;
            }
            state.draggedBlock->x = target->x;
        }

        // If dropped back in palette, delete it from editor
        if (x < state.paletteWidth) {
            auto& eb = state.editorBlocks;
            eb.erase(std::remove(eb.begin(), eb.end(), state.draggedBlock), eb.end());
            delete state.draggedBlock;
        }
    }

    state.draggedBlock = nullptr;
    state.snapTarget   = nullptr;
}

void handleMouseMotion(GameState& state, int x, int y) {
    state.mouseX = x;
    state.mouseY = y;

    if (state.draggedBlock) {
        state.draggedBlock->x = x - state.dragOffsetX;
        state.draggedBlock->y = y - state.dragOffsetY;

        if (x >= state.editorX && x < state.stageX)
            state.snapTarget = findSnapTarget(state);
        else
            state.snapTarget = nullptr;
    }
}

void handleKeyPress(GameState& state, SDL_Keycode key) {
    // Ask dialog: handle typing
    if (state.askActive) {
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            // Submit answer
            state.askActive = false;
            SDL_StopTextInput();
            Logger::info("Ask answered: " + state.askInput);
        } else if (key == SDLK_BACKSPACE && !state.askInput.empty()) {
            state.askInput.pop_back();
        }
        return;
    }

    switch (key) {
        case SDLK_SPACE:
            if (state.exec.running) {
                state.exec.paused = !state.exec.paused;
                Logger::info(state.exec.paused ? "Paused" : "Resumed");
            } else {
                state.greenFlagClicked = true;
            }
            break;

        case SDLK_s:
            // Step mode toggle
            state.stepMode = !state.stepMode;
            Logger::info(state.stepMode ? "Step mode ON" : "Step mode OFF");
            break;

        case SDLK_n:
            if (state.stepMode && state.exec.paused) {
                state.stepNext = true;
            }
            break;

        case SDLK_DELETE:
        case SDLK_BACKSPACE: {
            // Delete selected blocks from editor
            auto& eb = state.editorBlocks;
            for (int i = (int)eb.size()-1; i >= 0; i--) {
                if (eb[i]->selected) {
                    delete eb[i];
                    eb.erase(eb.begin() + i);
                }
            }
            break;
        }

        case SDLK_b:
            // Cycle background colour
            state.currentColorIndex = (state.currentColorIndex + 1) % (int)state.stageColors.size();
            state.stageColor = state.stageColors[state.currentColorIndex].color;
            Logger::info("Background: " + state.stageColors[state.currentColorIndex].name);
            break;

        case SDLK_c:
            // Next costume for selected sprite
            if (state.selectedSpriteIndex >= 0 &&
                state.selectedSpriteIndex < (int)state.sprites.size()) {
                Sprite* sp = state.sprites[state.selectedSpriteIndex];
                if (!sp->costumes.empty())
                    sp->currentCostume = (sp->currentCostume + 1) % (int)sp->costumes.size();
            }
            break;

        default: break;
    }
}

Block* findBlockAt(const std::vector<Block*>& blocks, int x, int y) {
    for (int i = (int)blocks.size()-1; i >= 0; i--) {
        Block* b = blocks[i];
        if (x >= b->x && x < b->x + b->width &&
            y >= b->y && y < b->y + b->height)
            return b;
    }
    return nullptr;
}

Block* findSnapTarget(GameState& state) {
    if (!state.draggedBlock) return nullptr;
    const int SNAP_DIST = 28;
    Block* best = nullptr;
    float  minD = 999999.0f;

    for (Block* target : state.editorBlocks) {
        if (target == state.draggedBlock) continue;

        if (std::abs(target->x - state.draggedBlock->x) >= 50) continue;

        // Snap below target
        int distBelow = std::abs((target->y + target->height + 4) - state.draggedBlock->y);
        if (distBelow < SNAP_DIST && distBelow < minD) {
            minD  = (float)distBelow;
            best  = target;
            state.snapAbove = false;
        }
        // Snap above target
        int distAbove = std::abs((target->y - state.draggedBlock->height - 4) - state.draggedBlock->y);
        if (distAbove < SNAP_DIST && distAbove < minD) {
            minD  = (float)distAbove;
            best  = target;
            state.snapAbove = true;
        }
    }
    return best;
}

}
