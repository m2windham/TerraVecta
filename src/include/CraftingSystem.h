#pragma once

#include <string>
#include <unordered_map>

// Forward declaration
class Inventory;

class CraftingSystem {
public:
    void addRecipe(const std::string& result, const std::unordered_map<std::string, int>& ingredients);
    bool craftItem(const std::string& result, Inventory& inventory);

private:
    std::unordered_map<std::string, std::unordered_map<std::string, int>> m_recipes;
};
