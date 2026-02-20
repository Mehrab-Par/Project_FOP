#include "SaveLoad.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <map>

namespace SaveLoad {

// ─── block type ↔ string maps ─────────────────────────────────────────────
static const std::map<BlockType, std::string>& typeToStr() {
    static std::map<BlockType, std::string> m = {
        {BLOCK_Move,          "move"},
        {BLOCK_TurnRight,     "turnRight"},
        {BLOCK_TurnLeft,      "turnLeft"},
        {BLOCK_GoToXY,        "goToXY"},
        {BLOCK_SetX,          "setX"},
        {BLOCK_SetY,          "setY"},
        {BLOCK_ChangeX,       "changeX"},
        {BLOCK_ChangeY,       "changeY"},
        {BLOCK_PointDirection,"pointDirection"},
        {BLOCK_BounceOffEdge, "bounceOffEdge"},
        {BLOCK_Say,           "say"},
        {BLOCK_SayForSecs,    "sayForSecs"},
        {BLOCK_Think,         "think"},
        {BLOCK_ThinkForSecs,  "thinkForSecs"},
        {BLOCK_Show,          "show"},
        {BLOCK_Hide,          "hide"},
        {BLOCK_SwitchCostume, "switchCostume"},
        {BLOCK_NextCostume,   "nextCostume"},
        {BLOCK_SetSize,       "setSize"},
        {BLOCK_ChangeSize,    "changeSize"},
        {BLOCK_SetColorEffect,"setColorEffect"},
        {BLOCK_ChangeColorEffect,"changeColorEffect"},
        {BLOCK_ClearGraphicEffects,"clearEffects"},
        {BLOCK_PlaySound,     "playSound"},
        {BLOCK_StopAllSounds, "stopAllSounds"},
        {BLOCK_WhenFlagClicked,"whenFlagClicked"},
        {BLOCK_Wait,          "wait"},
        {BLOCK_WaitUntil,     "waitUntil"},
        {BLOCK_Repeat,        "repeat"},
        {BLOCK_Forever,       "forever"},
        {BLOCK_If,            "if"},
        {BLOCK_IfElse,        "ifElse"},
        {BLOCK_Stop,          "stop"},
        {BLOCK_RepeatUntil,   "repeatUntil"},
        {BLOCK_AskWait,       "askWait"},
        {BLOCK_SetVariable,   "setVar"},
        {BLOCK_ChangeVariable,"changeVar"},
        {BLOCK_PenDown,       "penDown"},
        {BLOCK_PenUp,         "penUp"},
        {BLOCK_PenClear,      "penClear"},
        {BLOCK_SetPenColor,   "setPenColor"},
        {BLOCK_SetPenSize,    "setPenSize"},
        {BLOCK_ChangePenSize, "changePenSize"},
        {BLOCK_Stamp,         "stamp"},
        {BLOCK_Add,           "add"},
        {BLOCK_Subtract,      "sub"},
        {BLOCK_Multiply,      "mul"},
        {BLOCK_Divide,        "div"},
        {BLOCK_Mod,           "mod"},
        {BLOCK_And,           "and"},
        {BLOCK_Or,            "or"},
        {BLOCK_Not,           "not"},
        {BLOCK_LessThan,      "lt"},
        {BLOCK_Equal,         "eq"},
        {BLOCK_GreaterThan,   "gt"},
        {BLOCK_Literal,       "literal"},
        {BLOCK_None,          "none"},
    };
    return m;
}

static BlockType strToType(const std::string& s) {
    for (auto& kv : typeToStr())
        if (kv.second == s) return kv.first;
    return BLOCK_None;
}

static int catToInt(BlockCategory c) { return (int)c; }
static BlockCategory intToCat(int i) { return (BlockCategory)i; }

// ─── serialization helpers ────────────────────────────────────────────────
static void writeBlock(std::ofstream& f, const Block* b, int indent = 0) {
    std::string sp(indent * 2, ' ');
    auto it = typeToStr().find(b->type);
    std::string ts = (it != typeToStr().end()) ? it->second : "none";

    f << sp << "BLOCK " << ts << "\n";
    f << sp << "  cat " << catToInt(b->category) << "\n";
    f << sp << "  text " << b->text << "\n";
    f << sp << "  num " << b->numberValue << "\n";
    f << sp << "  str " << b->stringValue << "\n";
    f << sp << "  xy " << b->x << " " << b->y << "\n";

    if (!b->nested.empty()) {
        f << sp << "  NESTED " << b->nested.size() << "\n";
        for (auto* child : b->nested) writeBlock(f, child, indent + 2);
        f << sp << "  END_NESTED\n";
    }
    if (!b->nested2.empty()) {
        f << sp << "  NESTED2 " << b->nested2.size() << "\n";
        for (auto* child : b->nested2) writeBlock(f, child, indent + 2);
        f << sp << "  END_NESTED2\n";
    }
    f << sp << "END_BLOCK\n";
}

// ─── save ────────────────────────────────────────────────────────────────────
bool saveProject(const GameState& state, const std::string& filename) {
    std::ofstream f(filename);
    if (!f.is_open()) {
        Logger::error("Cannot open file for save: " + filename);
        return false;
    }

    f << "# ScratchClone Project v2\n";

    // Stage
    f << "[stage]\n";
    f << "color " << (int)state.stageColor.r << " "
                  << (int)state.stageColor.g << " "
                  << (int)state.stageColor.b << "\n";
    f << "colorIdx " << state.currentColorIndex << "\n";
    f << "penExt " << (state.penExtensionActive ? 1 : 0) << "\n";

    // Variables
    f << "[variables]\n";
    f << "count " << state.variables.size() << "\n";
    for (auto& kv : state.variables)
        f << "var " << kv.first << " " << kv.second << "\n";

    // Sprites
    f << "[sprites]\n";
    f << "count " << state.sprites.size() << "\n";
    for (auto* sp : state.sprites) {
        f << "SPRITE\n";
        f << "  name " << sp->name << "\n";
        f << "  pos "  << sp->x << " " << sp->y << "\n";
        f << "  dir "  << sp->direction << "\n";
        f << "  size " << sp->size << "\n";
        f << "  vis "  << (sp->visible ? 1 : 0) << "\n";
        f << "  cost " << sp->currentCostume << "\n";
        f << "END_SPRITE\n";
    }

    // Editor blocks
    f << "[blocks]\n";
    f << "count " << state.editorBlocks.size() << "\n";
    for (auto* b : state.editorBlocks)
        writeBlock(f, b);

    f.close();
    Logger::info("Project saved to: " + filename);
    return true;
}

// ─── load ────────────────────────────────────────────────────────────────────
// Simple line-based parser
static Block* parseBlock(std::ifstream& f) {
    Block* b = new Block();
    std::string line;
    while (std::getline(f, line)) {
        // trim leading spaces
        size_t start = line.find_first_not_of(' ');
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line == "END_BLOCK") break;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "cat")  { int c; ss >> c; b->category = intToCat(c); }
        else if (token == "text") { std::string t; std::getline(ss, t); if (!t.empty() && t[0]==' ') t=t.substr(1); b->text=t; }
        else if (token == "num")  { ss >> b->numberValue; }
        else if (token == "str")  { std::string v; std::getline(ss, v); if (!v.empty() && v[0]==' ') v=v.substr(1); b->stringValue=v; }
        else if (token == "xy")   { ss >> b->x >> b->y; }
        else if (token == "NESTED") {
            int cnt; ss >> cnt;
            for (int i = 0; i < cnt; i++) {
                std::string nl;
                while (std::getline(f, nl)) {
                    size_t s2 = nl.find_first_not_of(' ');
                    if (s2 == std::string::npos) continue;
                    nl = nl.substr(s2);
                    if (nl.substr(0,5) == "BLOCK") {
                        std::istringstream ns(nl); std::string bk, ts; ns >> bk >> ts;
                        Block* child = parseBlock(f);
                        child->type = strToType(ts);
                        b->nested.push_back(child);
                        break;
                    }
                    if (nl == "END_NESTED") break;
                }
            }
        }
        else if (token == "NESTED2") {
            int cnt; ss >> cnt;
            for (int i = 0; i < cnt; i++) {
                std::string nl;
                while (std::getline(f, nl)) {
                    size_t s2 = nl.find_first_not_of(' ');
                    if (s2 == std::string::npos) continue;
                    nl = nl.substr(s2);
                    if (nl.substr(0,5) == "BLOCK") {
                        std::istringstream ns(nl); std::string bk, ts; ns >> bk >> ts;
                        Block* child = parseBlock(f);
                        child->type = strToType(ts);
                        b->nested2.push_back(child);
                        break;
                    }
                    if (nl == "END_NESTED2") break;
                }
            }
        }
    }
    return b;
}

