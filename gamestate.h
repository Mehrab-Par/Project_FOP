
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>



struct Value {
    enum Type { DOUBLE, STRING, BOOL } type;

    double doubleVal;
    std::string stringVal;
    bool boolVal;

    // سازنده‌ها - دقیقاً مثل variant کار میکنه
    Value() : type(DOUBLE), doubleVal(0) {}
    Value(double v) : type(DOUBLE), doubleVal(v) {}
    Value(int v) : type(DOUBLE), doubleVal(v) {}
    Value(const std::string& v) : type(STRING), stringVal(v) {}
    Value(const char* v) : type(STRING), stringVal(v) {}
    Value(bool v) : type(BOOL), boolVal(v) {}

    // کپی سازنده و عملگر =
    Value(const Value& other) {
        type = other.type;
        switch (type) {
            case DOUBLE: doubleVal = other.doubleVal; break;
            case STRING: new(&stringVal) std::string(other.stringVal); break;
            case BOOL: boolVal = other.boolVal; break;
        }
    }

    ~Value() {
        if (type == STRING) {
            stringVal.~basic_string();
        }
    }

    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        if (type == STRING) stringVal.~basic_string();
        type = other.type;
        switch (type) {
            case DOUBLE: doubleVal = other.doubleVal; break;
            case STRING: new(&stringVal) std::string(other.stringVal); break;
            case BOOL: boolVal = other.boolVal; break;
        }
        return *this;
    }
};

// توابع تبدیل - دقیقاً مثل قبل کار میکنن
inline double toDouble(const Value& v) {
    switch (v.type) {
        case Value::DOUBLE: return v.doubleVal;
        case Value::BOOL:   return v.boolVal ? 1.0 : 0.0;
        case Value::STRING:
            try { return std::stod(v.stringVal); }
            catch (...) { return 0.0; }
    }
    return 0.0;
}

inline std::string toString(const Value& v) {
    switch (v.type) {
        case Value::STRING: return v.stringVal;
        case Value::DOUBLE: {
            double d = v.doubleVal;
            if (d == (int)d) return std::to_string((int)d);
            return std::to_string(d);
        }
        case Value::BOOL: return v.boolVal ? "true" : "false";
    }
    return "";
}

inline bool toBool(const Value& v) {
    switch (v.type) {
        case Value::BOOL:   return v.boolVal;
        case Value::DOUBLE: return v.doubleVal != 0.0;
        case Value::STRING: return !v.stringVal.empty();
    }
    return false;
}
// ─────────────────────────────────────────────
// Forward Declarations
// ─────────────────────────────────────────────
struct Block;
struct Sprite;
struct GameState;


// ─────────────────────────────────────────────
// Block Types Enum
// ─────────────────────────────────────────────
enum class BlockType {
    // Motion (blue)
    Move, TurnLeft, TurnRight, GoTo, ChangeX, ChangeY,
    SetX, SetY, PointInDirection, GoToRandom, GoToMouse,
    BounceOffEdge,

    // Looks (indigo)
    Say, SayForTime, Think, ThinkForTime,
    SwitchCostume, NextCostume,
    SwitchBackdrop, NextBackdrop,
    ChangeSize, SetSize,
    ChangeGraphicEffect, SetGraphicEffect, ClearGraphicEffects,
    Show, Hide,
    GoToFrontLayer, GoToBackLayer, MoveLayerForward, MoveLayerBackward,

    // Sound (purple)
    PlaySound, PlaySoundUntilDone, StopAllSounds, SetVolume, ChangeVolume,

    // Events (yellow)
    WhenGreenFlagClicked, WhenKeyPressed, WhenSpriteClicked, WhenBroadcast,
    Broadcast,

    // Control (orange)
    Wait, Repeat, Forever, IfThen, IfThenElse,
    WaitUntil, RepeatUntil, StopAll,
    EndRepeat, EndForever, EndIf, Else,

    // Sensing (cyan)
    TouchingMouse, TouchingEdge, TouchingSprite,
    DistanceToMouse, DistanceToSprite,
    Ask, Answer,
    KeyPressed, MouseDown, MouseX, MouseY,
    SetDragMode, Timer, ResetTimer,

    // Operators (green)
    Add, Subtract, Multiply, Divide,
    Equal, LessThan, GreaterThan,
    And, Or, Not,
    StringLength, StringAt, StringJoin,
    Modulo, Abs, Sqrt, Floor, Ceiling, Sin, Cos, Xor,

    // Variables
    SetVariable, ChangeVariable, ShowVariable, HideVariable,

    // Custom functions
    DefineFunction, CallFunction,

    // Pen Extension (dark green)
    PenUp, PenDown, Stamp, EraseAll,
    SetPenColor, ChangePenColor, SetPenSize, ChangePenSize,

    // Reporters (return values)
    ReporterCostumeNumber, ReporterBackdropNumber, ReporterSize,

    // Literal / Input node (not a real block — holds a value)
    Literal,

    None
};

// ─────────────────────────────────────────────
// Block Structure
// ─────────────────────────────────────────────
struct Block {
    BlockType type = BlockType::None;

    // Parameters (child expression blocks or literal values)
    std::vector<Block*> params;   // e.g. params[0] = amount for Move

    // For control structures: jump target line after pre-processing
    int jumpTarget = -1;          // e.g. If → EndIf line
    int elseTarget = -1;          // for IfElse → Else line

    // For literal / input nodes
    Value literalValue;

    // For named references (variable name, costume name, sound name, etc.)
    std::string name;

