#include "Renderer.h"
#include "UIManager.h"
#include <iostream>
#include "Logger.h"
#include <cmath>

namespace Renderer {

SDL_Color getCategoryColor(BlockCategory cat) {
    switch (cat) {
        case CAT_MOTION:    return {76, 151, 255, 255};   // Blue
        case CAT_LOOKS:     return {153, 102, 255, 255};  // Purple
        case CAT_SOUND:     return {207, 99, 207, 255};   // Magenta
        case CAT_EVENTS:    return {255, 191, 0, 255};    // Yellow
        case CAT_CONTROL:   return {255, 171, 25, 255};   // Orange
        case CAT_SENSING:   return {92, 177, 214, 255};   // Cyan
        case CAT_OPERATORS: return {89, 203, 94, 255};    // Green
        case CAT_VARIABLES: return {255, 140, 26, 255};   // Orange-red
        case CAT_PEN:       return {15, 189, 140, 255};   // Dark green
        default:            return {200, 200, 200, 255};
    }
}

// Separate render functions for UI Manager integration
void renderPaletteBlocks(GameState& state) {
    // Palette visible region (below category tab bar)
    const int CAT_TAB_H = 22;
    const int SCROLLBAR_W = 8;
    const int clipX  = 0;
    const int clipY  = UILayout::MENU_H + CAT_TAB_H + 2;
    const int clipW  = UILayout::PAL_W - SCROLLBAR_W - 2;
    const int clipH  = state.windowHeight - UILayout::MENU_H - UILayout::SPR_H
                       - CAT_TAB_H - 4;

    // Enable SDL clip rect so blocks don't overdraw outside palette
    SDL_Rect clip = {clipX, clipY, clipW, clipH};
    SDL_RenderSetClipRect(state.renderer, &clip);

    // Category map: paletteCategory -1 = ALL, else matches BlockCategory enum
    for (Block* block : state.paletteBlocks) {
        // Category filter
        if (state.paletteCategory >= 0 &&
            (int)block->category != state.paletteCategory) continue;

        // Apply scroll: shift Y by -paletteScrollY relative to palette top
        int drawY = block->y - state.paletteScrollY;

        // Skip if completely outside visible area
        if (drawY + block->height < clipY) continue;
        if (drawY > clipY + clipH)         continue;

        // Temporarily move block Y for rendering
        int origY = block->y;
        block->y  = drawY;
        renderBlock(state, block, false);
        block->y  = origY;
    }

    // Disable clip
    SDL_RenderSetClipRect(state.renderer, nullptr);
}

void renderEditorBlocks(GameState& state) {
    // Render editor blocks (except dragged)
    for (Block* block : state.editorBlocks) {
        if (block != state.draggedBlock) {
            renderBlock(state, block, false);
        }
    }
}

void renderStageContent(GameState& state) {
    // Stage background (solid color)
    SDL_SetRenderDrawColor(state.renderer,
        state.stageColor.r, state.stageColor.g, state.stageColor.b, 255);
    SDL_Rect stage = {state.stageX, state.stageY, state.stageWidth, state.stageHeight};
    SDL_RenderFillRect(state.renderer, &stage);

    // Draw pen strokes
    for (const PenStroke& stroke : state.penStrokes) {
        if (stroke.points.size() < 2) continue;

        SDL_SetRenderDrawColor(state.renderer,
            stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a);

        for (size_t i = 1; i < stroke.points.size(); i++) {
            int x1 = state.stageX + state.stageWidth / 2 + stroke.points[i-1].x;
            int y1 = state.stageY + state.stageHeight / 2 - stroke.points[i-1].y;
            int x2 = state.stageX + state.stageWidth / 2 + stroke.points[i].x;
            int y2 = state.stageY + state.stageHeight / 2 - stroke.points[i].y;

            for (int t = -stroke.size/2; t <= stroke.size/2; t++) {
                SDL_RenderDrawLine(state.renderer, x1 + t, y1, x2 + t, y2);
                SDL_RenderDrawLine(state.renderer, x1, y1 + t, x2, y2 + t);
            }
        }
    }

    // Draw current stroke
    if (state.isDrawingStroke && state.currentStroke.points.size() >= 2) {
        SDL_SetRenderDrawColor(state.renderer,
            state.currentStroke.color.r, state.currentStroke.color.g,
            state.currentStroke.color.b, state.currentStroke.color.a);

        for (size_t i = 1; i < state.currentStroke.points.size(); i++) {
            int x1 = state.stageX + state.stageWidth / 2 + state.currentStroke.points[i-1].x;
            int y1 = state.stageY + state.stageHeight / 2 - state.currentStroke.points[i-1].y;
            int x2 = state.stageX + state.stageWidth / 2 + state.currentStroke.points[i].x;
            int y2 = state.stageY + state.stageHeight / 2 - state.currentStroke.points[i].y;

            for (int t = -state.currentStroke.size/2; t <= state.currentStroke.size/2; t++) {
                SDL_RenderDrawLine(state.renderer, x1 + t, y1, x2 + t, y2);
                SDL_RenderDrawLine(state.renderer, x1, y1 + t, x2, y2 + t);
            }
        }
    }

    // Render sprite
    if (state.selectedSpriteIndex >= 0 &&
        state.selectedSpriteIndex < (int)state.sprites.size()) {

        Sprite* sprite = state.sprites[state.selectedSpriteIndex];

        if (sprite->visible && !sprite->costumes.empty()) {
            Costume& costume = sprite->costumes[sprite->currentCostume];

            int screenX = state.stageX + state.stageWidth / 2 + (int)sprite->x;
            int screenY = state.stageY + state.stageHeight / 2 - (int)sprite->y;

            int w = (int)(costume.width * sprite->size / 100.0f);
            int h = (int)(costume.height * sprite->size / 100.0f);

            SDL_Rect dst = {screenX - w/2, screenY - h/2, w, h};

            double angle = sprite->direction - 90.0;
            if (sprite->ghostEffect > 0)
            {
                Uint8 alpha = (Uint8)(255*(1.0f - sprite->ghostEffect)/100.0f);
                SDL_SetTextureAlphaMod(costume.texture, alpha);
            }
            else
            {
                SDL_SetTextureAlphaMod(costume.texture, 255);
            }
            if (sprite->brightnessEffect > 0) {
                Uint8 bright = (Uint8)(255 * (1.0f - sprite->brightnessEffect/100.0f));
                SDL_SetTextureColorMod(costume.texture, bright,bright,bright);
            }
            else
            {
                SDL_SetTextureColorMod(costume.texture,255,255,255);
            }
            if (sprite->saturationEffect > 0)
                Logger::info("Saturation effect: " + std::to_string(sprite->saturationEffect));
            SDL_RenderCopyEx(state.renderer, costume.texture, nullptr, &dst,
                angle, nullptr, SDL_FLIP_NONE);
        }

        // Speech bubble
        if (!sprite->sayText.empty() && (sprite->sayTimer > 0.0f || sprite->sayTimer == -1.0f)) {
            int screenX = state.stageX + state.stageWidth / 2 + (int)sprite->x;
            int screenY = state.stageY + state.stageHeight / 2 - (int)sprite->y;

            int bubbleX = screenX + 40;
            int bubbleY = screenY - 50;
            int bubbleW = 150;
            int bubbleH = 40;

            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
            SDL_Rect bubble = {bubbleX, bubbleY, bubbleW, bubbleH};
            SDL_RenderFillRect(state.renderer, &bubble);

            SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(state.renderer, &bubble);

            if (true) {
                renderText(state, sprite->sayText, bubbleX + 5, bubbleY + 10, {0, 0, 0, 255});
            }
        }
    }
    renderVariableMonitor(state);
}
