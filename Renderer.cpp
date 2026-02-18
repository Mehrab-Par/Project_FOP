#include "Renderer.h"
#include "SDL2/SDL_ttf.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// ------------------------------------------------------
// Simple font rendering (pixel-based fallback)
// We use SDL_ttf if available; here we do a
// simple blit-based approach with solid color rects
// representing text for portability.
// In production, link SDL_ttf and use TTF_RenderText.
// ------------------------------------------------------

static const int CHAR_W = 7;
static const int CHAR_H = 12;

// Block palette layout constants

static const int PALETTE_X       = 5;
static const int PALETTE_Y       = 10;
static const int PALETTE_W       = 250;
static const int BLOCK_H         = 28;
static const int BLOCK_PAD       = 4;
static const int CATEGORY_H      = 24;


// Color palette

static SDL_Color COL_BG          = {30,  30,  40,  255};
static SDL_Color COL_STAGE_BG    = {255,255,255,255};
static SDL_Color COL_PANEL_BG    = {45,  45,  58,  255};
static SDL_Color COL_PANEL_BORDER= {80,  80, 100,  255};
static SDL_Color COL_MOTION      = {72, 132, 219,  255};  // blue
static SDL_Color COL_LOOKS       = {100,100,215,  255};  // indigo
static SDL_Color COL_SOUND       = {170, 90, 200,  255};  // purple
static SDL_Color COL_EVENTS      = {255,200,  0,  255};  // yellow
static SDL_Color COL_CONTROL     = {255,150,  0,  255};  // orange
static SDL_Color COL_SENSING     = { 80,195,215,  255};  // cyan
static SDL_Color COL_OPERATORS   = { 90,195, 90,  255};  // green
static SDL_Color COL_VARIABLES   = {230,130, 60,  255};  // dark orange
static SDL_Color COL_FUNCTIONS   = {255,100,150,  255};  // pink
static SDL_Color COL_PEN         = { 50,180, 80,  255};  // dark green
static SDL_Color COL_TEXT        = {230,230,235,  255};
static SDL_Color COL_TEXT_DIM    = {140,140,155,  255};
static SDL_Color COL_PLAY        = { 50,200, 80,  255};
static SDL_Color COL_STOP        = {210, 60, 60,  255};
static SDL_Color COL_PAUSE       = {200,180, 50,  255};

namespace Renderer {
    // INIT / SHUTDOWN


