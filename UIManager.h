#pragma once
#include <SDL2/SDL.h>
#include "GameState.h"
#include <string>
#include <vector>

// NO SDL_ttf dependency at all

struct UIPanel { SDL_Rect rect; SDL_Color bgColor; bool visible;
    UIPanel():rect({0,0,0,0}),bgColor({240,240,240,255}),visible(true){} };

struct Button { SDL_Rect rect; std::string label; SDL_Color color,hoverColor;
    bool isHovered,isPressed; int id;
    Button():rect({0,0,0,0}),color({80,80,80,255}),hoverColor({110,110,110,255}),
             isHovered(false),isPressed(false),id(0){} };

namespace UILayout {
    const int MENU_H  = 35;
    const int PAL_W   = 210;
    const int SPR_H   = 110;
    const int LOG_W   = 300;
    const int LOG_H   = 140;
    const int STAGE_W = 480;
    const int STAGE_H = 360;
}

struct UIManager {
    enum ButtonID {
        BTN_NEW_PROJECT=1, BTN_SAVE=2,  BTN_LOAD=3,
        BTN_RUN=4,         BTN_STOP=5,  BTN_PAUSE=6,
        BTN_STEP=7,        BTN_ADD_SPRITE=8,
        BTN_TOGGLE_LOG=9,  BTN_CLEAR_LOG=10, BTN_HELP=11
    };

    UIManager();
    ~UIManager();


    void init(int w, int h);
    void render(SDL_Renderer* r, GameState& state);
    void handleMouseMove(int x, int y);
    void handleMouseClick(int x, int y, bool down, GameState& state);
    void handleMouseWheel(int x, int y, int deltaY);

    bool isButtonPressed(int id) const;
    void resetButtons();

    void addLog(const std::string& msg, const std::string& level="INFO");
    void clearLogs();
    void toggleLogPanel();

    void setSpriteCount(int n)          { spriteCount=n; }
    int  getSelectedSpriteIndex() const { return selectedIdx; }
    int  getPaletteScrollY()      const { return palScrollY; }
    void setPaletteContentHeight(int h) { palContentHeight = h; }

    SDL_Rect getStageRect()    const { return stagePanel.rect; }
    SDL_Rect getPaletteRect()  const { return palPanel.rect;   }
    SDL_Rect getEditorRect()   const { return edPanel.rect;    }

    int W,H;
    UIPanel menuBar, palPanel, edPanel, stagePanel, sprBar, logPanel;
    std::vector<Button> menuBtns, sprBtns;
    int spriteCount, selectedIdx;
    bool showSaveDialog;
    bool saveConfrimed;
    int lastSelectedSpriteIndex;
    bool showLog;

    // palette scroll
    int palScrollY;
    int palContentHeight;
    static const int PAL_SCROLL_STEP = 36;
    static const int CAT_TAB_H       = 22;  // height of category tab bar

    // category tabs: 0=ALL 1=Motion 2=Looks 3=Control 4=Ops 5=Vars 6=Pen
    int selectedCatTab;

    struct LogEntry { std::string msg,level; };
    std::vector<LogEntry> logs;

    void buildButtons();
    void renderPanel       (SDL_Renderer*, const UIPanel&);
    void renderButton      (SDL_Renderer*, const Button&);
    void renderLog         (SDL_Renderer*);
    void renderSprBar      (SDL_Renderer*,GameState& state);
    void renderCategoryTabs(SDL_Renderer*);
    void renderScrollBar   (SDL_Renderer*);

    // Built-in pixel text — no SDL_ttf needed
    void drawText(SDL_Renderer*, const std::string&, int x, int y,
                  SDL_Color col={255,255,255,255}, int scale=1);
    void drawChar(SDL_Renderer*, char c, int x, int y, SDL_Color col, int scale);

    bool hit(int x,int y,const SDL_Rect& r) const;
};
