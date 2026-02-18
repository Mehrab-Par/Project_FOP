#pragma once
#include "GameState.h"

// ─────────────────────────────────────────────
// Renderer — all SDL drawing
// ─────────────────────────────────────────────
namespace Renderer {
    void init(GameState& state);
    void shutdown(GameState& state);
    void renderFrame(GameState& state);
    // ── Sub-renderers ──
    void renderBackground(GameState& state);
    void renderStage(GameState& state);
    void renderBackdrop(GameState& state);
    void renderPenDrawings(GameState& state);
    void renderSprites(GameState& state);
    void renderSpeechBubbles(GameState& state);
    void renderUI(GameState& state);
    void renderSpritePanel(GameState& state);
    void renderCodeEditor(GameState& state);
    void renderBlockPalette(GameState& state);
    void renderVariableMonitors(GameState& state);
    void renderLogOverlay(GameState& state);
    void renderAskDialog(GameState& state);
    void renderPenExtensionBlocks(GameState& state);
    void renderExecutionControls(GameState& state);

    // ── Helpers ──
    void drawRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c);
    void drawFilledRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c);
    void drawText(SDL_Renderer* r, const std::string& text, int x, int y, SDL_Color c, int fontSize = 14);
    void drawLine(SDL_Renderer* r, int x1, int y1, int x2, int y2, SDL_Color c, int thickness = 1);
    void drawCircle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c);
    void drawFilledCircle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c);
}
