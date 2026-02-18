#include "SaveLoad.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace SaveLoad {
    
void logInfo(GameState& state, const std::string& action, const std::string& msg) {
    LogEntry entry;
    entry.category = "SaveLoad";
    entry.action = action;
    entry.message = msg;
    state.logs.push_back(entry);
}


// SAVE 

bool saveProject(GameState& state, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SaveLoad] Failed to open file for writing: " << filename << std::endl;
        return false;
    }

// Header
    file << "[HEADER]\n";
    file << "version|1.0\n";
    file << "spriteCount|" << state.sprites.size() << "\n";
    file << "backdropCount|" << state.backdrops.size() << "\n";
    file << "currentBackdrop|" << state.currentBackdrop << "\n";
    file << "variableCount|" << state.variables.size() << "\n";
    file << "penExtensionEnabled|" << (state.penExtensionEnabled ? 1 : 0) << "\n";
    file << "activeSpriteIndex|" << state.activeSpriteIndex << "\n";

// Sprites Data
    for (auto* sprite : state.sprites) {
        file << "\n[SPRITE:" << sprite->name << "]\n";
        
        file << sprite->x << "|" << sprite->y << "|"
             << sprite->direction << "|" << sprite->size << "|"
             << (sprite->visible ? 1 : 0) << "|"
             << sprite->layer << "|"
             << (sprite->draggable ? 1 : 0) << "|"
             << sprite->currentCostume << "\n";

 // Pen
        file << (sprite->penDown ? 1 : 0) << "|"
             << (int)sprite->penColor.r << "|" << (int)sprite->penColor.g << "|"
             << (int)sprite->penColor.b << "|" << sprite->penSize << "\n";

// Speech
        file << "\"" << sprite->speechText << "\"|" << (sprite->isThinker ? 1 : 0) << "\n";

// Blocks
        file << "blockCount|" << sprite->script.size() << "\n";
        for (auto* block : sprite->script) {
            file << "[BLOCK]" << serializeBlock(*block) << "\n";
        }
    }

// Global Variables
    file << "\n[VARIABLES]\n";
    file << serializeVariables(state) << "\n";
    file.close();
    logInfo(state, "SAVE", "Project saved to " + filename);
    std::cout << "[SaveLoad] Project saved to: " << filename << std::endl;
    return true;
}


// LOAD 

bool loadProject(GameState& state, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SaveLoad] Failed to open file for reading: " << filename << std::endl;
        return false;
    }

// reset state
    newProject(state);

    std::string line;
    std::string currentSection = "";
    Sprite* currentSprite = nullptr;
    int expectedBlocks = 0;
    int readBlocks = 0;

    while (std::getline(file, line)) {
// Trim whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) continue;

// Detect Sections
        if (line == "[HEADER]") { currentSection = "HEADER"; continue; }
        if (line == "[VARIABLES]") { currentSection = "VARIABLES"; continue; }
        if (line.substr(0, 8) == "[SPRITE:") {
            currentSection = "SPRITE";
            std::string name = line.substr(8, line.size() - 9); // remove ending ]
            currentSprite = new Sprite();
            currentSprite->name = name;
            state.sprites.push_back(currentSprite);
            readBlocks = 0;
            expectedBlocks = 0;
            continue;
        }

//Header
        if (currentSection == "HEADER") {
            size_t pipe = line.find('|');
            if (pipe == std::string::npos) continue;
            std::string key = line.substr(0, pipe);
            std::string val = line.substr(pipe + 1);

            if (key == "activeSpriteIndex") state.activeSpriteIndex = std::stoi(val);
            else if (key == "penExtensionEnabled") state.penExtensionEnabled = (val == "1");
            else if (key == "currentBackdrop") state.currentBackdrop = std::stoi(val);
        }
//  Sprite Data
        else if (currentSection == "SPRITE" && currentSprite)
        {
//block lines first
            if (line.substr(0, 10) == "blockCount")
            {
                expectedBlocks = std::stoi(line.substr(11));
                readBlocks=0;
                continue;
            }
            if (line.substr(0, 7) == "[BLOCK]" && readBlocks < expectedBlocks)
            {
                std::string blockData = line.substr(7);
                Block* block = deserializeBlock(blockData);
                
                if (block)
                    currentSprite->script.push_back(block);
                
                readBlocks++;
                continue;
            }

            // sprite 
            if (line.find('|') != std::string::npos)
            {
                std::istringstream ss(line);
                std::string token;
                std::vector<std::string> tokens;
                while (std::getline(ss, token, '|')) tokens.push_back(token);

                // Physical
                if (tokens.size() == 8)
                {
                    currentSprite->x              = std::stof(tokens[0]);
                    currentSprite->y              = std::stof(tokens[1]);
                    currentSprite->direction      = std::stof(tokens[2]);
                    currentSprite->size           = std::stof(tokens[3]);
                    currentSprite->visible        = (tokens[4] == "1");
                    currentSprite->layer          = std::stoi(tokens[5]);
                    currentSprite->draggable      = (tokens[6] == "1");
                    currentSprite->currentCostume = std::stoi(tokens[7]);
                    // Set init values
                    currentSprite->initX = currentSprite->x;
                    currentSprite->initY = currentSprite->y;
                    currentSprite->initDirection = currentSprite->direction;
                    currentSprite->initSize = currentSprite->size;
                    currentSprite->initCostume = currentSprite->currentCostume;
                }
                // Pen 
                else if (tokens.size() == 5)
                {
                    currentSprite->penDown= (tokens[0] == "1");
                    currentSprite->penColor.r= (Uint8)std::stoi(tokens[1]);
                    currentSprite->penColor.g= (Uint8)std::stoi(tokens[2]);
                    currentSprite->penColor.b= (Uint8)std::stoi(tokens[3]);
                    currentSprite->penSize= std::stoi(tokens[4]);
                }
                // Speech Properties (2 fields)
                else if (tokens.size()==2)
                {
                    std::string text = tokens[0];
                    
                    // Remove quotes
                    if (text.size() >1&& text.front() == '"' && text.back() == '"') 
                        text = text.substr(1, text.size() - 2);
                    
                    currentSprite->speechText = text;
                    currentSprite->isThinker = (tokens[1] == "1");
                }
            }
        }
        
        else if (currentSection == "VARIABLES") 
            deserializeVariables(state, line);
        
    }

    file.close();
    logInfo(state, "LOAD", "Project loaded from " + filename);
    std::cout << "[SaveLoad] Project loaded from: " << filename << std::endl;
    return true;
}
