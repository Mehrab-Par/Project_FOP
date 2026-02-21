#include "Renderer.h"
#include "UIManager.h"
// NO SDL_ttf - uses pixel font from UIManager pattern
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

// Legacy render function kept for compatibility
void render(GameState& state) {
    // Clear with Scratch gray background
    SDL_SetRenderDrawColor(state.renderer, 235, 235, 235, 255);
    SDL_RenderClear(state.renderer);

    renderTopBar(state);
    renderPalette(state);
    renderEditor(state);
    renderStage(state);

    // Render dragged block (on top)
    if (state.draggedBlock) {
        renderBlock(state, state.draggedBlock, true);

        // Show snap preview
        if (state.snapTarget) {
            int snapY = state.snapAbove ?
                state.snapTarget->y - state.draggedBlock->height - 5 :
                state.snapTarget->y + state.snapTarget->height + 5;

            SDL_SetRenderDrawColor(state.renderer, 255, 255, 0, 180);
            SDL_Rect snapRect = {state.snapTarget->x - 2, snapY - 2,
                state.draggedBlock->width + 4, state.draggedBlock->height + 4};
            SDL_RenderDrawRect(state.renderer, &snapRect);
        }
    }

    SDL_RenderPresent(state.renderer);
}

void renderTopBar(GameState& state) {
    // Top bar background (dark)
    SDL_SetRenderDrawColor(state.renderer, 68, 68, 68, 255);
    SDL_Rect topBar = {0, 0, state.windowWidth, 40};
    SDL_RenderFillRect(state.renderer, &topBar);

    // Green flag button
    SDL_SetRenderDrawColor(state.renderer, 0, 200, 0, 255);
    SDL_Rect greenFlag = {state.windowWidth - 100, 8, 35, 24};
    SDL_RenderFillRect(state.renderer, &greenFlag);

    // Stop button
    SDL_SetRenderDrawColor(state.renderer, 200, 0, 0, 255);
    SDL_Rect stopBtn = {state.windowWidth - 50, 8, 35, 24};
    SDL_RenderFillRect(state.renderer, &stopBtn);

    // Background color button
    SDL_SetRenderDrawColor(state.renderer,
        state.stageColor.r, state.stageColor.g, state.stageColor.b, 255);
    SDL_Rect bgBtn = {10, 8, 85, 24};
    SDL_RenderFillRect(state.renderer, &bgBtn);
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(state.renderer, &bgBtn);

    if (true) {
        renderText(state, "Color", 15, 10, {0, 0, 0, 255});
    }
}

