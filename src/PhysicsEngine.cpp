#include "include/PhysicsEngine.h"
#include <iostream> // For basic logging
#include <btBulletDynamicsCommon.h>
#include <btIDebugDraw.h>

class DebugDrawer : public btIDebugDraw {
public:
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {
        // Implement line drawing logic (e.g., using OpenGL or another rendering system)
        // For now, this is a placeholder
    }

    void reportErrorWarning(const char* warningString) override {
        std::cerr << "Bullet Debug Warning: " << warningString << std::endl;
    }

    void draw3dText(const btVector3& location, const char* textString) override {
        // Optional: Implement 3D text rendering if needed
    }

    void setDebugMode(int debugMode) override {
        m_debugMode = debugMode;
    }

    int getDebugMode() const override {
        return m_debugMode;
    }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {
        // Placeholder implementation for drawing contact points
        // This can be extended to visualize contact points in the debug view
    }

private:
    int m_debugMode = btIDebugDraw::DBG_DrawWireframe;
};

PhysicsEngine::PhysicsEngine() {
    // Initialize Bullet Physics components
    m_collisionConfig = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfig);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfig);

    // Set default gravity (adjust as needed)
    m_dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

    // Set up debug drawer
    DebugDrawer* debugDrawer = new DebugDrawer();
    debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb);
    m_dynamicsWorld->setDebugDrawer(debugDrawer);

    std::cout << "PhysicsEngine: Debug drawer initialized with level 2." << std::endl;
    std::cout << "PhysicsEngine: Initialized Bullet Physics." << std::endl;
}

PhysicsEngine::~PhysicsEngine() {
    // Cleanup rigid bodies (important!)
    // Iterate backwards to safely remove bodies
    if (m_dynamicsWorld) {
        for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            m_dynamicsWorld->removeCollisionObject(obj);
            delete obj;
        }
    }

    // Clean up debug drawer
    if (m_dynamicsWorld && m_dynamicsWorld->getDebugDrawer()) {
        delete m_dynamicsWorld->getDebugDrawer();
    }

    // Delete Bullet components in reverse order of creation
    delete m_dynamicsWorld;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collisionConfig;

    std::cout << "PhysicsEngine: Cleaned up Bullet Physics." << std::endl;
}

void PhysicsEngine::addRigidBody(btRigidBody* body) {
    if (m_dynamicsWorld && body) {
        m_dynamicsWorld->addRigidBody(body);
        // std::cout << "PhysicsEngine: Added rigid body." << std::endl; // Optional log
    }
}

void PhysicsEngine::stepSimulation(float deltaTime) {
    if (m_dynamicsWorld) {
        // Use a fixed time step for stability if possible, or clamp deltaTime
        // Max substeps = 10, fixed timestep = 1/60.0f (adjust as needed)
        m_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
    }
}

// TODO: Implement methods for managing chunk collision shapes
// void PhysicsEngine::addChunkShape(...) { ... }
// void PhysicsEngine::removeChunkShape(...) { ... }
// void PhysicsEngine::updateChunkShape(...) { ... }
