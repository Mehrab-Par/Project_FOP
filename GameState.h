#pragma once
#include <string>
#include <vector>
#include <map>
#include <SDL2/SDL.h>

// ─────────────────────────────────────────────────────────────────────────────
// Block types  (all Scratch categories)
// ─────────────────────────────────────────────────────────────────────────────
enum BlockType {
    // Motion (blue)
    BLOCK_Move, BLOCK_TurnRight, BLOCK_TurnLeft,
    BLOCK_GoToXY, BLOCK_SetX, BLOCK_SetY,
    BLOCK_ChangeX, BLOCK_ChangeY,
    BLOCK_PointDirection, BLOCK_BounceOffEdge,
    BLOCK_GoToMousePointer, BLOCK_GoToRandomPosition,
    // Looks (purple)
    BLOCK_Say, BLOCK_SayForSecs, BLOCK_Think, BLOCK_ThinkForSecs,
    BLOCK_Show, BLOCK_Hide,
    BLOCK_SwitchCostume, BLOCK_NextCostume,
    BLOCK_SwitchBackdrop, BLOCK_NextBackdrop,
    BLOCK_SetSize, BLOCK_ChangeSize,
    BLOCK_SetColorEffect, BLOCK_ChangeColorEffect, BLOCK_ClearGraphicEffects,
    BLOCK_SetGhostEffect,BLOCK_ChangeGhostEffect,
    BLOCK_SetBrightnessEffect, BLOCK_ChangeBrightnessEffect,
    BLOCK_SetSaturationEffect, BLOCK_ChangeSaturationEffect,
    BLOCK_GoToFrontLayer, BLOCK_GoToBackLayer,
    BLOCK_GoForwardLayers, BLOCK_GoBackwardLayers,
    // Sound (magenta)
    BLOCK_PlaySound, BLOCK_PlaySoundUntilDone, BLOCK_StopAllSounds,
    BLOCK_SetVolume, BLOCK_ChangeVolume,
    // Events (yellow)
    BLOCK_WhenFlagClicked, BLOCK_WhenKeyPressed, BLOCK_WhenSpriteClicked,
    BLOCK_Broadcast, BLOCK_BroadcastAndWait, BLOCK_WhenReceive,
    // Control (orange)
    BLOCK_Wait, BLOCK_WaitUntil,
    BLOCK_Repeat, BLOCK_Forever,
    BLOCK_If, BLOCK_IfElse, BLOCK_Stop, BLOCK_RepeatUntil,
    // Sensing (cyan)
    BLOCK_Touching, BLOCK_TouchingColor, BLOCK_ColorTouching,
    BLOCK_DistanceTo, BLOCK_AskWait, BLOCK_Answer,
    BLOCK_KeyPressed, BLOCK_MouseDown,
    BLOCK_MouseX, BLOCK_MouseY,
    BLOCK_SetDragMode,
    BLOCK_Timer, BLOCK_ResetTimer,
    // Operators (green)
    BLOCK_Add, BLOCK_Subtract, BLOCK_Multiply, BLOCK_Divide,
    BLOCK_Random, BLOCK_LessThan, BLOCK_Equal, BLOCK_GreaterThan,
    BLOCK_And, BLOCK_Or, BLOCK_Not,
    BLOCK_Join, BLOCK_LetterOf, BLOCK_LengthOf,
    BLOCK_Mod, BLOCK_Round, BLOCK_Abs, BLOCK_Sqrt,
    BLOCK_Floor, BLOCK_Ceiling, BLOCK_Sin, BLOCK_Cos,
    // Variables (orange-red)
    BLOCK_SetVariable, BLOCK_ChangeVariable,
    BLOCK_ShowVariable, BLOCK_HideVariable,
    // Pen extension (dark green)
    BLOCK_PenClear, BLOCK_PenDown, BLOCK_PenUp,
    BLOCK_SetPenColor, BLOCK_SetPenSize, BLOCK_ChangePenSize,
    BLOCK_SetPenColorEffect, BLOCK_ChangePenColorEffect,
    BLOCK_Stamp,
    // Internal
    BLOCK_Literal,
    BLOCK_None
};

enum BlockCategory {
    CAT_MOTION, CAT_LOOKS, CAT_SOUND, CAT_EVENTS,
    CAT_CONTROL, CAT_SENSING, CAT_OPERATORS,
    CAT_VARIABLES, CAT_PEN
};

// ─────────────────────────────────────────────────────────────────────────────
// Block node
// ─────────────────────────────────────────────────────────────────────────────
struct Block {
    BlockType    type;
    BlockCategory category;
    std::string  text;
    std::vector<Block*> inputs;   // expression inputs
    std::string  stringValue;
    double       numberValue;
    int x, y, width, height;
    Block* nextBlock;             // next sibling in sequence
    std::vector<Block*> nested;   // if-body / repeat-body
    std::vector<Block*> nested2;  // else-body (IfElse only)
    bool selected, isDragging;
    // Pre-scan jump targets (indices into flat editorBlocks)
    int jumpTarget;
    int elseTarget;
    Block();
    ~Block();
};

