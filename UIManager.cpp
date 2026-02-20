#include "UIManager.h"
#include <cstring>

// ─── 5x7 pixel font (printable ASCII 32-126) ────────────────────────────────
static const unsigned char FONT5x7[95][7] = {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
{0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // !
{0x0A,0x0A,0x00,0x00,0x00,0x00,0x00}, // "
{0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00}, // #
{0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}, // $
{0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // %
{0x0C,0x12,0x14,0x08,0x15,0x12,0x0D}, // &
{0x04,0x04,0x00,0x00,0x00,0x00,0x00}, // '
{0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // (
{0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // )
{0x00,0x04,0x15,0x0E,0x15,0x04,0x00}, // *
{0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, // +
{0x00,0x00,0x00,0x00,0x04,0x04,0x08}, // ,
{0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
{0x00,0x00,0x00,0x00,0x00,0x04,0x00}, // .
{0x01,0x01,0x02,0x04,0x08,0x10,0x10}, // /
{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 2
{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E}, // 3
{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
{0x00,0x04,0x00,0x00,0x04,0x00,0x00}, // :
{0x00,0x04,0x00,0x00,0x04,0x04,0x08}, // ;
{0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // <
{0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}, // =
{0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // >
{0x0E,0x11,0x01,0x02,0x04,0x00,0x04}, // ?
{0x0E,0x11,0x17,0x15,0x17,0x10,0x0E}, // @
{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
{0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}, // D
{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
{0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
{0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
{0x11,0x11,0x15,0x15,0x15,0x15,0x0A}, // W
{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
{0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}, // [
{0x10,0x10,0x08,0x04,0x02,0x01,0x01}, // backslash
{0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}, // ]
{0x04,0x0A,0x11,0x00,0x00,0x00,0x00}, // ^
{0x00,0x00,0x00,0x00,0x00,0x00,0x1F}, // _
{0x08,0x04,0x00,0x00,0x00,0x00,0x00}, // `
{0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // a
{0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}, // b
{0x00,0x00,0x0E,0x10,0x10,0x10,0x0E}, // c
{0x01,0x01,0x0F,0x11,0x11,0x11,0x0F}, // d
{0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}, // e
{0x06,0x09,0x08,0x1C,0x08,0x08,0x08}, // f
{0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}, // g
{0x10,0x10,0x16,0x19,0x11,0x11,0x11}, // h
{0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}, // i
{0x02,0x00,0x06,0x02,0x02,0x12,0x0C}, // j
{0x10,0x10,0x12,0x14,0x18,0x14,0x12}, // k
{0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}, // l
{0x00,0x00,0x1A,0x15,0x15,0x11,0x11}, // m
{0x00,0x00,0x16,0x19,0x11,0x11,0x11}, // n
{0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}, // o
{0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}, // p
{0x00,0x00,0x0F,0x11,0x0F,0x01,0x01}, // q
{0x00,0x00,0x16,0x19,0x10,0x10,0x10}, // r
{0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E}, // s
{0x08,0x08,0x1C,0x08,0x08,0x09,0x06}, // t
{0x00,0x00,0x11,0x11,0x11,0x13,0x0D}, // u
{0x00,0x00,0x11,0x11,0x11,0x0A,0x04}, // v
{0x00,0x00,0x11,0x15,0x15,0x15,0x0A}, // w
{0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // x
{0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}, // y
{0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}, // z
{0x06,0x08,0x08,0x18,0x08,0x08,0x06}, // {
{0x04,0x04,0x04,0x00,0x04,0x04,0x04}, // |
{0x0C,0x02,0x02,0x03,0x02,0x02,0x0C}, // }
{0x08,0x15,0x02,0x00,0x00,0x00,0x00}, // ~
};

void UIManager::drawChar(SDL_Renderer* r, char c, int x, int y, SDL_Color col, int s) {
    int idx = (unsigned char)c - 32;
    if (idx < 0 || idx >= 95) return;
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 7; row++) {
        unsigned char bits = FONT5x7[idx][row];
        for (int col2 = 0; col2 < 5; col2++) {
            if (bits & (0x10 >> col2)) {
                SDL_Rect px = {x + col2*s, y + row*s, s, s};
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

void UIManager::drawText(SDL_Renderer* r, const std::string& txt,
                         int x, int y, SDL_Color col, int s) {
    int cx = x;
    for (char c : txt) {
        if (c == ' ') { cx += 4*s; continue; }
        drawChar(r, c, cx, y, col, s);
        cx += 6*s;
    }
}

// ─── lifecycle ───────────────────────────────────────────────────────────────

UIManager::UIManager()
    : W(1280), H(720), spriteCount(1), selectedIdx(0),
      palScrollY(0), palContentHeight(2000), selectedCatTab(0) {}
UIManager::~UIManager() {}

void UIManager::init(int w, int h) {
    W = w; H = h;
    int contH = H - UILayout::MENU_H - UILayout::SPR_H;
    int edW   = W - UILayout::PAL_W - UILayout::STAGE_W - 8;

    menuBar.rect  = {0,0,W,UILayout::MENU_H};
    menuBar.bgColor = {55,55,55,255};

    palPanel.rect  = {0,UILayout::MENU_H,UILayout::PAL_W,contH};
    palPanel.bgColor = {245,245,245,255};

    edPanel.rect   = {UILayout::PAL_W,UILayout::MENU_H,edW,contH};
    edPanel.bgColor = {255,255,255,255};

    int sx = UILayout::PAL_W + edW + 8;
    stagePanel.rect   = {sx, UILayout::MENU_H+5, UILayout::STAGE_W, UILayout::STAGE_H};
    stagePanel.bgColor= {255,255,255,255};

    sprBar.rect   = {0,H-UILayout::SPR_H,W,UILayout::SPR_H};
    sprBar.bgColor= {235,235,235,255};

    logPanel.rect = {sx, UILayout::MENU_H+5+UILayout::STAGE_H+6,
                     UILayout::LOG_W, UILayout::LOG_H};
    logPanel.bgColor={25,25,25,255};
    logPanel.visible = true;

    buildButtons();
    addLog("Ready! Drag blocks to editor.", "INFO");
}

void UIManager::buildButtons() {
    menuBtns.clear(); sprBtns.clear();

    auto mb = [](int id, const char* lbl, int x, int y, int w, int h,
                 SDL_Color c, SDL_Color hc) {
        Button b; b.id=id; b.label=lbl; b.rect={x,y,w,h};
        b.color=c; b.hoverColor=hc; return b;
    };

    int y=5, bh=24;
    // Left group
    menuBtns.push_back(mb(BTN_NEW_PROJECT,"New",   6, y,52,bh,{70,120,170,255},{90,140,190,255}));
    menuBtns.push_back(mb(BTN_SAVE,      "Save",  62, y,52,bh,{40,110,40,255}, {60,140,60,255}));
    menuBtns.push_back(mb(BTN_LOAD,      "Load", 118, y,52,bh,{130,90,30,255},{155,110,50,255}));

    // Right group
    int rx = W-320;
    menuBtns.push_back(mb(BTN_RUN,  "Run",   rx,   y,70,bh,{34,177,76,255},{50,200,90,255}));
    menuBtns.push_back(mb(BTN_PAUSE,"Pause", rx+74,y,70,bh,{200,160,0,255},{220,180,20,255}));
    menuBtns.push_back(mb(BTN_STOP, "Stop",  rx+148,y,70,bh,{190,40,40,255},{215,60,60,255}));
    menuBtns.push_back(mb(BTN_TOGGLE_LOG,"Log",W-62,y,54,bh,{60,60,60,255},{85,85,85,255}));

    // Sprite bar
    Button ab; ab.id=BTN_ADD_SPRITE; ab.label="+Add";
    ab.rect={8,sprBar.rect.y+10,58,28};
    ab.color={60,120,170,255}; ab.hoverColor={80,140,190,255};
    sprBtns.push_back(ab);
}

// ─── render ──────────────────────────────────────────────────────────────────

void UIManager::render(SDL_Renderer* r, GameState& state) {
    // Menu bar
    SDL_SetRenderDrawColor(r,55,55,55,255);
    SDL_RenderFillRect(r,&menuBar.rect);
    for (auto& b:menuBtns) renderButton(r,b);

    // Palette + editor panels
    renderPanel(r,palPanel);
    renderPanel(r,edPanel);
    drawText(r,"Code Editor",edPanel.rect.x+6,edPanel.rect.y+6,{80,80,80,255});

    // Category tab bar inside palette (top of palette)
    renderCategoryTabs(r);

    // Scrollbar on right edge of palette
    renderScrollBar(r);

    // Stage border + white fill
    SDL_SetRenderDrawColor(r,160,160,160,255);
    SDL_Rect sb={stagePanel.rect.x-2,stagePanel.rect.y-2,
                 stagePanel.rect.w+4,stagePanel.rect.h+4};
    SDL_RenderFillRect(r,&sb);
    SDL_SetRenderDrawColor(r,255,255,255,255);
    SDL_RenderFillRect(r,&stagePanel.rect);

    // Sprite bar + log
    renderSprBar(r,state);
    if (logPanel.visible) renderLog(r);
}

void UIManager::renderCategoryTabs(SDL_Renderer* r) {
    // Tab bar sits just below palette top label
    const int TAB_Y  = palPanel.rect.y + 2;
    const int TAB_H  = CAT_TAB_H;
    const int PAL_X  = palPanel.rect.x;
    const int PAL_W  = palPanel.rect.w - 8; // leave room for scrollbar

    // Category names + colors (index 0 = ALL)
    struct Cat { const char* name; SDL_Color col; };
    Cat cats[] = {
        {"ALL",  {90,90,90,255}},
        {"Move", {76,151,255,255}},
        {"Look", {153,102,255,255}},
        {"Ctrl", {255,171,25,255}},
        {"Ops",  {89,203,94,255}},
        {"Vars", {255,140,26,255}},
        {"Pen",  {15,189,140,255}},
    };
    const int N = 7;
    int tabW = PAL_W / N;

    for (int i = 0; i < N; i++) {
        SDL_Rect tab = {PAL_X + i*tabW, TAB_Y, tabW, TAB_H};
        SDL_Color c  = cats[i].col;
        if (i == selectedCatTab) {
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
        } else {
            SDL_SetRenderDrawColor(r,
                (Uint8)(c.r*0.45f), (Uint8)(c.g*0.45f), (Uint8)(c.b*0.45f), 255);
        }
        SDL_RenderFillRect(r, &tab);

        // border
        SDL_SetRenderDrawColor(r, 30,30,30,255);
        SDL_RenderDrawRect(r, &tab);

        // label centred
        int tw = (int)strlen(cats[i].name)*6;
        int tx = tab.x + (tab.w - tw)/2;
        int ty = tab.y + (tab.h - 7)/2;
        drawText(r, cats[i].name, tx, ty, {255,255,255,255});
    }
}

void UIManager::renderScrollBar(SDL_Renderer* r) {
    // Scrollbar on right edge of palette panel
    const int SB_W   = 7;
    const int SB_X   = palPanel.rect.x + palPanel.rect.w - SB_W - 1;
    const int SB_Y   = palPanel.rect.y + CAT_TAB_H + 4;
    const int SB_H   = palPanel.rect.h - CAT_TAB_H - 6;

    // Track
    SDL_Rect track = {SB_X, SB_Y, SB_W, SB_H};
    SDL_SetRenderDrawColor(r, 210,210,210,255);
    SDL_RenderFillRect(r, &track);

    // Thumb
    int visibleH = palPanel.rect.h - CAT_TAB_H - 6;
    int totalH   = (palContentHeight > visibleH) ? palContentHeight : visibleH;
    float ratio  = (float)visibleH / totalH;
    int thumbH   = (int)(SB_H * ratio);
    if (thumbH < 20) thumbH = 20;
    float scrollRatio = (totalH > visibleH)
                        ? (float)palScrollY / (totalH - visibleH) : 0.f;
    int thumbY = SB_Y + (int)((SB_H - thumbH) * scrollRatio);

    SDL_Rect thumb = {SB_X, thumbY, SB_W, thumbH};
    SDL_SetRenderDrawColor(r, 140,140,140,255);
    SDL_RenderFillRect(r, &thumb);
}

void UIManager::renderPanel(SDL_Renderer* r, const UIPanel& p) {
    SDL_SetRenderDrawColor(r,p.bgColor.r,p.bgColor.g,p.bgColor.b,255);
    SDL_RenderFillRect(r,&p.rect);
    SDL_SetRenderDrawColor(r,190,190,190,255);
    SDL_RenderDrawRect(r,&p.rect);
}

void UIManager::renderButton(SDL_Renderer* r, const Button& b) {
    SDL_Color c = b.isHovered ? b.hoverColor : b.color;
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,255);
    SDL_RenderFillRect(r,&b.rect);
    SDL_SetRenderDrawColor(r,0,0,0,100);
    SDL_RenderDrawRect(r,&b.rect);
    // Centre text manually
    int tw = (int)b.label.size() * 6;
    int tx = b.rect.x + (b.rect.w - tw)/2;
    int ty = b.rect.y + (b.rect.h - 7)/2;
    drawText(r, b.label, tx, ty, {255,255,255,255});
}

void UIManager::renderLog(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r,25,25,25,255);
    SDL_RenderFillRect(r,&logPanel.rect);
    SDL_SetRenderDrawColor(r,70,70,70,255);
    SDL_RenderDrawRect(r,&logPanel.rect);

    // Title strip
    SDL_Rect ts={logPanel.rect.x,logPanel.rect.y,logPanel.rect.w,16};
    SDL_SetRenderDrawColor(r,45,45,45,255);
    SDL_RenderFillRect(r,&ts);
    drawText(r,"Console",logPanel.rect.x+4,logPanel.rect.y+4,{170,170,170,255});
    drawText(r,"Clear",logPanel.rect.x+logPanel.rect.w-34,logPanel.rect.y+4,{255,80,80,255});

    int lh=12, yp=logPanel.rect.y+20;
    int maxL=(logPanel.rect.h-22)/lh;
    int start=(int)logs.size()>maxL?(int)logs.size()-maxL:0;
    for (int i=start;i<(int)logs.size()&&yp<logPanel.rect.y+logPanel.rect.h-4;i++) {
        SDL_Color col={140,140,140,255};
        if      (logs[i].level=="ERROR")   col={255,90,90,255};
        else if (logs[i].level=="WARNING") col={255,200,60,255};
        else if (logs[i].level=="INFO")    col={70,190,255,255};
        std::string line="["+logs[i].level+"] "+logs[i].msg;
        if ((int)line.size()>40) line=line.substr(0,40)+"...";
        drawText(r,line,logPanel.rect.x+4,yp,col);
        yp+=lh;
    }
}

void UIManager::renderSprBar(SDL_Renderer* r,GameState& state) {
    SDL_SetRenderDrawColor(r,235,235,235,255);
    SDL_RenderFillRect(r,&sprBar.rect);
    SDL_SetRenderDrawColor(r,190,190,190,255);
    SDL_RenderDrawRect(r,&sprBar.rect);
    drawText(r,"Sprites",8,sprBar.rect.y+4,{80,80,80,255});

    for (auto& b:sprBtns) renderButton(r,b);

    int sx=76, sy=sprBar.rect.y+8, sz=78, gap=6;
    for (int i=0;i<spriteCount;i++) {
        if (i==selectedIdx) {
            SDL_SetRenderDrawColor(r,76,151,255,255);
            SDL_Rect hi={sx-2,sy-2,sz+4,sz+4};
            SDL_RenderFillRect(r,&hi);
        }
        SDL_Rect box={sx,sy,sz,sz};
        SDL_SetRenderDrawColor(r,255,255,255,255);
        SDL_RenderFillRect(r,&box);
        SDL_SetRenderDrawColor(r,170,170,170,255);
        SDL_RenderDrawRect(r,&box);
        // Mini sprite icon
        SDL_SetRenderDrawColor(r,76,151,255,180);
        SDL_Rect icon={sx+24,sy+16,30,30};
        SDL_RenderFillRect(r,&icon);

        std::string nm="Spr"+std::to_string(i+1);
        drawText(r,nm,sx+10,sy+sz-14,{60,60,60,255});
        sx+=sz+gap;
    }

    int vy = sprBar.rect.y + sprBar.rect.h-30;
    drawText(r,"Variables",sx,vy,{80,80,80,255});
    vy+=15;
    int vx = 8;
    for (auto& var : state.variables)
    {
        std::string text= var.first + ": " + var.second;
        drawText(r,text,sx,vy,{200,100,50,255});
        vx+=120;
        if (vx>300)
        {
            vx = 8;
            vy += 15;
        }
    }
}

// ─── input ───────────────────────────────────────────────────────────────────

void UIManager::handleMouseMove(int x,int y) {
    for (auto& b:menuBtns) b.isHovered=hit(x,y,b.rect);
    for (auto& b:sprBtns)  b.isHovered=hit(x,y,b.rect);
}

void UIManager::handleMouseWheel(int mx, int /*my*/, int deltaY) {
    // Only scroll when mouse is over palette
    if (mx >= palPanel.rect.x && mx < palPanel.rect.x + palPanel.rect.w) {
        palScrollY -= deltaY * PAL_SCROLL_STEP;
        // clamp
        int visH   = palPanel.rect.h - CAT_TAB_H - 6;
        int maxScroll = palContentHeight - visH;
        if (maxScroll < 0) maxScroll = 0;
        if (palScrollY < 0)         palScrollY = 0;
        if (palScrollY > maxScroll) palScrollY = maxScroll;
    }
}

void UIManager::handleMouseClick(int x,int y,bool down) {
    if (!down) return;

    // ── Category tabs ──────────────────────────────────────────────────────
    const int TAB_Y = palPanel.rect.y + 2;
    const int TAB_H = CAT_TAB_H;
    const int PAL_W = palPanel.rect.w - 8;
    const int N = 7;
    int tabW = PAL_W / N;
    if (y >= TAB_Y && y < TAB_Y + TAB_H &&
        x >= palPanel.rect.x && x < palPanel.rect.x + PAL_W) {
        int tab = (x - palPanel.rect.x) / tabW;
        if (tab >= 0 && tab < N) {
            selectedCatTab = tab;
            palScrollY = 0;   // jump to top on category change
            return;
        }
    }

    for (auto& b:menuBtns) { if(hit(x,y,b.rect)){b.isPressed=true;if(b.id==BTN_TOGGLE_LOG)toggleLogPanel();return;} }
    for (auto& b:sprBtns)  { if(hit(x,y,b.rect)){b.isPressed=true;return;} }

    // Sprite selection
    int sx=76,sy=sprBar.rect.y+8,sz=78,gap=6;
    for (int i=0;i<spriteCount;i++) {
        SDL_Rect box={sx,sy,sz,sz};
        if (hit(x,y,box)){selectedIdx=i;return;}
        sx+=sz+gap;
    }
    // Clear log button
    if (logPanel.visible) {
        SDL_Rect clr={logPanel.rect.x+logPanel.rect.w-38,logPanel.rect.y+2,36,14};
        if (hit(x,y,clr)) clearLogs();
    }
}

bool UIManager::isButtonPressed(int id) const {
    for (auto& b:menuBtns) if(b.id==id&&b.isPressed)return true;
    for (auto& b:sprBtns)  if(b.id==id&&b.isPressed)return true;
    return false;
}

void UIManager::resetButtons() {
    for (auto& b:menuBtns) b.isPressed=false;
    for (auto& b:sprBtns)  b.isPressed=false;
}

void UIManager::addLog(const std::string& msg,const std::string& level) {
    logs.push_back({msg,level});
    if ((int)logs.size()>80) logs.erase(logs.begin());
}
void UIManager::clearLogs()  { logs.clear(); }
void UIManager::toggleLogPanel() { logPanel.visible=!logPanel.visible; }

bool UIManager::hit(int x,int y,const SDL_Rect& r) const {
    return x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h;
}
