#pragma once
#include "GameState.h"
#include <string>


namespace SaveLoad
{
    bool saveProject(GameState& state, const std::string& filename);
    bool loadProject(GameState& state, const std::string& filename);
    void newProject(GameState& state);


    std::string serializeSprite(const Sprite& sprite);
    bool        deserializeSprite(GameState& state, const std::string& data, Sprite& sprite);

    std::string serializeBlock(const Block& block);
    Block*      deserializeBlock(const std::string& data);

    std::string serializeVariables(GameState& state);
    void        deserializeVariables(GameState& state, const std::string& data);
}