void renderPalette(GameState& state) {
    // Palette panel (light gray)
    SDL_SetRenderDrawColor(state.renderer, 248, 248, 248, 255);
    SDL_Rect palette = {0, 40, state.paletteWidth, state.windowHeight - 40};
    SDL_RenderFillRect(state.renderer, &palette);

    // Border
    SDL_SetRenderDrawColor(state.renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(state.renderer, &palette);

    // Title
    if (true) {
        renderText(state, "Block Palette", 10, 45, {0, 0, 0, 255});
    }

    // Render palette blocks
    for (Block* block : state.paletteBlocks) {
        renderBlock(state, block, false);
    }
}

void renderEditor(GameState& state) {
    // Editor panel (white)
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    SDL_Rect editor = {state.editorX, 40, state.stageX - state.editorX, state.windowHeight - 40};
    SDL_RenderFillRect(state.renderer, &editor);

    // Border
    SDL_SetRenderDrawColor(state.renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(state.renderer, &editor);

    // Title
    if (true) {
        renderText(state, "Code Editor", state.editorX + 10, 45, {0, 0, 0, 255});
        renderText(state, "(Drag blocks here)", state.editorX + 10, 65, {128, 128, 128, 255});
    }

    // Render editor blocks (except dragged)
    for (Block* block : state.editorBlocks) {
        if (block != state.draggedBlock) {
            renderBlock(state, block, false);
        }
    }
}

void renderStage(GameState& state) {
    // Stage border
    SDL_SetRenderDrawColor(state.renderer, 200, 200, 200, 255);
    SDL_Rect stageBorder = {state.stageX - 2, state.stageY - 2,
        state.stageWidth + 4, state.stageHeight + 4};
    SDL_RenderFillRect(state.renderer, &stageBorder);

    // Stage background (solid color)
    SDL_SetRenderDrawColor(state.renderer,
        state.stageColor.r, state.stageColor.g, state.stageColor.b, 255);
    SDL_Rect stage = {state.stageX, state.stageY, state.stageWidth, state.stageHeight};
    SDL_RenderFillRect(state.renderer, &stage);

    // ═══════════════════════════════════════════════
    // PEN DRAWING (FIXED!)
    // ═══════════════════════════════════════════════

    // Draw completed strokes
    for (const PenStroke& stroke : state.penStrokes) {
        if (stroke.points.size() < 2) continue;

        SDL_SetRenderDrawColor(state.renderer,
            stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a);

        for (size_t i = 1; i < stroke.points.size(); i++) {
            // Convert stage coordinates to screen coordinates
            int x1 = state.stageX + state.stageWidth / 2 + stroke.points[i-1].x;
            int y1 = state.stageY + state.stageHeight / 2 - stroke.points[i-1].y;
            int x2 = state.stageX + state.stageWidth / 2 + stroke.points[i].x;
            int y2 = state.stageY + state.stageHeight / 2 - stroke.points[i].y;

            // Draw thick line
            for (int t = -stroke.size/2; t <= stroke.size/2; t++) {
                SDL_RenderDrawLine(state.renderer, x1 + t, y1, x2 + t, y2);
                SDL_RenderDrawLine(state.renderer, x1, y1 + t, x2, y2 + t);
            }
        }
    }

    // Draw current stroke (being drawn)
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

            // Convert stage coords to screen coords
            int screenX = state.stageX + state.stageWidth / 2 + (int)sprite->x;
            int screenY = state.stageY + state.stageHeight / 2 - (int)sprite->y;

            int w = (int)(costume.width * sprite->size / 100.0f);
            int h = (int)(costume.height * sprite->size / 100.0f);

            SDL_Rect dst = {screenX - w/2, screenY - h/2, w, h};

            // Rotate sprite
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
            if (sprite->ghostEffect > 0) {
                Uint8 alpha = (Uint8)(255 * (1.0f - sprite->ghostEffect/100.0f));
                SDL_SetTextureAlphaMod(costume.texture, alpha);
            } else {
                SDL_SetTextureAlphaMod(costume.texture, 255);
            }
            if (sprite->brightnessEffect > 0) {
                Uint8 bright = (Uint8)(255 * (1.0f - sprite->brightnessEffect/100.0f));
                SDL_SetTextureAlphaMod(costume.texture, bright);
            }
            else
            {
                SDL_SetTextureAlphaMod(costume.texture,255);
            }

            SDL_RenderCopyEx(state.renderer, costume.texture, nullptr, &dst,
                angle, nullptr, SDL_FLIP_NONE);
        }

        // Speech bubble
        if (!sprite->sayText.empty() && sprite->sayTimer > 0.0f) {
            int screenX = state.stageX + state.stageWidth / 2 + (int)sprite->x;
            int screenY = state.stageY + state.stageHeight / 2 - (int)sprite->y;

            int bubbleX = screenX + 40;
            int bubbleY = screenY - 50;
            int bubbleW = 150;
            int bubbleH = 40;

            // Bubble background
            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
            SDL_Rect bubble = {bubbleX, bubbleY, bubbleW, bubbleH};
            SDL_RenderFillRect(state.renderer, &bubble);

            // Bubble border
            SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(state.renderer, &bubble);

            // Text
            if (true) {
                renderText(state, sprite->sayText, bubbleX + 5, bubbleY + 10, {0, 0, 0, 255});
            }
        }
    }
}

