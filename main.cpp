#include "GameState.h"
#include "Engine.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "SpriteGenerator.h"
#include "Logger.h"
#include "SaveLoad.h"
#include "UIManager.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>

// Forward declarations
bool  initSDL   (GameState& state);
bool  loadAssets(GameState& state);
void  initPalette(GameState& state);
void  gameLoop  (GameState& state, UIManager& ui);

// ─── Entry point ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    Logger::init("scratch.log");
    Logger::info("=== Scratch Clone Starting ===");

    GameState  state;


    UIManager  ui;

    if (!initSDL(state)) {
        std::cerr << "Failed to initialize SDL!" << std::endl;
        return 1;
    }

    ui.init(state.windowWidth, state.windowHeight);

    if (!loadAssets(state)) {
        Logger::warning("Some assets were not loaded");
    }

    initPalette(state);

    ui.addLog("Scratch Clone ready!", "INFO");
    ui.addLog("Drag blocks from palette -> editor", "INFO");
    ui.addLog("Press SPACE to run, S = step mode", "INFO");

    gameLoop(state, ui);

    // Cleanup
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    Logger::close();
    return 0;
}

// ─── SDL init ─────────────────────────────────────────────────────────────────
bool initSDL(GameState& state) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        Logger::warning("IMG_Init warning: " + std::string(IMG_GetError()));
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        Logger::warning("Mix_OpenAudio failed: " + std::string(SDL_GetError()));
    }

    state.window = SDL_CreateWindow(
        "Scratch Clone \xe2\x80\x94 C++/SDL2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        state.windowWidth, state.windowHeight,
        SDL_WINDOW_SHOWN);

    if (!state.window) {
        std::cerr << "Window failed: " << SDL_GetError() << std::endl;
        return false;
    }

    state.renderer = SDL_CreateRenderer(state.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!state.renderer) {
        std::cerr << "Renderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);

    // Enable text input for Ask dialog
    SDL_StopTextInput(); // will be started when ask is active
    return true;
}

// ─── Load / generate assets ───────────────────────────────────────────────────
bool loadAssets(GameState& state) {
    SpriteGen::generateAllSprites(state.renderer);


    for (auto* s : state.sprites) delete s;
    state.sprites.clear();



    Sprite* cat = new Sprite();
    cat->name = "Cat1";
    cat->x = 0;
    cat->y = 0;

    SDL_Surface* catSurface = IMG_Load("assets/cat.png");
    if (catSurface) {
        Costume c;
        c.name = "cat";

        float originalW = catSurface->w;
        float originalH = catSurface->h;
        float targetW = 120;
        float scale = targetW / originalW;

        c.width = (int)targetW;
        c.height = (int)(originalH * scale);

        c.texture = SDL_CreateTextureFromSurface(state.renderer, catSurface);
        SDL_FreeSurface(catSurface);

        cat->costumes.push_back(c);
        Logger::info("Cat sprite loaded! Size: " + std::to_string(c.width) + "x" + std::to_string(c.height));
    } else {
        Logger::warning("Failed to load cat.png");
    }
    state.sprites.push_back(cat);
    // ============================


    Sprite* shapes = new Sprite();
    shapes->name = "Shape1";
    shapes->x = 0;
    shapes->y = 0;

    const char* names[] = {"circle","square","triangle","star",
                           "hexagon","pentagon","diamond","arrow"};

    for (auto* nm : names) {
        SDL_Texture* tex = SpriteGen::createTextureFor(state.renderer, nm);
        if (tex) {
            Costume c;
            c.name = nm;
            c.texture = tex;
            c.width = 80;
            c.height = 80;
            shapes->costumes.push_back(c);
        }
    }

    state.sprites.push_back(shapes);
    // ================================

    return true;
}

