#pragma once
#include "GameState.h"

namespace Renderer {
    void render(GameState& state);
    void renderPaletteBlocks  (GameState& state);
    void renderEditorBlocks   (GameState& state);
    void renderStageContent   (GameState& state);
    void renderTopBar         (GameState& state);
    void renderPalette        (GameState& state);
    void renderEditor         (GameState& state);
    void renderStage          (GameState& state);
    void renderBlock          (GameState& state, Block* block, bool ghost = false);
    void renderAskDialog      (GameState& state);
    void renderVariableMonitor(GameState& state);
    void renderSnapPreview    (GameState& state);
    void renderExecutionCursor(GameState& state);

    SDL_Color getCategoryColor(BlockCategory cat);
    void renderText(GameState& state, const std::string& text,
                    int x, int y, SDL_Color color);
}