void renderBlock(GameState& state, Block* block, bool ghost) {
    SDL_Color color = getCategoryColor(block->category);

    if (ghost) {
        color.a = 180;
    }

    // Block body
    SDL_SetRenderDrawColor(state.renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {block->x, block->y, block->width, block->height};
    SDL_RenderFillRect(state.renderer, &rect);

    // Block border (darker) – clamp to avoid uint8 underflow
    Uint8 dr = (color.r > 50) ? color.r - 50 : 0;
    Uint8 dg = (color.g > 50) ? color.g - 50 : 0;
    Uint8 db = (color.b > 50) ? color.b - 50 : 0;
    SDL_SetRenderDrawColor(state.renderer, dr, dg, db, 255);
    SDL_RenderDrawRect(state.renderer, &rect);

    // Text
    if (true) {
        renderText(state, block->text, block->x + 8, block->y + 10, {255, 255, 255, 255});
    }
}

void renderText(GameState& state, const std::string& text, int x, int y, SDL_Color color) {
    // Pixel font - 5x7 glyphs, same table as UIManager
    static const unsigned char F[95][7]={
    {0,0,0,0,0,0,0},{0x04,0x04,0x04,0x04,0,0x04,0},{0x0A,0x0A,0,0,0,0,0},
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0},{0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
    {0,0,0,0,0,0,0},{0,0,0,0,0,0,0},{0x04,0x04,0,0,0,0,0},
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},{0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    {0,0x04,0x15,0x0E,0x15,0x04,0},{0,0x04,0x04,0x1F,0x04,0x04,0},
    {0,0,0,0,0x04,0x04,0x08},{0,0,0,0x1F,0,0,0},{0,0,0,0,0,0x04,0},
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10},
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0,0x04,0,0,0x04,0,0},{0,0x04,0,0,0x04,0x04,0x08},
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02},{0,0,0x1F,0,0x1F,0,0},
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08},{0x0E,0x11,0x01,0x02,0x04,0,0x04},
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},{0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},{0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},{0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x15,0x15,0x15,0x15,0x0A},{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},{0x10,0x10,0x08,0x04,0x02,0x01,0x01},
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E},{0x04,0x0A,0x11,0,0,0,0},
    {0,0,0,0,0,0,0x1F},{0x08,0x04,0,0,0,0,0},
    {0,0,0x0E,0x01,0x0F,0x11,0x0F},{0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
    {0,0,0x0E,0x10,0x10,0x10,0x0E},{0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
    {0,0,0x0E,0x11,0x1F,0x10,0x0E},{0x06,0x09,0x08,0x1C,0x08,0x08,0x08},
    {0,0,0x0F,0x11,0x0F,0x01,0x0E},{0x10,0x10,0x16,0x19,0x11,0x11,0x11},
    {0x04,0,0x0C,0x04,0x04,0x04,0x0E},{0x02,0,0x06,0x02,0x02,0x12,0x0C},
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12},{0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0,0,0x1A,0x15,0x15,0x11,0x11},{0,0,0x16,0x19,0x11,0x11,0x11},
    {0,0,0x0E,0x11,0x11,0x11,0x0E},{0,0,0x1E,0x11,0x1E,0x10,0x10},
    {0,0,0x0F,0x11,0x0F,0x01,0x01},{0,0,0x16,0x19,0x10,0x10,0x10},
    {0,0,0x0E,0x10,0x0E,0x01,0x1E},{0x08,0x08,0x1C,0x08,0x08,0x09,0x06},
    {0,0,0x11,0x11,0x11,0x13,0x0D},{0,0,0x11,0x11,0x11,0x0A,0x04},
    {0,0,0x11,0x15,0x15,0x15,0x0A},{0,0,0x11,0x0A,0x04,0x0A,0x11},
    {0,0,0x11,0x11,0x0F,0x01,0x0E},{0,0,0x1F,0x02,0x04,0x08,0x1F},
    {0x06,0x08,0x08,0x18,0x08,0x08,0x06},{0x04,0x04,0x04,0,0x04,0x04,0x04},
    {0x0C,0x02,0x02,0x03,0x02,0x02,0x0C},{0x08,0x15,0x02,0,0,0,0}
    };
    SDL_SetRenderDrawColor(state.renderer, color.r, color.g, color.b, color.a);
    int cx = x;
    for (char ch : text) {
        if (ch == ' ') { cx += 4; continue; }
        int idx = (unsigned char)ch - 32;
        if (idx < 0 || idx >= 95) { cx += 6; continue; }
        for (int row = 0; row < 7; row++) {
            unsigned char bits = F[idx][row];
            for (int col = 0; col < 5; col++) {
                if (bits & (0x10 >> col)) {
                    SDL_Rect px = {cx+col, y+row, 1, 1};
                    SDL_RenderFillRect(state.renderer, &px);
                }
            }
        }
        cx += 6;
    }
}