// ─── Build palette (block menu) ───────────────────────────────────────────────
void initPalette(GameState& state) {
    int x = 12, y = 60;
    const int spacing = 42;

    auto add = [&](BlockType t, BlockCategory c, const char* txt,
                   double num = 0, const char* str = "") {
        Block* b = new Block();
        b->type        = t;
        b->category    = c;
        b->text        = txt;
        b->numberValue = num;
        b->stringValue = str;
        b->x = x; b->y = y;
        b->width  = 185;
        b->height = 36;
        state.paletteBlocks.push_back(b);
        y += spacing;
    };

    // ── MOTION (blue) ──────────────────────────────────────────────────────
    add(BLOCK_Move,              CAT_MOTION,    "move 10 steps",       10);
    add(BLOCK_TurnRight,         CAT_MOTION,    "turn right 15 deg",   15);
    add(BLOCK_TurnLeft,          CAT_MOTION,    "turn left 15 deg",    15);
    add(BLOCK_GoToXY,            CAT_MOTION,    "go to x:0 y:0",        0);
    add(BLOCK_SetX,              CAT_MOTION,    "set x to 0",           0);
    add(BLOCK_SetY,              CAT_MOTION,    "set y to 0",           0);
    add(BLOCK_ChangeX,           CAT_MOTION,    "change x by 10",      10);
    add(BLOCK_ChangeY,           CAT_MOTION,    "change y by 10",      10);
    add(BLOCK_PointDirection,    CAT_MOTION,    "point in dir 90",     90);
    add(BLOCK_BounceOffEdge,     CAT_MOTION,    "if on edge bounce",    0);
    add(BLOCK_GoToMousePointer,  CAT_MOTION,    "go to mouse pointer",  0);
    add(BLOCK_GoToRandomPosition,CAT_MOTION,    "go to random pos",     0);

    // ── LOOKS (purple) ─────────────────────────────────────────────────────
    y = 60; x = 12; // reset for category view (all shown together here)
    // (in full UI we'd filter by category; for now just append)
    y = state.paletteBlocks.back()->y + spacing;

    add(BLOCK_Say,               CAT_LOOKS,     "say Hello!",           0, "Hello!");
    add(BLOCK_Say,               CAT_LOOKS,     "say",                  0, "");
    add(BLOCK_SayForSecs,        CAT_LOOKS,     "say Hello! 2 secs",    2, "Hello!");
    add(BLOCK_Think,             CAT_LOOKS,     "think Hmm...",         0, "Hmm...");
    add(BLOCK_Show,              CAT_LOOKS,     "show",                  0);
    add(BLOCK_Hide,              CAT_LOOKS,     "hide",                  0);
    add(BLOCK_NextCostume,       CAT_LOOKS,     "next costume",          0);
    add(BLOCK_SetSize,           CAT_LOOKS,     "set size to 100%",    100);
    add(BLOCK_ChangeSize,        CAT_LOOKS,     "change size by 10",    10);
    add(BLOCK_ClearGraphicEffects,CAT_LOOKS,    "clear graphic effects", 0);
    add(BLOCK_SetGhostEffect     ,CAT_LOOKS,    "set ghost effect to 50",50);
    add(BLOCK_SetGhostEffect     ,CAT_LOOKS,    "set ghost effect to 0",0);
    add(BLOCK_ChangeGhostEffect,  CAT_LOOKS,      "change ghost effect by 10",10);
    add(BLOCK_SetBrightnessEffect,      CAT_LOOKS,    "set brightness to 50",    50);
    add(BLOCK_SetBrightnessEffect,      CAT_LOOKS,    "set brightness to 0",    0);
    add(BLOCK_ChangeBrightnessEffect,    CAT_LOOKS,    "change brightness by 10",  10);
    add(BLOCK_SetSaturationEffect,      CAT_LOOKS,    "set saturation to 50",    50);
    add(BLOCK_SetSaturationEffect,      CAT_LOOKS,    "set saturation to 0",    0);
    add(BLOCK_ChangeSaturationEffect,    CAT_LOOKS,    "change saturation by 10",    10);

    // ── BACKDROP ──────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;

    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "next backdrop",    0, "next");

    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "backdrop White",    0, "White");
    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "backdrop Sky",    0, "Sky");
    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "backdrop Grass",    0, "Grass");
    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "backdrop Night",    0, "Night");
    add(BLOCK_SwitchBackdrop,     CAT_LOOKS,     "backdrop Sunset",    0, "Sunset");

    // ── CONTROL (orange) ──────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;
    add(BLOCK_Wait,              CAT_CONTROL,   "wait 1 sec",            1);
    add(BLOCK_Repeat,            CAT_CONTROL,   "repeat 10",            10);
    add(BLOCK_Forever,           CAT_CONTROL,   "forever",               0);
    add(BLOCK_If,                CAT_CONTROL,   "if <cond>",             0);
    add(BLOCK_IfElse,            CAT_CONTROL,   "if <cond> else",        0);
    add(BLOCK_RepeatUntil,       CAT_CONTROL,   "repeat until <cond>",   0);
    add(BLOCK_Stop,              CAT_CONTROL,   "stop all",              0);
    add(BLOCK_AskWait,           CAT_CONTROL,   "ask and wait",          0, "What is your name?");
    add(BLOCK_ResetTimer,        CAT_SENSING,   "rest timer",            0);
    add(BLOCK_DistanceTo,CAT_SENSING,   "distance to mouse pointer",   0,"mouse pointer");
    add(BLOCK_MouseX,CAT_SENSING,"mouse x",0);

    // ── OPERATORS ─────────────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;
    add(BLOCK_Add,               CAT_OPERATORS, "add",                   0);
    add(BLOCK_Subtract,          CAT_OPERATORS, "subtract",              0);
    add(BLOCK_Multiply,          CAT_OPERATORS, "multiply",              0);
    add(BLOCK_Divide,            CAT_OPERATORS, "divide",                0);
    add(BLOCK_Random,            CAT_OPERATORS, "pick random 1 to 10",   0);
    add(BLOCK_And,               CAT_OPERATORS, "and",                   0);
    add(BLOCK_Or,                CAT_OPERATORS, "or",                    0);
    add(BLOCK_Not,               CAT_OPERATORS, "not",                   0);
    add(BLOCK_LessThan,          CAT_OPERATORS, "< (less than)",         0);
    add(BLOCK_GreaterThan,       CAT_OPERATORS, "> (greater than)",      0);
    add(BLOCK_Equal,             CAT_OPERATORS, "= (equal)",             0);
    add(BLOCK_DistanceTo,CAT_OPERATORS, "distance to mouse",   0,"mouse pointer");

    // ── VARIABLES ─────────────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;
    add(BLOCK_SetVariable,       CAT_VARIABLES, "set var to 0",          0, "myVar");
    add(BLOCK_ChangeVariable,    CAT_VARIABLES, "change var by 1",       1, "myVar");

    // ── EVENTS (yellow) ──────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;
    add(BLOCK_WhenFlagClicked, CAT_EVENTS,"when flag clicked",0);
    add(BLOCK_WhenKeyPressed, CAT_EVENTS,  "when space key pressed",0,"space");
    add(BLOCK_WhenSpriteClicked, CAT_EVENTS,"when this sprite clicked",0);
    add(BLOCK_Broadcast,CAT_EVENTS,"broadcast message1",0,"message1");
    add(BLOCK_WhenReceive,CAT_EVENTS,"when I receive message1",0,"message1");
    // ── PEN (dark green) ──────────────────────────────────────────────────
    y = state.paletteBlocks.back()->y + spacing;
    add(BLOCK_PenDown,           CAT_PEN,       "pen down",              0);
    add(BLOCK_PenUp,             CAT_PEN,       "pen up",                0);
    add(BLOCK_PenClear,          CAT_PEN,       "erase all",             0);
    add(BLOCK_SetPenColor,       CAT_PEN,       "set pen color",         0);
    add(BLOCK_SetPenSize,        CAT_PEN,       "set pen size to 2",     2);
    add(BLOCK_ChangePenSize,     CAT_PEN,       "change pen size by 1",  1);
    add(BLOCK_Stamp,             CAT_PEN,       "stamp",                 0);

    Logger::info("Palette initialized with " +
                 std::to_string(state.paletteBlocks.size()) + " blocks");
}