    void init(GameState& state) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }
        IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
        Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG);
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

        TTF_Init();
        state.window = SDL_CreateWindow("Scratch Clone - C++ SDL2",
                                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         state.windowWidth, state.windowHeight,
                                         SDL_WINDOW_SHOWN);
        state.renderer = SDL_CreateRenderer(state.window, -1,
                                             SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!state.renderer) {
            std::cerr << "Renderer failed: " << SDL_GetError() << std::endl;
        }
        srand((unsigned)SDL_GetTicks());
    }

    void shutdown(GameState& state) {
        if (state.renderer) SDL_DestroyRenderer(state.renderer);
        if (state.window)   SDL_DestroyWindow(state.window);
        TTF_Quit();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
    }


    // MAIN RENDER


    void renderFrame(GameState& state) {
        SDL_SetRenderDrawColor(state.renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
        SDL_RenderClear(state.renderer);

        renderBackground(state);
        renderStage(state);
        renderBackdrop(state);
        renderPenDrawings(state);
        renderSprites(state);
        renderSpeechBubbles(state);

        // UI
        renderBlockPalette(state);
        renderCodeEditor(state);
        renderSpritePanel(state);
        renderExecutionControls(state);

        if (state.showVariableMonitors)
            renderVariableMonitors(state);
        if (state.showLogs)
            renderLogOverlay(state);
        if (state.waitingForAnswer)
            renderAskDialog(state);

        SDL_RenderPresent(state.renderer);
    }

    // BACKGROUND
    void renderBackground(GameState& state) {
        // Already cleared with COL_BG
    }

    //
    // STAGE
    //

    void renderStage(GameState& state) {
        // Stage white area
        drawFilledRect(state.renderer,
                       state.stageX, state.stageY,
                       state.stageWidth, state.stageHeight,
                       COL_STAGE_BG);
        // Stage border
        drawRect(state.renderer,
                 state.stageX, state.stageY,
                 state.stageWidth, state.stageHeight,
                 COL_PANEL_BORDER);

        // Label
        drawText(state.renderer, "Stage", state.stageX, state.stageY - 14, COL_TEXT_DIM, 11);
    }


    // BACKDROP


    void renderBackdrop(GameState& state) {
        if (state.backdrops.empty()) return;
        auto& bd = state.backdrops[state.currentBackdrop];
        if (bd.texture) {
            SDL_Rect dst = {state.stageX, state.stageY, state.stageWidth, state.stageHeight};
            SDL_RenderCopy(state.renderer, bd.texture, nullptr, &dst);
        }
    }


    // PEN DRAWINGS


    void renderPenDrawings(GameState& state) {
        // Stamps first (behind sprites)
        for (auto& stamp : state.penStamps) {
            if (!stamp.texture) continue;
            int w = (int)(64 * stamp.size / 100.0f);
            int h = (int)(64 * stamp.size / 100.0f);
            SDL_Rect dst = {
                state.stageX + (int)stamp.x - w/2,
                state.stageY + (int)stamp.y - h/2,
                w, h
            };
            SDL_RenderCopy(state.renderer, stamp.texture, nullptr, &dst);
        }

        // Pen strokes
        for (auto& stroke : state.penStrokes) {
            SDL_SetRenderDrawColor(state.renderer, stroke.color.r, stroke.color.g, stroke.color.b, 255);
            SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);

            int x1 = state.stageX + (int)stroke.x1;
            int y1 = state.stageY + (int)stroke.y1;
            int x2 = state.stageX + (int)stroke.x2;
            int y2 = state.stageY + (int)stroke.y2;

            // Draw thick line by drawing multiple lines offset
            for (int i = 0; i < stroke.size; i++) {
                int offset = i - stroke.size / 2;
                SDL_RenderDrawLine(state.renderer, x1, y1 + offset, x2, y2 + offset);
                SDL_RenderDrawLine(state.renderer, x1 + offset, y1, x2 + offset, y2);
            }
        }
    }


    // SPRITES

    void renderSprites(GameState& state) {
        // Sort by layer
        std::vector<Sprite*> sorted = state.sprites;
        std::sort(sorted.begin(), sorted.end(),
                  [](const Sprite* a, const Sprite* b) { return a->layer < b->layer; });

        for (auto* sprite : sorted) {
            if (!sprite->visible) continue;

            int w = (int)(64 * sprite->size / 100.0f);
            int h = (int)(64 * sprite->size / 100.0f);
            int sx = state.stageX + (int)sprite->x - w / 2;
            int sy = state.stageY + (int)sprite->y - h / 2;

            if (!sprite->costumes.empty() && sprite->costumes[sprite->currentCostume].texture) {
                SDL_Rect dst = {sx, sy, w, h};
                double angle = sprite->direction - 90.0;
                SDL_Point center = {w / 2, h / 2};
                SDL_RenderCopyEx(state.renderer,
                                 sprite->costumes[sprite->currentCostume].texture,
                                 nullptr, &dst, angle, &center, SDL_FLIP_NONE);
            } else {
                // Draw a colored placeholder circle
                SDL_Color col = {(Uint8)(100 + (sprite - state.sprites[0]) * 50 % 156),
                                 (Uint8)(150 + (sprite - state.sprites[0]) * 70 % 106),
                                 (Uint8)(80  + (sprite - state.sprites[0]) * 90 % 176), 255};
                int cx = sx + w / 2;
                int cy = sy + h / 2;
                drawFilledCircle(state.renderer, cx, cy, w / 2, col);

                // Draw name
                drawText(state.renderer, sprite->name, sx, sy + h + 2, COL_TEXT_DIM, 10);
            }

            // Active sprite highlight
            if (state.activeSpriteIndex >= 0 && state.sprites[state.activeSpriteIndex] == sprite) {
                SDL_Color highlight = {255, 255, 0, 255};
                drawRect(state.renderer, sx, sy, w, h, highlight);
            }
        }
    }


    // SPEECH BUBBLES


    void renderSpeechBubbles(GameState& state) {
        for (auto* sprite : state.sprites) {
            if (!sprite->visible || sprite->speechText.empty()) continue;

            int w = (int)sprite->speechText.size() * CHAR_W + 24;
            int h = 28;
            int bx = state.stageX + (int)sprite->x + 20;
            int by = state.stageY + (int)sprite->y - h - 10;

            // Clamp bubble to stage
            if (bx + w > state.stageX + state.stageWidth) bx = state.stageX + state.stageWidth - w - 4;
            if (by < state.stageY) by = state.stageY + 4;

            SDL_Color bubbleBg = sprite->isThinker ? SDL_Color{240,240,245,255} : SDL_Color{255,255,255,255};
            SDL_Color bubbleBorder = {180,180,190,255};

            drawFilledRect(state.renderer, bx, by, w, h, bubbleBg);
            drawRect(state.renderer, bx, by, w, h, bubbleBorder);

            // Tail
            if (sprite->isThinker) {
                // Thought dots
                drawFilledCircle(state.renderer, bx - 4, by + h + 2, 3, bubbleBg);
                drawFilledCircle(state.renderer, bx - 10, by + h + 8, 5, bubbleBg);
            } else {
                // Speech tail (triangle-ish)
                SDL_SetRenderDrawColor(state.renderer, bubbleBorder.r, bubbleBorder.g, bubbleBorder.b, 255);
                SDL_RenderDrawLine(state.renderer, bx + 10, by + h, bx + 5, by + h + 8);
                SDL_RenderDrawLine(state.renderer, bx + 5,  by + h + 8, bx + 20, by + h);
            }

            // Text
            drawText(state.renderer, sprite->speechText, bx + 10, by + 7, SDL_Color{30,30,30,255}, 12);
        }
    }


    // BLOCK PALETTE (left sidebar)


    struct PaletteCategory {
        std::string name;
        SDL_Color color;
        std::vector<std::string> blocks;
    };

    static std::vector<PaletteCategory> getCategories(GameState& state) {
        std::vector<PaletteCategory> cats;

        cats.push_back({"Motion", COL_MOTION, {
            "move steps", "turn left", "turn right", "go to x y",
            "change x", "change y", "set x", "set y",
            "point direction", "go to random", "go to mouse", "bounce"
        }});

        cats.push_back({"Looks", COL_LOOKS, {
            "say", "say for", "think", "think for",
            "switch costume", "next costume",
            "switch backdrop", "next backdrop",
            "change size", "set size", "show", "hide",
            "front layer", "back layer"
        }});

        cats.push_back({"Sound", COL_SOUND, {
            "play sound", "play until done", "stop all sounds",
            "set volume", "change volume"
        }});

        cats.push_back({"Events", COL_EVENTS, {
            "green flag", "key pressed", "sprite clicked",
            "broadcast", "when broadcast"
        }});

        cats.push_back({"Control", COL_CONTROL, {
            "wait", "repeat", "forever",
            "if then", "if then else",
            "wait until", "repeat until", "stop all"
        }});

        cats.push_back({"Sensing", COL_SENSING, {
            "touching mouse?", "touching edge?",
            "distance to mouse", "ask and wait",
            "key pressed?", "mouse down?",
            "mouse x", "mouse y", "timer", "reset timer"
        }});

        cats.push_back({"Operators", COL_OPERATORS, {
            "+ add", "- subtract", "× multiply", "÷ divide",
            "= equal", "< less", "> greater",
            "AND", "OR", "NOT",
            "string length", "string at", "string join"
        }});

        cats.push_back({"Variables", COL_VARIABLES, {
            "set variable", "change variable"
        }});

        cats.push_back({"Functions", COL_FUNCTIONS, {
            "define function", "call function"
        }});

        if (state.penExtensionEnabled) {
            cats.push_back({"Pen", COL_PEN, {
                "pen down", "pen up", "stamp", "erase all",
                "set pen color", "set pen size", "change pen size"
            }});
        }

        return cats;
    }

    static int paletteScrollY = 0;

    void renderBlockPalette(GameState& state) {
        int panelW = 250;
        int panelH = state.windowHeight - 60;

        // Panel background
        drawFilledRect(state.renderer, 0, 0, panelW, state.windowHeight, COL_PANEL_BG);
        drawRect(state.renderer, 0, 0, panelW, state.windowHeight, COL_PANEL_BORDER);

        // Title
        drawText(state.renderer, "Blocks", 10, 6, COL_TEXT, 14);

        auto cats = getCategories(state);
        int y = 30 + paletteScrollY;

        for (auto& cat : cats) {
            // Category header
            drawFilledRect(state.renderer, PALETTE_X, y, PALETTE_W - 10, CATEGORY_H, cat.color);
            drawText(state.renderer, cat.name, PALETTE_X + 8, y + 5, COL_TEXT, 12);
            y += CATEGORY_H + 2;

            // Blocks
            for (auto& blockName : cat.blocks) {
                SDL_Color darker = {(Uint8)(cat.color.r * 0.8), (Uint8)(cat.color.g * 0.8),
                                    (Uint8)(cat.color.b * 0.8), 255};
                drawFilledRect(state.renderer, PALETTE_X + 4, y, PALETTE_W - 18, BLOCK_H, darker);
                drawRect(state.renderer, PALETTE_X + 4, y, PALETTE_W - 18, BLOCK_H, cat.color);
                drawText(state.renderer, blockName, PALETTE_X + 12, y + 7, COL_TEXT, 11);
                y += BLOCK_H + BLOCK_PAD;
            }
            y += 4; // gap between categories
        }
    }


    // CODE EDITOR (middle area, right of palette)


    void renderCodeEditor(GameState& state) {
        int edX = 255;
        int edY = state.stageY + state.stageHeight + 10;
        int edW = state.stageWidth + 10;
        int edH = state.windowHeight - edY - 5;

        drawFilledRect(state.renderer, edX, edY, edW, edH, COL_PANEL_BG);
        drawRect(state.renderer, edX, edY, edW, edH, COL_PANEL_BORDER);
        drawText(state.renderer, "Code Editor", edX + 8, edY + 4, COL_TEXT_DIM, 11);

        // Render placed blocks
        int blockY = edY + 24;
        for (size_t i = 0; i < state.editorBlocks.size(); i++) {
            Block* b = state.editorBlocks[i];
            if (!b) continue;

            // Determine color by type
            SDL_Color col = COL_MOTION;
            if (b->type >= BlockType::Say && b->type <= BlockType::ClearGraphicEffects) col = COL_LOOKS;
            else if (b->type >= BlockType::PlaySound && b->type <= BlockType::ChangeVolume) col = COL_SOUND;
            else if (b->type >= BlockType::WhenGreenFlagClicked && b->type <= BlockType::Broadcast) col = COL_EVENTS;
            else if (b->type >= BlockType::Wait && b->type <= BlockType::Else) col = COL_CONTROL;
            else if (b->type >= BlockType::TouchingMouse && b->type <= BlockType::ResetTimer) col = COL_SENSING;
            else if (b->type >= BlockType::Add && b->type <= BlockType::StringJoin) col = COL_OPERATORS;
            else if (b->type >= BlockType::SetVariable && b->type <= BlockType::HideVariable) col = COL_VARIABLES;
            else if (b->type >= BlockType::DefineFunction && b->type <= BlockType::CallFunction) col = COL_FUNCTIONS;
            else if (b->type >= BlockType::PenUp && b->type <= BlockType::ChangePenSize) col = COL_PEN;

            int indent = 0;
            // Simple indentation for control structures
            // (simplified: just show flat list with color)

            int bx = edX + 8 + indent;
            int by = blockY;
            int bw = edW - 20;
            int bh = 24;

            // Highlight current PC block
            bool isCurrent = state.execCtx.isRunning &&
                             (int)i == (state.execCtx.pc - 1);
            if (isCurrent) {
                drawFilledRect(state.renderer, bx - 2, by - 1, bw + 4, bh + 2,
                               SDL_Color{255, 255, 0, 60});
            }

            SDL_Color darker = {(Uint8)(col.r * 0.75f), (Uint8)(col.g * 0.75f),
                                (Uint8)(col.b * 0.75f), 255};
            drawFilledRect(state.renderer, bx, by, bw, bh, darker);
            drawRect(state.renderer, bx, by, bw, bh, col);

            // Line number
            drawText(state.renderer, std::to_string(i), bx + 4, by + 5, COL_TEXT_DIM, 10);

            // Block name (simplified)
            std::string label = "Block_" + std::to_string((int)b->type);
            if (!b->name.empty()) label = b->name;
            drawText(state.renderer, label, bx + 28, by + 5, COL_TEXT, 11);

            blockY += bh + 3;
            if (blockY > edY + edH - 4) break; // clip
        }
    }


    // SPRITE PANEL (bottom right)


    void renderSpritePanel(GameState& state) {
        int spX = state.stageX + state.stageWidth + 5;
        int spY = state.stageY;
        int spW = state.windowWidth - spX - 5;
        int spH = state.stageHeight;

        drawFilledRect(state.renderer, spX, spY, spW, spH, COL_PANEL_BG);
        drawRect(state.renderer, spX, spY, spW, spH, COL_PANEL_BORDER);
        drawText(state.renderer, "Sprites", spX + 8, spY + 4, COL_TEXT, 13);

        int iconSize = 52;
        int cols = std::max(1, spW / (iconSize + 8));
        int row = 0, col = 0;

        for (int i = 0; i < (int)state.sprites.size(); i++) {
            Sprite* sprite = state.sprites[i];
            int ix = spX + 8 + col * (iconSize + 8);
            int iy = spY + 28 + row * (iconSize + 20);

            if (iy + iconSize + 16 > spY + spH) break;

            // Background
            SDL_Color bg = (i == state.activeSpriteIndex)
                           ? SDL_Color{60,60,80,255} : SDL_Color{38,38,50,255};
            drawFilledRect(state.renderer, ix, iy, iconSize, iconSize + 16, bg);
            drawRect(state.renderer, ix, iy, iconSize, iconSize + 16,
                     (i == state.activeSpriteIndex) ? SDL_Color{255,255,0,255} : COL_PANEL_BORDER);

            // Sprite thumbnail (placeholder circle)
            SDL_Color thumbCol = {(Uint8)(80 + i * 50 % 156), (Uint8)(130 + i * 70 % 126),
                                  (Uint8)(60 + i * 90 % 196), 255};
            drawFilledCircle(state.renderer, ix + iconSize/2, iy + iconSize/2, iconSize/2 - 4, thumbCol);

            // Name
            std::string shortName = sprite->name;
            if (shortName.size() > 7) shortName = shortName.substr(0, 6) + "..";
            drawText(state.renderer, shortName, ix + 2, iy + iconSize + 2, COL_TEXT_DIM, 9);

            col++;
            if (col >= cols) { col = 0; row++; }
        }

        // "+" button to add sprite
        int addBtnX = spX + 8;
        int addBtnY = spY + spH - 28;
        drawFilledRect(state.renderer, addBtnX, addBtnY, 60, 22, SDL_Color{50,180,80,255});
        drawText(state.renderer, "+ Add", addBtnX + 8, addBtnY + 4, COL_TEXT, 11);
    }

    // EXECUTION CONTROLS


    void renderExecutionControls(GameState& state) {
        int ctrlX = state.stageX;
        int ctrlY = state.stageY + state.stageHeight + 4;
        int btnW  = 70, btnH = 24;
        int gap   = 6;

        // Play
        SDL_Color playCol = state.execCtx.isRunning ? SDL_Color{40,150,60,255} : COL_PLAY;
        drawFilledRect(state.renderer, ctrlX, ctrlY, btnW, btnH, playCol);
        drawRect(state.renderer, ctrlX, ctrlY, btnW, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, state.execCtx.isRunning ? "▶ Run" : "▶ Run",
                 ctrlX + 16, ctrlY + 5, COL_TEXT, 12);

        // Pause
        drawFilledRect(state.renderer, ctrlX + btnW + gap, ctrlY, btnW, btnH, COL_PAUSE);
        drawRect(state.renderer, ctrlX + btnW + gap, ctrlY, btnW, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, state.execCtx.isPaused ? "⏩ Resume" : "⏸ Pause",
                 ctrlX + btnW + gap + 8, ctrlY + 5, COL_TEXT, 11);

        // Stop
        drawFilledRect(state.renderer, ctrlX + 2*(btnW + gap), ctrlY, btnW, btnH, COL_STOP);
        drawRect(state.renderer, ctrlX + 2*(btnW + gap), ctrlY, btnW, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, "■ Stop", ctrlX + 2*(btnW + gap) + 14, ctrlY + 5, COL_TEXT, 12);

        // Debug toggle
        int dbgX = ctrlX + 3*(btnW + gap);
        SDL_Color dbgCol = state.execCtx.isDebugMode ? SDL_Color{180,100,220,255} : SDL_Color{100,60,140,255};
        drawFilledRect(state.renderer, dbgX, ctrlY, 80, btnH, dbgCol);
        drawRect(state.renderer, dbgX, ctrlY, 80, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, "🐛 Debug", dbgX + 10, ctrlY + 5, COL_TEXT, 11);

        // Log toggle
        int logX = dbgX + 86;
        SDL_Color logCol = state.showLogs ? SDL_Color{60,140,180,255} : SDL_Color{40,80,110,255};
        drawFilledRect(state.renderer, logX, ctrlY, 55, btnH, logCol);
        drawRect(state.renderer, logX, ctrlY, 55, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, "📋 Log", logX + 8, ctrlY + 5, COL_TEXT, 11);

        // Pen extension toggle
        int penX = logX + 61;
        SDL_Color penCol = state.penExtensionEnabled ? COL_PEN : SDL_Color{35,100,55,255};
        drawFilledRect(state.renderer, penX, ctrlY, 55, btnH, penCol);
        drawRect(state.renderer, penX, ctrlY, 55, btnH, SDL_Color{255,255,255,120});
        drawText(state.renderer, "✏ Pen", penX + 10, ctrlY + 5, COL_TEXT, 11);
    }
    // ═══════════════════════════════════════════════
    // DRAWING HELPERS
    // ═══════════════════════════════════════════════

    void drawFilledRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(r, &rect);
    }

    void drawRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderDrawRect(r, &rect);
    }

    void drawText(SDL_Renderer* r, const std::string& text, int x, int y, SDL_Color c, int fontSize) {
        TTF_Font* font = TTF_OpenFont("comic.ttf", fontSize);
        if (!font)
        {
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

            int charW = (fontSize <= 10) ? 6 : (fontSize <= 12) ? 7 : 8;
            int charH = fontSize;

            for (size_t i = 0; i < text.size(); i++) {
                int cx = x + (int)i * charW;
                // Draw a tiny glyph approximation (rectangle per character)
                SDL_Rect rect = {cx, y, charW - 1, charH};
                // We just fill a small rect with slight variation to simulate text
                SDL_Rect glyph = {cx + 1, y + 1, charW - 2, charH - 2};
                SDL_RenderFillRect(r, &glyph);
            }
            return;
        }

        SDL_Surface* surf = TTF_RenderUTF8_Blended(font,text.c_str(),c);
        if (surf)
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r,surf);
            SDL_Rect dst = {x,y,surf->w,surf->h};
            SDL_RenderCopy(r,tex,NULL,&dst);

            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }
        TTF_CloseFont(font);
    }

    void drawLine(SDL_Renderer* r, int x1, int y1, int x2, int y2, SDL_Color c, int thickness) {
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < thickness; i++) {
            SDL_RenderDrawLine(r, x1, y1 + i, x2, y2 + i);
        }
    }

    void drawCircle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c) {
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < 360; i++) {
            float rad = i * 3.14159265f / 180.0f;
            int px = cx + (int)(radius * cosf(rad));
            int py = cy + (int)(radius * sinf(rad));
            SDL_RenderDrawPoint(r, px, py);
        }
    }

    void drawFilledCircle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c) {
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int dy = -radius; dy <= radius; dy++) {
            int dx = (int)sqrtf((float)(radius * radius - dy * dy));
            SDL_Rect line = {cx - dx, cy + dy, 2 * dx, 1};
            SDL_RenderFillRect(r, &line);
        }
    }
} // namespace Renderer
