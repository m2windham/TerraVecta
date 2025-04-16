#include <glm/gtc/matrix_transform.hpp>

// Frustum class for view culling
class Frustum {
private:
    enum Planes { Left, Right, Bottom, Top, Near, Far, Count };
    glm::vec4 m_planes[Planes::Count]; // Frustum planes in world space

public:
    // Update frustum planes from view-projection matrix
    void updateFromVPMatrix(const glm::mat4& viewProjection) {
        // Extract planes from view-projection matrix
        // Left plane
        m_planes[Planes::Left].x = viewProjection[0][3] + viewProjection[0][0];
        m_planes[Planes::Left].y = viewProjection[1][3] + viewProjection[1][0];
        m_planes[Planes::Left].z = viewProjection[2][3] + viewProjection[2][0];
        m_planes[Planes::Left].w = viewProjection[3][3] + viewProjection[3][0];
        
        // Right plane
        m_planes[Planes::Right].x = viewProjection[0][3] - viewProjection[0][0];
        m_planes[Planes::Right].y = viewProjection[1][3] - viewProjection[1][0];
        m_planes[Planes::Right].z = viewProjection[2][3] - viewProjection[2][0];
        m_planes[Planes::Right].w = viewProjection[3][3] - viewProjection[3][0];
        
        // Bottom plane
        m_planes[Planes::Bottom].x = viewProjection[0][3] + viewProjection[0][1];
        m_planes[Planes::Bottom].y = viewProjection[1][3] + viewProjection[1][1];
        m_planes[Planes::Bottom].z = viewProjection[2][3] + viewProjection[2][1];
        m_planes[Planes::Bottom].w = viewProjection[3][3] + viewProjection[3][1];
        
        // Top plane
        m_planes[Planes::Top].x = viewProjection[0][3] - viewProjection[0][1];
        m_planes[Planes::Top].y = viewProjection[1][3] - viewProjection[1][1];
        m_planes[Planes::Top].z = viewProjection[2][3] - viewProjection[2][1];
        m_planes[Planes::Top].w = viewProjection[3][3] - viewProjection[3][1];
        
        // Near plane
        m_planes[Planes::Near].x = viewProjection[0][3] + viewProjection[0][2];
        m_planes[Planes::Near].y = viewProjection[1][3] + viewProjection[1][2];
        m_planes[Planes::Near].z = viewProjection[2][3] + viewProjection[2][2];
        m_planes[Planes::Near].w = viewProjection[3][3] + viewProjection[3][2];
        
        // Far plane
        m_planes[Planes::Far].x = viewProjection[0][3] - viewProjection[0][2];
        m_planes[Planes::Far].y = viewProjection[1][3] - viewProjection[1][2];
        m_planes[Planes::Far].z = viewProjection[2][3] - viewProjection[2][2];
        m_planes[Planes::Far].w = viewProjection[3][3] - viewProjection[3][2];
        
        // Normalize all planes
        for (int i = 0; i < Planes::Count; i++) {
            float length = sqrtf(m_planes[i].x * m_planes[i].x + 
                               m_planes[i].y * m_planes[i].y + 
                               m_planes[i].z * m_planes[i].z);
            m_planes[i] /= length;
        }
    }
    
    // Check if a point is inside the frustum
    bool isPointVisible(const glm::vec3& point) const {
        for (int i = 0; i < Planes::Count; i++) {
            if (m_planes[i].x * point.x + 
                m_planes[i].y * point.y + 
                m_planes[i].z * point.z + 
                m_planes[i].w <= 0) {
                return false;
            }
        }
        return true;
    }
    
    // Check if a sphere is inside or intersects the frustum
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (int i = 0; i < Planes::Count; i++) {
            float distance = m_planes[i].x * center.x + 
                           m_planes[i].y * center.y + 
                           m_planes[i].z * center.z + 
                           m_planes[i].w;
            if (distance <= -radius) {
                return false;
            }
        }
        return true;
    }
    
    // Check if an axis-aligned bounding box is inside or intersects the frustum
    bool isAABBVisible(const glm::vec3& min, const glm::vec3& max) const {
        // Check each plane
        for (int i = 0; i < Planes::Count; i++) {
            glm::vec3 pVertex = min; // Positive vertex (closest to normal)
            glm::vec3 nVertex = max; // Negative vertex (farthest from normal)
            
            // Select positive vertex based on plane normal
            if (m_planes[i].x >= 0) {
                pVertex.x = max.x;
                nVertex.x = min.x;
            }
            if (m_planes[i].y >= 0) {
                pVertex.y = max.y;
                nVertex.y = min.y;
            }
            if (m_planes[i].z >= 0) {
                pVertex.z = max.z;
                nVertex.z = min.z;
            }
            
            // If positive vertex is outside, the box is outside the frustum
            float dp = m_planes[i].x * pVertex.x + 
                     m_planes[i].y * pVertex.y + 
                     m_planes[i].z * pVertex.z + 
                     m_planes[i].w;
            
            if (dp < 0) {
                return false;
            }
        }
        
        return true;
    }
};