bool loadProject(GameState& state, const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        Logger::error("Cannot open file for load: " + filename);
        return false;
    }

    // Clear existing state
    for (auto* b : state.editorBlocks) delete b;
    state.editorBlocks.clear();
    state.penStrokes.clear();
    state.isDrawingStroke = false;

    std::string line;
    std::string section;

    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') { section = line; continue; }

        std::istringstream ss(line);
        std::string token; ss >> token;

        if (section == "[stage]") {
            if (token == "color") {
                int r,g,b; ss >> r >> g >> b;
                state.stageColor = {(Uint8)r,(Uint8)g,(Uint8)b,255};
            }
            else if (token == "colorIdx") { ss >> state.currentColorIndex; }
            else if (token == "penExt") { int v; ss >> v; state.penExtensionActive = (v==1); }
        }
        else if (section == "[variables]") {
            if (token == "var") {
                std::string name, val;
                ss >> name; std::getline(ss, val);
                if (!val.empty() && val[0]==' ') val=val.substr(1);
                state.variables[name] = val;
            }
        }
        else if (section == "[sprites]") {
            if (token == "SPRITE") {
                // Parse sprite block
                Sprite* sp = nullptr;
                // Find existing or create
                while (std::getline(f, line)) {
                    if (line.empty()) continue;
                    std::istringstream ss2(line);
                    std::string t2; ss2 >> t2;
                    if (t2 == "name") {
                        std::string nm; std::getline(ss2, nm);
                        if (!nm.empty() && nm[0]==' ') nm=nm.substr(1);
                        sp = nullptr;
                        for (auto* s : state.sprites)
                            if (s->name == nm) { sp = s; break; }
                        if (!sp) { sp = new Sprite(); sp->name = nm; state.sprites.push_back(sp); }
                    }
                    else if (t2 == "pos"  && sp) { ss2 >> sp->x >> sp->y; }
                    else if (t2 == "dir"  && sp) { ss2 >> sp->direction; }
                    else if (t2 == "size" && sp) { ss2 >> sp->size; }
                    else if (t2 == "vis"  && sp) { int v; ss2 >> v; sp->visible=(v==1); }
                    else if (t2 == "cost" && sp) { ss2 >> sp->currentCostume; }
                    else if (t2 == "END_SPRITE") break;
                }
            }
        }
        else if (section == "[blocks]") {
            if (token == "BLOCK") {
                std::string ts; ss >> ts;
                Block* b = parseBlock(f);
                b->type = strToType(ts);
                state.editorBlocks.push_back(b);
            }
        }
    }

    f.close();
    Logger::info("Project loaded from: " + filename);
    return true;
}

std::string getDefaultSavePath() { return "scratch_project.sav"; }

} // namespace SaveLoad