// ─────────────────────────────────────────────────────────────────────────────
// Costume / Sprite
// ─────────────────────────────────────────────────────────────────────────────
struct Costume {
    std::string  name;
    SDL_Texture* texture;
    int width, height;
    Costume();
};

struct Sprite {
    std::string name;
    float x, y, direction, size; // size in %
    bool  visible;
    int   layer;
    std::vector<Costume> costumes;
    int   currentCostume;
    // Speech
    std::string sayText;
    bool  isThinking;
    float sayTimer;      // > 0 = timed, -1 = permanent
    // Pen
    bool      penDown;
    SDL_Color penColor;
    int       penSize;
    // Looks effects
    float colorEffect;   // 0-360 hue shift
    float ghostEffect;   // 0-100 transparency
    float brightnessEffect;    // 0-100 brightness
    float saturationEffect;    //0-100 saturation
    // Sensing / interaction
    bool  isDraggable;
    std::string answer; // last ask-answer
    // Scripts attached to this sprite
    std::vector<Block*> scripts;
    Sprite();
    ~Sprite();
};

// ─────────────────────────────────────────────────────────────────────────────
// Pen layer
// ─────────────────────────────────────────────────────────────────────────────
struct PenStroke {
    std::vector<SDL_Point> points;
    SDL_Color color;
    int size;
    PenStroke();
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-sprite execution context
// ─────────────────────────────────────────────────────────────────────────────
struct SpriteExecCtx {
    int  pc;                        // index into editorBlocks for THIS sprite
    std::vector<int>  loopCount;    // repeat counter stack
    std::vector<int>  loopStart;    // loop-start pc stack
    float waitTimer;                // seconds remaining in a wait
    bool  waitUntilActive;
    bool  askWaiting;
    bool  finished;
    SpriteExecCtx();
};

// ─────────────────────────────────────────────────────────────────────────────
// Global execution context
// ─────────────────────────────────────────────────────────────────────────────
struct ExecutionContext {
    bool running;
    bool paused;
    std::map<Sprite*, SpriteExecCtx> ctx; // one ctx per sprite
    float globalTimer;
    std::string pendingBroadcast;
    ExecutionContext();
};

// ─────────────────────────────────────────────────────────────────────────────
// GameState
// ─────────────────────────────────────────────────────────────────────────────
struct StageColor { std::string name; SDL_Color color; };

struct GameState {
    SDL_Window*   window;
    SDL_Renderer* renderer;

    int windowWidth, windowHeight;

    // Layout zones (computed from UIManager constants)
    int stageX, stageY, stageWidth, stageHeight;
    int paletteWidth, editorX, editorWidth;

    // Background
    SDL_Color stageColor;
    SDL_Texture* backdropTexture;
    std::vector<StageColor> stageColors;
    int currentColorIndex;

    // Sprites
    std::vector<Sprite*> sprites;
    int selectedSpriteIndex;

    // Variables (dynamic typed: stored as string, interpreted as needed)
    std::map<std::string, std::string> variables;
    std::map<std::string, bool>        variableVisible;

    // Execution engine
    ExecutionContext exec;

    // Pen layer
    std::vector<PenStroke> penStrokes;
    PenStroke currentStroke;
    bool      isDrawingStroke;

    // Block palette (left panel, never executed directly)
    std::vector<Block*> paletteBlocks;

    // Editor (centre panel, user-assembled script)
    std::vector<Block*> editorBlocks; // top-level blocks only

    // Drag & drop
    Block* draggedBlock;
    int    dragOffsetX, dragOffsetY;
    bool   draggingFromPalette;
    Block* snapTarget;
    bool   snapAbove;

    // Input snapshot
    int  mouseX, mouseY;
    bool mousePressed;
    bool greenFlagClicked, stopClicked;

    // Safety: watchdog counter
    int watchdogCounter;
    static const int WATCHDOG_LIMIT = 2000;

    // Debug step-mode
    bool stepMode;
    bool stepNext;

    // Palette category filter (-1 = all)
    int paletteCategory;
    // Palette scroll offset in pixels
    int paletteScrollY;

    // Audio
    bool globalMute;
    int  globalVolume; // 0-100

    // Ask/answer overlay
    bool        askActive;
    std::string askQuestion;
    std::string askInput;     // typed so far
    Sprite*     askSprite;    // which sprite asked

    // Pen extension active?
    bool penExtensionActive;

    GameState();
    ~GameState();
};
