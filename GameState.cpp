#include "GameState.h"

// ─────────────────────────────────────────────────────────────────────────────
Block::Block() {
    type = BLOCK_None; category = CAT_MOTION;
    text = ""; stringValue = ""; numberValue = 0;
    x = y = 0; width = 185; height = 36;
    nextBlock = nullptr; selected = false; isDragging = false;
    jumpTarget = -1; elseTarget = -1;
}
Block::~Block() {
    // Do NOT recursively delete children here — ownership is in vectors
}

// ─────────────────────────────────────────────────────────────────────────────
Costume::Costume() : texture(nullptr), width(64), height(64) {}

// ─────────────────────────────────────────────────────────────────────────────
Sprite::Sprite() {
    name = "Sprite"; x = 0; y = 0;
    direction = 90; // facing right (Scratch convention: 90 = right)
    size = 100.0f;  // 100%
    visible = true; layer = 0;
    currentCostume = 0; isThinking = false; sayTimer = 0;
    penDown = false;
    penColor = {0, 0, 200, 255};
    penSize = 2;
    colorEffect = 0; ghostEffect = 0; brightnessEffect = 0; saturationEffect = 0;
    isDraggable = true;
}
Sprite::~Sprite() {
    for (auto& c : costumes)
        if (c.texture) SDL_DestroyTexture(c.texture);
}

// ─────────────────────────────────────────────────────────────────────────────
PenStroke::PenStroke() : size(2) { color = {0, 0, 200, 255}; }

// ─────────────────────────────────────────────────────────────────────────────
SpriteExecCtx::SpriteExecCtx()
    : pc(0), waitTimer(0), waitUntilActive(false),
      askWaiting(false), finished(false) {}

ExecutionContext::ExecutionContext()
    : running(false), paused(false), globalTimer(0) {}

// ─────────────────────────────────────────────────────────────────────────────
GameState::GameState() {
    window   = nullptr;
    renderer = nullptr;

    windowWidth  = 1280;
    windowHeight = 720;

    // Layout mirrors UIManager constants
    paletteWidth = 210;
    editorX      = 210;
    stageWidth   = 480;
    stageHeight  = 360;
    stageX       = windowWidth - stageWidth - 8;
    stageY       = 35;        // below menu bar
    editorWidth  = stageX - editorX - 4;

    stageColor       = {255, 255, 255, 255};
    backdropTexture  = nullptr;
    stageColors      = {
        {"White",  {255, 255, 255, 255}},
        {"Sky",    {135, 206, 235, 255}},
        {"Grass",  {144, 238, 144, 255}},
        {"Night",  {25,  25,  112, 255}},
        {"Sunset", {255, 140,  70, 255}}
    };
    currentColorIndex = 0;
    selectedSpriteIndex = 0;

    draggedBlock       = nullptr;
    dragOffsetX        = 0;
    dragOffsetY        = 0;
    draggingFromPalette = false;
    snapTarget         = nullptr;
    snapAbove          = false;

    mouseX = mouseY = 0;
    mousePressed       = false;
    greenFlagClicked   = false;
    stopClicked        = false;
    isDrawingStroke    = false;

    watchdogCounter    = 0;
    stepMode           = false;
    stepNext           = false;
    paletteCategory    = -1;
    paletteScrollY     = 0;

    globalMute         = false;
    globalVolume       = 80;

    askActive          = false;
    askSprite          = nullptr;
    penExtensionActive = false;
}

GameState::~GameState() {
    for (auto* s : sprites)      delete s;
    for (auto* b : paletteBlocks) delete b;
    for (auto* b : editorBlocks)  delete b;
    if (backdropTexture) SDL_DestroyTexture(backdropTexture);
}