// ─── Main game loop ───────────────────────────────────────────────────────────
void gameLoop(GameState& state, UIManager& ui) {
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        // ── Event processing ──────────────────────────────────────────────
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEWHEEL) {
                // Forward scroll to UIManager for palette scrolling
                ui.handleMouseWheel(state.mouseX, state.mouseY, event.wheel.y);
            } else if (event.type == SDL_MOUSEMOTION) {
                ui.handleMouseMove(event.motion.x, event.motion.y);
                Input::handleEvent(state, event);
            } else if (event.type == SDL_MOUSEBUTTONDOWN ||
                       event.type == SDL_MOUSEBUTTONUP) {
                ui.handleMouseClick(event.button.x, event.button.y,
                                    event.type == SDL_MOUSEBUTTONDOWN,state);
                Input::handleEvent(state, event);
            } else {
                Input::handleEvent(state, event);
            }
        }

        // Sync palette scroll offset from UIManager → GameState
        state.paletteScrollY = ui.getPaletteScrollY();

        // ── Process UI button events ──────────────────────────────────────
        if (ui.isButtonPressed(UIManager::BTN_RUN)) {
            state.greenFlagClicked = true;
            Engine::startExecution(state);
            ui.addLog("Program started", "INFO");
        }
        if (ui.isButtonPressed(UIManager::BTN_STOP)) {
            state.exec.running       = false;
            state.exec.paused        = false;
            state.greenFlagClicked   = false;
            ui.addLog("Program stopped", "WARNING");
        }
        if (ui.isButtonPressed(UIManager::BTN_PAUSE)) {
            if (state.exec.running) {
                state.exec.paused = !state.exec.paused;
                ui.addLog(state.exec.paused ? "Paused" : "Resumed", "INFO");
            }
        }
        if (ui.isButtonPressed(UIManager::BTN_STEP)) {
            state.stepMode = true;
            state.stepNext = true;
            if (!state.exec.running) Engine::startExecution(state);
            state.exec.paused = true;
            ui.addLog("Step executed", "INFO");
        }
        if (ui.isButtonPressed(UIManager::BTN_SAVE)) {
            // SaveLoad::saveProject(state, SaveLoad::getDefaultSavePath());
            // ui.addLog("Project saved", "INFO");
            ui.showSaveDialog = true;
        }
        if (ui.isButtonPressed(UIManager::BTN_LOAD)) {
            SaveLoad::loadProject(state, SaveLoad::getDefaultSavePath());
            ui.addLog("Project loaded", "INFO");
        }
        if (ui.isButtonPressed(UIManager::BTN_NEW_PROJECT)) {
            for (auto* b : state.editorBlocks) delete b;
            state.editorBlocks.clear();
            state.penStrokes.clear();
            state.variables.clear();
            state.exec.running = false;
            ui.addLog("New project created", "INFO");
        }
        // if (ui.isButtonPressed(UIManager::BTN_ADD_SPRITE)) {
        //     Sprite* ns = new Sprite();
        //     ns->name = "Sprite" + std::to_string(state.sprites.size() + 1);
        //     // Copy first costume from sprite 0 if available
        //     if (!state.sprites.empty() && !state.sprites[0]->costumes.empty())
        //         ns->costumes = state.sprites[0]->costumes;
        //     state.sprites.push_back(ns);
        //     ui.addLog("Sprite added: " + ns->name, "INFO");
        // }

        if (ui.isButtonPressed(UIManager::BTN_ADD_SPRITE)) {
            Sprite* ns = new Sprite();


            int lastSelected = ui.lastSelectedSpriteIndex;

            if (lastSelected == 0) {
                ns->name = "Cat" + std::to_string(state.sprites.size() + 1);
                if (state.sprites.size() > 0 && !state.sprites[0]->costumes.empty())
                    ns->costumes = state.sprites[0]->costumes;
                ui.addLog("New cat added!", "INFO");
            } else {  // shapes
                ns->name = "Shape" + std::to_string(state.sprites.size() + 1);
                if (state.sprites.size() > 1 && !state.sprites[1]->costumes.empty())
                    ns->costumes = state.sprites[1]->costumes;
                ui.addLog("New shape added!", "INFO");
            }

            state.sprites.push_back(ns);
            ui.setSpriteCount((int)state.sprites.size());
        }
        if (ui.isButtonPressed(UIManager::BTN_CLEAR_LOG)) {
            ui.clearLogs();
        }
        if (ui.isButtonPressed(UIManager::BTN_TOGGLE_LOG)) {
            ui.toggleLogPanel();
        }

        // Sync selected sprite
        ui.setSpriteCount((int)state.sprites.size());
        int sel = ui.getSelectedSpriteIndex();
        if (sel >= 0 && sel < (int)state.sprites.size())
            state.selectedSpriteIndex = sel;

        // Tell UIManager how tall the palette content is (for scrollbar)
        if (!state.paletteBlocks.empty()) {
            Block* last = state.paletteBlocks.back();
            ui.setPaletteContentHeight(last->y + last->height + 20);
        }

        ui.resetButtons();

        // Handle save dialog buttons
        if (ui.showSaveDialog) {
            // MOUSE CLICK
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;

                // DIALOG
                int dw = 300, dh = 120;
                int dx = (state.windowWidth - dw)/2;
                int dy = (state.windowHeight - dh)/2;

                // YES
                SDL_Rect yesBtn = {dx + 50, dy + 70, 60, 30};
                if (mx >= yesBtn.x && mx <= yesBtn.x + yesBtn.w &&
                    my >= yesBtn.y && my <= yesBtn.y + yesBtn.h) {
                    // SAVe
                    SaveLoad::saveProject(state, SaveLoad::getDefaultSavePath());
                    ui.addLog("Project saved", "INFO");
                    ui.showSaveDialog = false;
                    }

                // NO
                SDL_Rect noBtn = {dx + 150, dy + 70, 60, 30};
                if (mx >= noBtn.x && mx <= noBtn.x + noBtn.w &&
                    my >= noBtn.y && my <= noBtn.y + noBtn.h) {
                    ui.showSaveDialog = false;
                    ui.addLog("Save cancelled", "WARNING");
                    }
            }
        }

        // Start text input if ask dialog is active
        if (state.askActive)  SDL_StartTextInput();
        else                   SDL_StopTextInput();

        // ── Update ────────────────────────────────────────────────────────
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - lastTime) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;  // cap at 100ms to avoid huge jumps
        lastTime   = now;

        Engine::update(state, dt);

        // Sync layout from UIManager
        SDL_Rect sr = ui.getStageRect();
        state.stageX      = sr.x;
        state.stageY      = sr.y;
        state.stageWidth  = sr.w;
        state.stageHeight = sr.h;

        SDL_Rect pr = ui.getPaletteRect();
        state.paletteWidth = pr.w;

        SDL_Rect er = ui.getEditorRect();
        state.editorX     = er.x;
        state.editorWidth = er.w;

        // ── Render ────────────────────────────────────────────────────────
        SDL_SetRenderDrawColor(state.renderer, 220, 220, 220, 255);
        SDL_RenderClear(state.renderer);

        // 1. UI chrome (menu, panels, sprite bar, log)
        ui.render(state.renderer, state);

        // 2. Palette blocks
        Renderer::renderPaletteBlocks(state);

        // 3. Editor blocks + execution cursor (step mode)
        Renderer::renderEditorBlocks(state);
        Renderer::renderExecutionCursor(state);

        // 4. Snap preview while dragging
        Renderer::renderSnapPreview(state);

        // 5. Stage content (pen layer + sprites + speech bubbles)
        Renderer::renderStageContent(state);

        // 6. Variable monitor overlay
        Renderer::renderVariableMonitor(state);

        // 7. Dragged block (top-most)
        if (state.draggedBlock)
            Renderer::renderBlock(state, state.draggedBlock, true);

        // 8. Ask/answer dialog (modal)
        Renderer::renderAskDialog(state);

        SDL_RenderPresent(state.renderer);
        SDL_Delay(16); // ~60 FPS
    }
}