#include "include/Inventory.h"
#include <iostream> // For basic logging

void Inventory::addItem(const std::string& item, int quantity) {
    m_items[item] += quantity;
    std::cout << "Inventory: Added " << quantity << " " << item << ". Total: " << m_items[item] << std::endl;
}

void Inventory::removeItem(const std::string& item, int quantity) {
    auto it = m_items.find(item);
    if (it != m_items.end()) {
        it->second -= quantity;
        std::cout << "Inventory: Removed " << quantity << " " << item << ". Remaining: " << it->second << std::endl;
        if (it->second <= 0) {
            m_items.erase(it);
            std::cout << "Inventory: Removed item " << item << " completely." << std::endl;
        }
    } else {
        std::cout << "Inventory: Tried to remove " << item << ", but not found." << std::endl;
    }
}

int Inventory::getItemCount(const std::string& item) const {
    auto it = m_items.find(item);
    return (it != m_items.end()) ? it->second : 0;
}

// Example placeholder implementation
std::string Inventory::getSelectedItem() const {
    // TODO: Implement actual item selection logic (e.g., based on a hotbar)
    if (!m_items.empty()) {
        return m_items.begin()->first; // Just return the first item for now
    }
    return ""; // Return empty string if inventory is empty
}
