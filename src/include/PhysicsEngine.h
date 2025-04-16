#pragma once

// Include Bullet Physics headers
#include <btBulletDynamicsCommon.h>

class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();

    void addRigidBody(btRigidBody* body);
    void stepSimulation(float deltaTime);
    // TODO: Add methods for managing chunk collision shapes
    // void addChunkShape(const glm::ivec2& chunkPos, ...);
    // void removeChunkShape(const glm::ivec2& chunkPos);
    // void updateChunkShape(const glm::ivec2& chunkPos, ...);

private:
    // Using pointers for Bullet objects, managed in constructor/destructor
    btDefaultCollisionConfiguration* m_collisionConfig = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btBroadphaseInterface* m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver* m_solver = nullptr;
    btDiscreteDynamicsWorld* m_dynamicsWorld = nullptr;
};
