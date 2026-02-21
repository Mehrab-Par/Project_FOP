#include "SpriteGenerator.h"
#include "Logger.h"
#include <SDL2/SDL_image.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <sys/stat.h>   // mkdir
#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#define mkdir(p,m) _mkdir(p)
#endif

namespace SpriteGen {

void setPixel(SDL_Surface* surface, int x, int y, SDL_Color color) {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;
    
    Uint32 pixel = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    Uint32* pixels = (Uint32*)surface->pixels;
    pixels[y * surface->w + x] = pixel;
}

void drawFilledCircle(SDL_Surface* surface, int cx, int cy, int radius, SDL_Color color) {
    SDL_LockSurface(surface);
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                setPixel(surface, cx + x, cy + y, color);
            }
        }
    }
    
    SDL_UnlockSurface(surface);
}

void drawFilledPolygon(SDL_Surface* surface, const SDL_Point* points, int count, SDL_Color color) {
    if (count < 3) return;
    
    SDL_LockSurface(surface);
    
    // Find bounding box
    int minX = surface->w, maxX = 0, minY = surface->h, maxY = 0;
    for (int i = 0; i < count; i++) {
        minX = std::min(minX, points[i].x);
        maxX = std::max(maxX, points[i].x);
        minY = std::min(minY, points[i].y);
        maxY = std::max(maxY, points[i].y);
    }
    
    // Scanline fill
    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections;
        
        for (int i = 0; i < count; i++) {
            int j = (i + 1) % count;
            int y1 = points[i].y;
            int y2 = points[j].y;
            
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                int x1 = points[i].x;
                int x2 = points[j].x;
                int x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
                intersections.push_back(x);
            }
        }
        
        std::sort(intersections.begin(), intersections.end());
        
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            for (int x = intersections[i]; x <= intersections[i + 1]; x++) {
                setPixel(surface, x, y, color);
            }
        }
    }
    
    SDL_UnlockSurface(surface);
}

SDL_Surface* createCircle(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    // Red circle
    drawFilledCircle(surface, size/2, size/2, size/2 - 5, {255, 0, 0, 255});
    
    // Darker outline
    SDL_LockSurface(surface);
    int cx = size / 2;
    int cy = size / 2;
    int r = size / 2 - 5;
    
    for (int angle = 0; angle < 360; angle++) {
        for (int t = 0; t < 3; t++) {
            int x = cx + (r - t) * cos(angle * M_PI / 180.0);
            int y = cy + (r - t) * sin(angle * M_PI / 180.0);
            setPixel(surface, x, y, {180, 0, 0, 255});
        }
    }
    SDL_UnlockSurface(surface);
    
    return surface;
}

SDL_Surface* createSquare(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    int margin = 10;
    SDL_Rect rect = {margin, margin, size - 2*margin, size - 2*margin};
    
    // Blue square
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, 0, 100, 255, 255));
    
    // Outline
    SDL_LockSurface(surface);
    for (int t = 0; t < 3; t++) {
        for (int x = margin; x < size - margin; x++) {
            setPixel(surface, x, margin + t, {0, 70, 200, 255});
            setPixel(surface, x, size - margin - t - 1, {0, 70, 200, 255});
        }
        for (int y = margin; y < size - margin; y++) {
            setPixel(surface, margin + t, y, {0, 70, 200, 255});
            setPixel(surface, size - margin - t - 1, y, {0, 70, 200, 255});
        }
    }
    SDL_UnlockSurface(surface);
    
    return surface;
}

SDL_Surface* createTriangle(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    SDL_Point points[3] = {
        {size / 2, 10},
        {size - 10, size - 10},
        {10, size - 10}
    };
    
    // Green triangle
    drawFilledPolygon(surface, points, 3, {0, 200, 0, 255});
    
    return surface;
}

SDL_Surface* createStar(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    float cx = size / 2.0f;
    float cy = size / 2.0f;
    float outerR = size / 2.0f - 8;
    float innerR = outerR * 0.4f;
    
    SDL_Point points[10];
    for (int i = 0; i < 10; i++) {
        float angle = M_PI / 2 + (2 * M_PI * i) / 10;
        float r = (i % 2 == 0) ? outerR : innerR;
        points[i].x = (int)(cx + r * cos(angle));
        points[i].y = (int)(cy - r * sin(angle));
    }
    
    // Yellow star
    drawFilledPolygon(surface, points, 10, {255, 215, 0, 255});
    
    return surface;
}

SDL_Surface* createHexagon(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    float cx = size / 2.0f;
    float cy = size / 2.0f;
    float r = size / 2.0f - 10;
    
    SDL_Point points[6];
    for (int i = 0; i < 6; i++) {
        float angle = M_PI / 6 + (2 * M_PI * i) / 6;
        points[i].x = (int)(cx + r * cos(angle));
        points[i].y = (int)(cy + r * sin(angle));
    }
    
    // Purple hexagon
    drawFilledPolygon(surface, points, 6, {160, 0, 200, 255});
    
    return surface;
}

SDL_Surface* createPentagon(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    float cx = size / 2.0f;
    float cy = size / 2.0f;
    float r = size / 2.0f - 10;
    
    SDL_Point points[5];
    for (int i = 0; i < 5; i++) {
        float angle = M_PI / 2 + (2 * M_PI * i) / 5;
        points[i].x = (int)(cx + r * cos(angle));
        points[i].y = (int)(cy - r * sin(angle));
    }
    
    // Orange pentagon
    drawFilledPolygon(surface, points, 5, {255, 140, 0, 255});
    
    return surface;
}

SDL_Surface* createDiamond(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    SDL_Point points[4] = {
        {size / 2, 10},
        {size - 10, size / 2},
        {size / 2, size - 10},
        {10, size / 2}
    };
    
    // Pink diamond
    drawFilledPolygon(surface, points, 4, {255, 100, 200, 255});
    
    return surface;
}

SDL_Surface* createArrow(int size) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, size, size, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
    int cy = size / 2;
    SDL_Point points[7] = {
        {10, cy - 8},
        {10, cy + 8},
        {size - 25, cy + 8},
        {size - 25, cy + 20},
        {size - 5, cy},
        {size - 25, cy - 20},
        {size - 25, cy - 8}
    };
    
    // Cyan arrow
    drawFilledPolygon(surface, points, 7, {0, 200, 200, 255});
    
    return surface;
}

void generateAllSprites(SDL_Renderer* renderer) {
    // Ensure assets directory exists before saving PNGs
    mkdir("assets", 0755);
    Logger::info("Generating sprite assets...");
    // We store generated textures in a global map so loadAssets can retrieve them
    // Nothing to do here – textures are generated on-demand via createTextureFor()
    (void)renderer;
}

// Create a SDL_Texture directly from a generated surface (used by loadAssets)
SDL_Texture* createTextureFor(SDL_Renderer* renderer, const std::string& shape) {
    const int SIZE = 80;
    SDL_Surface* surface = nullptr;

    if      (shape == "circle")   surface = createCircle(SIZE);
    else if (shape == "square")   surface = createSquare(SIZE);
    else if (shape == "triangle") surface = createTriangle(SIZE);
    else if (shape == "star")     surface = createStar(SIZE);
    else if (shape == "hexagon")  surface = createHexagon(SIZE);
    else if (shape == "pentagon") surface = createPentagon(SIZE);
    else if (shape == "diamond")  surface = createDiamond(SIZE);
    else if (shape == "arrow")    surface = createArrow(SIZE);
    else                          surface = createCircle(SIZE);  // default

    if (!surface) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return tex;
}

} // namespace SpriteGen