// ─── Ask/Answer overlay ────────────────────────────────────────────────────
void renderAskDialog(GameState& state) {
    if (!state.askActive) return;

    int W = state.windowWidth;
    int H = state.windowHeight;

    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 120);
    SDL_Rect overlay = {0, 0, W, H};
    SDL_RenderFillRect(state.renderer, &overlay);

    // Dialog box
    int dw = 480, dh = 120;
    int dx = (W - dw) / 2;
    int dy = (H - dh) / 2;

    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    SDL_Rect dlg = {dx, dy, dw, dh};
    SDL_RenderFillRect(state.renderer, &dlg);
    SDL_SetRenderDrawColor(state.renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(state.renderer, &dlg);

    // Question text
    renderText(state, state.askQuestion, dx + 12, dy + 14, {30, 30, 30, 255});

    // Input box
    int ibY = dy + 50;
    SDL_SetRenderDrawColor(state.renderer, 240, 240, 240, 255);
    SDL_Rect ib = {dx + 10, ibY, dw - 20, 30};
    SDL_RenderFillRect(state.renderer, &ib);
    SDL_SetRenderDrawColor(state.renderer, 120, 120, 120, 255);
    SDL_RenderDrawRect(state.renderer, &ib);

    renderText(state, state.askInput + "_", dx + 16, ibY + 10, {10, 10, 10, 255});

    // Hint
    renderText(state, "Press ENTER to confirm", dx + 12, dy + 92, {100, 100, 100, 255});
}

// ─── Variable monitor display ─────────────────────────────────────────────
void renderVariableMonitor(GameState& state) {
    if (state.variables.empty()) return;
    int vy = state.stageY + state.stageHeight + 10;
    int vx = state.stageX;
    for (auto& kv : state.variables) {
        if (state.variableVisible.count(kv.first) &&
            !state.variableVisible.at(kv.first)) continue;

        SDL_SetRenderDrawColor(state.renderer, 200, 100, 30, 220);
        SDL_Rect r = {vx, vy, 160, 20};
        SDL_RenderFillRect(state.renderer, &r);
        renderText(state, kv.first + ": " + kv.second, vx + 4, vy + 6, {255,255,255,255});
        vy += 24;
    }
}

// ─── Snap preview highlight ────────────────────────────────────────────────
void renderSnapPreview(GameState& state) {
    if (!state.snapTarget || !state.draggedBlock) return;
    int snapY = state.snapAbove
        ? state.snapTarget->y - state.draggedBlock->height - 4
        : state.snapTarget->y + state.snapTarget->height + 4;

    SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 0, 160);
    SDL_Rect sr = {state.snapTarget->x - 2, snapY - 2,
                   state.draggedBlock->width + 4, state.draggedBlock->height + 4};
    SDL_RenderDrawRect(state.renderer, &sr);
}

// ─── Execution cursor (step mode) ─────────────────────────────────────────
void renderExecutionCursor(GameState& state) {
    if (!state.exec.running && !state.exec.paused) return;
    if (state.selectedSpriteIndex < 0 ||
        state.selectedSpriteIndex >= (int)state.sprites.size()) return;

    Sprite* sp = state.sprites[state.selectedSpriteIndex];
    auto it = state.exec.ctx.find(sp);
    if (it == state.exec.ctx.end()) return;
    int pc = it->second.pc;

    if (pc >= 0 && pc < (int)state.editorBlocks.size()) {
        Block* cur = state.editorBlocks[pc];
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 0, 200);
        SDL_Rect cursor = {cur->x - 4, cur->y - 4, cur->width + 8, cur->height + 8};
        SDL_RenderDrawRect(state.renderer, &cursor);
    }
}

} // namespace Renderer
