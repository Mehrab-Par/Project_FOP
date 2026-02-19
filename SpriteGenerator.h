#pragma once
#include <SDL2/SDL.h>
#include <string>

namespace SpriteGen {
    // Generate all sprite shapes and save them
    void generateAllSprites(SDL_Renderer* renderer);

    // Create a GPU texture for a named shape (circle/square/triangle/star/…)
    SDL_Texture* createTextureFor(SDL_Renderer* renderer, const std::string& shape);

    // Individual sprite generators
    SDL_Surface* createCircle(int size);
    SDL_Surface* createSquare(int size);
    SDL_Surface* createTriangle(int size);
    SDL_Surface* createStar(int size);
    SDL_Surface* createHexagon(int size);
    SDL_Surface* createPentagon(int size);
    SDL_Surface* createDiamond(int size);
    SDL_Surface* createArrow(int size);

    // Helper functions
    void drawFilledCircle(SDL_Surface* surface, int cx, int cy, int radius, SDL_Color color);
    void drawFilledPolygon(SDL_Surface* surface, const SDL_Point* points, int count, SDL_Color color);
    void setPixel(SDL_Surface* surface, int x, int y, SDL_Color color);
}
