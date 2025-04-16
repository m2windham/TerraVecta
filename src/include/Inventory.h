#pragma once

#include <string>
#include <unordered_map>

class Inventory {
public:
    void addItem(const std::string& item, int quantity);
    void removeItem(const std::string& item, int quantity);
    int getItemCount(const std::string& item) const;
    std::string getSelectedItem() const; // Example placeholder

private:
    std::unordered_map<std::string, int> m_items;
};
