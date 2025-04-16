#pragma once

#include <glm/glm.hpp>

// Add comparison operators for glm::ivec2
namespace glm {
    inline bool operator<(const glm::ivec2& lhs, const glm::ivec2& rhs) {
        // First compare x, then y as a tie-breaker
        return (lhs.x < rhs.x) || (lhs.x == rhs.x && lhs.y < rhs.y);
    }
    
    inline bool operator>(const glm::ivec2& lhs, const glm::ivec2& rhs) {
        return rhs < lhs;
    }
    
    inline bool operator<=(const glm::ivec2& lhs, const glm::ivec2& rhs) {
        return !(rhs < lhs);
    }
    
    inline bool operator>=(const glm::ivec2& lhs, const glm::ivec2& rhs) {
        return !(lhs < rhs);
    }
    
}