    // For custom function definition
    std::vector<std::string> paramNames;   // parameter names
    std::vector<Block*> body;              // body blocks of the function

    // Debug / source info
    int sourceLine = -1;

    // UI positioning (for drag & drop editor)
    int uiX = 0, uiY = 0;
    bool selected = false;

    ~Block();
};

// ─────────────────────────────────────────────
// Costume
// ─────────────────────────────────────────────
struct Costume {
    std::string name;
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    int width = 64, height = 64;
};

// ─────────────────────────────────────────────
// Sound
// ─────────────────────────────────────────────
struct Sound {
    std::string name;
    Mix_Chunk* chunk = nullptr;
    int volume = 100;   // 0–100
    bool muted = false;
    int channel = -1;   // SDL_mixer channel (-1 = not playing)
};

// ─────────────────────────────────────────────
// Sprite
// ─────────────────────────────────────────────
struct Sprite {
    std::string name;

    // Position & transform
    float x = 0.0f, y = 0.0f;
    float direction = 90.0f;   // degrees: 90 = right, 0 = up
    float size = 100.0f;       // percentage
    bool visible = true;
    int layer = 0;
    bool draggable = false;

    // Costumes
    std::vector<Costume> costumes;
    int currentCostume = 0;

    // Sounds
    std::vector<Sound> sounds;

    // Speech bubble
    std::string speechText;
    bool isThinker = false;    // think vs say
    float speechTimer = -1.0f; // -1 = permanent, else seconds remaining

    // Graphic effects
    float colorEffect = 0.0f;

    // Script (list of blocks for this sprite)
    std::vector<Block*> script;

    // Pen state
    bool penDown = false;
    SDL_Color penColor = {0, 0, 255, 255};
    int penSize = 1;
    float penSaturation = 100.0f;
    float penBrightness = 100.0f;

    // Initial state (for reset)
    float initX = 0.0f, initY = 0.0f;
    float initDirection = 90.0f;
    float initSize = 100.0f;
    int initCostume = 0;

    ~Sprite();
};

// ─────────────────────────────────────────────
// Logger
// ─────────────────────────────────────────────
enum class LogLevel { INFO, WARNING, ERROR_LVL };

struct LogEntry {
    int cycle;
    int line;
    std::string command;
    std::string operation;
    std::string data;
    LogLevel level;
};

// ─────────────────────────────────────────────
// Execution Context (per-sprite runtime state)
// ─────────────────────────────────────────────
struct ExecutionContext {
    int pc = 0;                          // program counter
    std::vector<int> loopStack;          // repeat counters
    std::vector<int> loopReturnStack;    // where to jump back
    bool isRunning = false;
    bool isPaused = false;
    bool isDebugMode = false;
    bool waitingForStep = false;         // step-by-step mode
    float waitTimer = 0.0f;             // for Wait blocks
    bool waitingUntil = false;

    // Custom function call stack
    struct CallFrame {
        int returnPC;
        std::unordered_map<std::string, Value> localVars;
    };
    std::vector<CallFrame> callStack;

    int watchdogCounter = 0;
    static const int WATCHDOG_MAX = 10000;
};

// ─────────────────────────────────────────────
// Backdrop
// ─────────────────────────────────────────────
struct Backdrop {
    std::string name;
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
};

// ─────────────────────────────────────────────
// Pen Drawing Record
// ─────────────────────────────────────────────
struct PenStroke {
    float x1, y1, x2, y2;
    SDL_Color color;
    int size;
};

struct PenStamp {
    SDL_Texture* texture = nullptr;
    float x, y;
    float size;
};

// ─────────────────────────────────────────────
// Custom Function Definition
// ─────────────────────────────────────────────
struct FunctionDef {
    std::string name;
    std::vector<std::string> paramNames;
    std::vector<Block*> body;
};

// ─────────────────────────────────────────────
// Main Game State
// ─────────────────────────────────────────────
struct GameState {
    // SDL
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int windowWidth = 1280;
    int windowHeight = 720;

    // Stage / scene area
    int stageX = 260, stageY = 10;
    int stageWidth = 480, stageHeight = 360;

    // Sprites
    std::vector<Sprite*> sprites;
    int activeSpriteIndex = -1;

    // Backdrops
    std::vector<Backdrop> backdrops;
    int currentBackdrop = 0;

    // Global variables
    std::unordered_map<std::string, Value> variables;

    // Custom functions
    std::unordered_map<std::string, FunctionDef> functions;

    // Execution
    ExecutionContext execCtx;
    int globalCycle = 0;

    // Logger
    std::vector<LogEntry> logs;
    bool showLogs = false;

    // Pen drawings
    std::vector<PenStroke> penStrokes;
    std::vector<PenStamp> penStamps;

    // Broadcast system
    std::string lastBroadcast;
    bool broadcastPending = false;

    // Input state
    std::string userAnswer;
    bool waitingForAnswer = false;
    std::string askQuestion;

    // Timer
    float globalTimer = 0.0f;

    // Mouse state
    int mouseX = 0, mouseY = 0;
    bool mouseDown = false;

    // UI state
    enum class UIPanel { NONE, CODE, COSTUMES, SOUNDS, SETTINGS, EXTENSIONS };
    UIPanel activePanel = UIPanel::CODE;
    bool penExtensionEnabled = false;
    bool showVariableMonitors = true;

    // Block editor state
    std::vector<Block*> editorBlocks;   // blocks placed in the code editor
    Block* draggedBlock = nullptr;
    int dragOffsetX = 0, dragOffsetY = 0;

    // Pen editor
    bool inPaintMode = false;

    ~GameState();
};
_H