#include "include/CraftingSystem.h"
#include "include/Inventory.h" // Updated include path
#include <iostream> // For basic logging

void CraftingSystem::addRecipe(const std::string& result, const std::unordered_map<std::string, int>& ingredients) {
    m_recipes[result] = ingredients;
    std::cout << "Crafting: Added recipe for " << result << std::endl;
}

bool CraftingSystem::craftItem(const std::string& result, Inventory& inventory) {
    auto it = m_recipes.find(result);
    if (it == m_recipes.end()) {
        std::cout << "Crafting: Recipe for " << result << " not found." << std::endl;
        return false; // Recipe not found
    }

    // Check if inventory has all required ingredients
    for (const auto& [ingredient, quantity] : it->second) {
        if (inventory.getItemCount(ingredient) < quantity) {
            std::cout << "Crafting: Not enough " << ingredient << " to craft " << result << ". Need " << quantity << ", have " << inventory.getItemCount(ingredient) << std::endl;
            return false; // Not enough ingredients
        }
    }

    // Deduct ingredients from inventory
    for (const auto& [ingredient, quantity] : it->second) {
        inventory.removeItem(ingredient, quantity);
    }

    // Add crafted item to inventory
    inventory.addItem(result, 1); // Assuming recipes always yield 1 result item
    std::cout << "Crafting: Successfully crafted " << result << "." << std::endl;
    return true;
}
