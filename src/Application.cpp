// h:\Repo\TerraVecta\src\Application.cpp

#include "include/Application.h"
#include "include/Window.h"
#include "include/Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <memory>
#include <random>
#include <string>
#include "FastNoiseLite.h"
#include "include/VoxelChunk.h"
#include <unordered_set>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <cmath>
#include <limits>

// --- Include Actual Game System Headers ---
#include "include/Inventory.h"
#include "include/CraftingSystem.h"
#include "include/PhysicsEngine.h"
#include "include/GameUI.h"
#include "include/TextureAtlas.h"
#include "include/BiomeManager.h"
#include "include/WaterSystem.h"
#include <bullet/Bullet3Collision/NarrowPhaseCollision/b3ConvexUtility.h>

// --- OpenGL Error Checking Utility ---
GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    // Loop while there are errors to fetch
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            // REMOVED: case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break; // Deprecated
            // REMOVED: case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break; // Deprecated
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            // Add other potential errors if using newer OpenGL versions (e.g., GL_CONTEXT_LOST)
            default:                               error = "UNKNOWN_ERROR"; break;
        }
        std::cerr << "OpenGL Error (" << error << " - " << errorCode << ") | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode; // Returns GL_NO_ERROR if everything was okay
}
// Macro to automatically insert file and line number into the error check call
#define glCheckError() glCheckError_(__FILE__, __LINE__)


// Definition moved before initialize
GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// --- Constructor ---
// Initializes the window and calls the main initialization logic.
Application::Application(int windowWidth, int windowHeight, const char* windowTitle)
    : // Remove m_SquareVAO/VBO initialization
      m_IsRunning(false), m_LastFrameTime(0.0f), m_textureID(0), // Initialize texture ID
      m_cameraPosition(0.0f, CHUNK_SIZE + 10.0f, CHUNK_SIZE + 10.0f), // Position camera higher and further away to see the terrain
      m_cameraYaw(-45.0f), m_cameraPitch(-30.0f), // Look down at the terrain
      m_cameraSpeed(5.0f), m_mouseSensitivity(0.1f),
      m_lastMouseX(static_cast<double>(windowWidth) / 2.0), // Center mouse initially
      m_lastMouseY(static_cast<double>(windowHeight) / 2.0),
      m_leftMouseDown(false), m_rightMouseDown(false), // Initialize mouse state
      // Initialize game systems using unique_ptr (assumes headers are included)
      m_inventory(std::make_unique<Inventory>()),
      m_craftingSystem(std::make_unique<CraftingSystem>()),
      m_physicsEngine(std::make_unique<PhysicsEngine>()),
      m_biomeManager(nullptr),
      m_textureAtlas(nullptr),
      m_waterSystem(nullptr),
      // Initialize seed with a random value
      m_seed(std::random_device{}()),
      // Initialize random number generator
      m_randomGenerator(m_seed),
      m_noiseDistribution(-1.0f, 1.0f),
      m_gameUI(nullptr)
{
    try {
        // Create the window using smart pointer
        m_Window = std::make_unique<Window>(windowWidth, windowHeight, windowTitle);

        // Set cursor mode for FPS camera AFTER window creation
        if (m_Window) {
            glfwSetInputMode(m_Window->getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // Get initial cursor position (might be redundant if set above, but safe)
            glfwGetCursorPos(m_Window->getNativeWindow(), &m_lastMouseX, &m_lastMouseY);
            std::cout << "Window created and cursor mode set." << std::endl;
        } else {
             throw std::runtime_error("Window creation failed.");
        }


        // Call the main initialization sequence for OpenGL resources, shaders, etc.
        if (!initialize()) {
            // If initialize() signals failure, throw an exception to halt construction
            throw std::runtime_error("Application::initialize() failed.");
        }

        // Only set running state if all initialization succeeded
        m_IsRunning = true;
        std::cout << "Application construction successful. Ready to run." << std::endl;

    } catch (const std::runtime_error& e) {
        // Catch errors from Window creation or initialize()
        std::cerr << "Application failed to initialize during construction: " << e.what() << std::endl;
        m_IsRunning = false; // Ensure application won't try to run
        // Depending on design, you might want to re-throw 'e' here
        // throw;
    } catch (const std::exception& e) {
        // Catch other standard exceptions
        std::cerr << "Application failed during construction (std::exception): " << e.what() << std::endl;
        m_IsRunning = false;
    } catch (...) {
        // Catch any other unknown exceptions
        std::cerr << "Application failed during construction (unknown exception)." << std::endl;
        m_IsRunning = false;
    }
}

// --- Destructor ---
// Calls the shutdown logic to clean up resources.
Application::~Application() {
    shutdown(); // Ensure cleanup happens when Application object goes out of scope
}

// --- initialize ---
// Sets up OpenGL state, loads shaders, creates geometry (VAO/VBO).
bool Application::initialize() {
    // Set initial OpenGL state for 3D Voxel Scene
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Dark grey-blue background
    glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D
    glEnable(GL_CULL_FACE);  // Enable face culling
    glCullFace(GL_BACK);     // Cull back faces
    glCheckError(); // Check for errors after setting initial state
    std::cout << "OpenGL state initialized (Depth Test, Face Culling enabled)." << std::endl;

    // --- Load Shaders ---
    try {
        // Ensure paths are correct relative to the executable's working directory
        m_ShaderProgram = std::make_unique<Shader>(
            "../../../shaders/simp.vert",
            "../../../shaders/simple.frag"
        );

        if (!m_ShaderProgram || m_ShaderProgram->getID() == 0) {
             std::cerr << "ERROR::APPLICATION::INITIALIZE: Failed to load or link shaders (Shader ID is 0)." << std::endl;
             return false;
        }
        std::cout << "Shaders loaded and linked successfully." << std::endl;
        glCheckError();

    } catch (const std::exception& e) {
        std::cerr << "ERROR::APPLICATION::INITIALIZE: Exception while loading shaders: " << e.what() << std::endl;
        return false;
    }

    // --- Remove Square Vertex Data Setup ---
    // The square VAO/VBO are no longer needed. Cleanup is handled in shutdown().

    // --- Initialize Voxel System ---
    std::cout << "Initializing Voxel System..." << std::endl;
    initializeVoxelSystem(); // Load initial chunk(s)
    glCheckError();
    std::cout << "Voxel System Initialized." << std::endl;

    // --- Initialize Physics ---
    // m_physicsEngine is already created in the constructor via unique_ptr
    // Add ground plane or initial chunk collision shapes here (TODO)
    if (m_physicsEngine) {
        std::cout << "Physics engine instance created." << std::endl;
        // Example: Add static ground plane (if desired)
        // btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0); // Y-up plane at Y=0
        // btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
        // btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
        // btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
        // m_physicsEngine->addRigidBody(groundRigidBody);
    } else {
        std::cerr << "WARNING::APPLICATION::INITIALIZE: Physics engine pointer is null after construction." << std::endl;
    }
    glCheckError();

    // --- Initialize Inventory & Crafting ---
    // m_inventory and m_craftingSystem are already created via unique_ptr
    if (m_inventory && m_craftingSystem) {
        // Add initial items or recipes if needed
        m_craftingSystem->addRecipe("Wooden Plank", {{"Wood Log", 1}}); // Example recipe
        m_inventory->addItem("Wood Log", 10); // Example starting item
        std::cout << "Inventory and Crafting systems initialized with example data." << std::endl;
    } else {
         std::cerr << "WARNING::APPLICATION::INITIALIZE: Inventory or Crafting system pointer is null after construction." << std::endl;
    }
    glCheckError();

    // --- Initialize Biome System ---
    std::cout << "Initializing Biome System..." << std::endl;
    m_biomeManager = std::make_unique<BiomeManager>();
    std::cout << "Biome System Initialized." << std::endl;
    glCheckError();

    // --- Initialize Texture Atlas ---
    std::cout << "Initializing Texture Atlas..." << std::endl;
    m_textureAtlas = std::make_unique<TextureAtlas>();
    
    if (!m_textureAtlas->initialize("../../../assets/textures/block_atlas.png", 16)) {
        std::cerr << "WARNING: Failed to load texture atlas, falling back to single texture" << std::endl;
        // Fall back to the old texture loading method
        m_textureID = loadTexture("../../../assets/textures/grass.png");
    } else {
        // Register block types with their textures
        // Format: blockType, {top, bottom, left, right, front, back} indices in the atlas
        // If less than 6 indices are provided, the first one is used for all faces
        m_textureAtlas->registerBlock(0, {0}); // Air (not rendered, but for completeness)
        m_textureAtlas->registerBlock(1, {0, 2, 1, 1, 1, 1}); // Grass block (top, bottom, sides)
        m_textureAtlas->registerBlock(2, {2}); // Dirt
        m_textureAtlas->registerBlock(3, {3}); // Water
        m_textureAtlas->registerBlock(4, {4}); // Sand
        m_textureAtlas->registerBlock(5, {5}); // Stone
        m_textureAtlas->registerBlock(6, {6}); // Snow
        m_textureAtlas->registerBlock(7, {7}); // Wood
        m_textureAtlas->registerBlock(8, {8}); // Leaves
        
        // Store the texture ID for use in shaders
        m_textureID = m_textureAtlas->getTextureID();
        std::cout << "Texture Atlas initialized and block types registered." << std::endl;
    }
    glCheckError();

    // --- Initialize Water System ---
    std::cout << "Initializing Water System..." << std::endl;
    m_waterSystem = std::make_unique<WaterSystem>();
    m_waterSystem->setApplication(this);
    m_waterSystem->initialize(static_cast<int>(m_seed));
    std::cout << "Water System Initialized." << std::endl;
    glCheckError();

    // --- Initialize Game UI ---
    std::cout << "Initializing Game UI..." << std::endl;
    m_gameUI = std::make_unique<GameUI>();
    m_gameUI->setApplication(this);
    
    if (!m_gameUI->initialize()) {
        std::cerr << "WARNING: Failed to initialize Game UI, continuing without UI" << std::endl;
    } else {
        std::cout << "Game UI initialized." << std::endl;
    }
    glCheckError();

    // --- Final Setup ---
    m_LastFrameTime = static_cast<float>(glfwGetTime()); // Initialize frame timer

    // Load texture using the correct relative path
    m_textureID = loadTexture("../../../assets/textures/grass.png"); // Store texture ID
    if (m_textureID == 0) {
        std::cerr << "ERROR::APPLICATION::INITIALIZE: Failed to load texture '../../../assets/textures/grass.png'." << std::endl;
        
        // Create a default texture if loading fails
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        
        // Create a simple colored texture (2x2 pink/black checkerboard for visibility)
        unsigned char checkerData[16] = {
            255, 0, 255, 255,   0, 0, 0, 255,
            0, 0, 0, 255,   255, 0, 255, 255
        };
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        std::cout << "Created fallback checkerboard texture (ID: " << m_textureID << ")" << std::endl;
    } else {
         std::cout << "Texture loaded successfully (ID: " << m_textureID << ")" << std::endl;
    }
    glCheckError();


    // Bind the texture and set uniform (can be done once if only one texture unit is used)
    if (m_ShaderProgram && m_textureID != 0) {
        m_ShaderProgram->use();
        m_ShaderProgram->setInt("texture1", 0); // Tell shader to use texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        glCheckError();
        std::cout << "Texture bound to unit 0 and shader uniform set." << std::endl;
    }

    std::cout << "Application initialized successfully." << std::endl;
    return true; // Indicate successful initialization
}

// --- run ---
// Contains the main application loop.
void Application::run() {
    if (!m_IsRunning) {
        std::cerr << "Application cannot run due to initialization failure." << std::endl;
        return;
    }

    std::cout << "Starting main loop..." << std::endl;
    // Loop until the application is signaled to stop or the window should close
    while (m_IsRunning && m_Window && !m_Window->shouldClose()) {
        // Calculate delta time for frame-independent updates/physics
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - m_LastFrameTime;
        m_LastFrameTime = currentTime;

        // 1. Process Input
        m_Window->pollEvents(); // Check for window events (close button, resize, etc.)
        processInput(deltaTime); // Handle application-specific input

        // 2. Update Game State
        update(deltaTime); // Update game logic, physics, animations

        // 3. Render
        render(deltaTime); // Draw the current frame, passing deltaTime for animations

        // 4. Swap Buffers
        m_Window->swapBuffers(); // Present the rendered frame to the screen
    }
    std::cout << "Exiting main loop." << std::endl;
}

// --- processInput ---
// Handles application-level input (like closing the window).
void Application::processInput(float deltaTime) {
    if (!m_Window) return;

    GLFWwindow* window = m_Window->getNativeWindow();

    // Calculate camera direction vectors
    glm::vec3 front;
    front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    front.y = sin(glm::radians(m_cameraPitch));
    front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    glm::vec3 cameraFront = glm::normalize(front);
    glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, globalUp));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));

    // Movement speed adjusted by deltaTime
    float currentSpeed = m_cameraSpeed * deltaTime;

    // Forward/Backward movement (W/S)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        m_cameraPosition += cameraFront * currentSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        m_cameraPosition -= cameraFront * currentSpeed;
    }
    // Left/Right strafing (A/D)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        m_cameraPosition -= cameraRight * currentSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        m_cameraPosition += cameraRight * currentSpeed;
    }
    // Up/Down movement (Space/Left Shift)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_cameraPosition += globalUp * currentSpeed; // Move straight up
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_cameraPosition -= globalUp * currentSpeed; // Move straight down
    }

    // Mouse look
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    // Calculate offset only if mouse has moved
    // (Consider adding a flag for first mouse input)
    float xoffset = xpos - m_lastMouseX;
    float yoffset = m_lastMouseY - ypos; // Reversed since y-coordinates go from top to bottom
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_cameraYaw += xoffset;
    m_cameraPitch += yoffset;

    // Clamp pitch
    m_cameraPitch = std::clamp(m_cameraPitch, -89.0f, 89.0f);

    // Close window on ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "ESC key pressed. Closing window." << std::endl;
        m_Window->setShouldClose(true);
        m_IsRunning = false;
    }

    // Handle Voxel Manipulation Input
    handleVoxelManipulation(); // Call the voxel interaction handler
}


// --- update ---
// Updates game logic (physics, chunk loading/meshing).
void Application::update(float deltaTime) {
    // Update Chunks (Load/Unload)
    updateChunks(); // Handle dynamic chunk loading/unloading

    // Update Physics
    if (m_physicsEngine) {
        m_physicsEngine->stepSimulation(deltaTime);
        // TODO: Update camera/player position based on physics body
    }

    // Check for and update chunk meshes that need it
    for (auto& pair : m_loadedChunks) {
        VoxelChunk* chunk = pair.second.get();
        if (chunk && chunk->needsMeshUpdate()) {
            // std::cout << "Updating mesh for chunk at (" << pair.first.x << ", " << pair.first.y << ")" << std::endl; // Optional debug log
            chunk->generateMesh(); // Regenerate vertex/index data
            chunk->setupMesh();    // Re-upload data to GPU buffers
            chunk->clearMeshUpdateFlag(); // Reset the flag
             // TODO: Update physics collision shape for this chunk
             // Example: m_physicsEngine->updateChunkShape(pair.first, chunk->getMeshData());
        }
    }

    // Other game logic updates
    // e.g., animations, AI, game state based on deltaTime
}

// --- render ---
// Clears the screen and draws the scene (renders chunks).
void Application::render(float deltaTime) {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers
    // Depth test and culling should be enabled in initialize()

    if (!m_ShaderProgram || !m_Window) {
        // std::cerr << "Render skipped: Shader program or window not available." << std::endl; // Debug log if needed
        return;
    }


    m_ShaderProgram->use();

    // --- Camera/View Transformation ---
    // ... existing view/projection matrix calculation ...
    glm::vec3 front;
    front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    front.y = sin(glm::radians(m_cameraPitch));
    front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    glm::vec3 cameraFront = glm::normalize(front);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
    glm::mat4 view = glm::lookAt(m_cameraPosition, m_cameraPosition + cameraFront, cameraUp);

    int width, height;
    glfwGetFramebufferSize(m_Window->getNativeWindow(), &width, &height);
    // Prevent division by zero if window is minimized
    if (width == 0 || height == 0) return;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f); // Increased far plane

    m_ShaderProgram->setMat4("view", view);
    m_ShaderProgram->setMat4("projection", projection);

    // Update view frustum for culling
    glm::mat4 viewProjection = projection * view;
    m_viewFrustum.updateFromVPMatrix(viewProjection);

    // --- Set Lighting Uniforms ---
    m_ShaderProgram->setVec3("lightPos", m_cameraPosition + cameraFront * 5.0f + cameraUp * 3.0f); // Example light position slightly ahead and above camera
    m_ShaderProgram->setVec3("viewPos", m_cameraPosition);
    m_ShaderProgram->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    // objectColor might be set per-chunk or per-voxel type later
    m_ShaderProgram->setVec3("objectColor", 0.8f, 0.8f, 0.8f); // Default grey

    // --- Bind Texture ---
    // Assuming texture unit 0 is active and texture is bound from initialize()
    // If using multiple textures, bind the correct one here.
    if (m_textureID != 0) {
        glActiveTexture(GL_TEXTURE0); // Ensure correct texture unit is active
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        m_ShaderProgram->setInt("texture1", 0); // Ensure uniform points to texture unit 0
    }


    // --- Render Chunks ---
    // This logic is now directly in render()
    for (const auto& pair : m_loadedChunks) {
        const glm::ivec2& chunkPos = pair.first;
        const auto& chunk = pair.second;

        // TODO: Add proper frustum culling check here using isChunkInView
        if (isChunkInView(chunk)) {
            // Calculate model matrix for this chunk based on its world position
            glm::mat4 model = glm::mat4(1.0f);
            // Chunk position is in chunk coordinates, multiply by CHUNK_SIZE for world coordinates
            model = glm::translate(model, glm::vec3(chunkPos.x * CHUNK_SIZE, 0.0f, chunkPos.y * CHUNK_SIZE));
            m_ShaderProgram->setMat4("model", model);

            // Render the chunk
            chunk->render();
        }
    }

    glBindVertexArray(0); // Unbind VAO after rendering all chunks
    glCheckError();

    // --- Render Game UI ---
    if (m_gameUI) {
        // Calculate FPS
        static int frameCount = 0;
        static float timeAccumulator = 0.0f;
        static int fps = 0;
        
        frameCount++;
        timeAccumulator += deltaTime;
        
        // Update FPS calculation once per second
        if (timeAccumulator >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            timeAccumulator = 0.0f;
        }
        
        // Get selected block type from inventory
        int selectedBlock = 1; // Default to block type 1 (Grass)
        if (m_inventory) {
            std::string selectedItem = m_inventory->getSelectedItem();
            if (!selectedItem.empty() && std::isdigit(selectedItem[0])) {
                selectedBlock = std::stoi(selectedItem); // Convert string to int if valid
            }
        }
        
        // Update water animation
        if (m_waterSystem) {
            m_waterSystem->updateAnimation(deltaTime);
        }
        
        // Render UI elements
        m_gameUI->render(deltaTime, fps, selectedBlock, m_cameraPosition);
    }
}

// --- shutdown ---
// Cleans up OpenGL resources and other application resources.
void Application::shutdown() {
    std::cout << "Application shutting down." << std::endl;

    // --- Clean up Chunks ---
    // unique_ptr in m_loadedChunks handles VoxelChunk object deletion,
    // including their OpenGL resources (VAO/VBO/EBO) via VoxelChunk destructor.
    m_loadedChunks.clear();
    std::cout << "Cleaned up voxel chunks (unique_ptr handles resources)." << std::endl;

    // --- Clean up OpenGL resources ---
    // Remove Square VAO/VBO cleanup as they are no longer created
    /*
    if (m_SquareVAO != 0) { ... }
    if (m_SquareVBO != 0) { ... }
    */

    // Delete texture
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        glCheckError();
        std::cout << "Deleted Texture (ID: " << m_textureID << ")" << std::endl;
        m_textureID = 0;
    }

    // m_ShaderProgram (std::unique_ptr) automatically calls Shader destructor (glDeleteProgram).
    // m_Window (std::unique_ptr) automatically calls Window destructor (glfwDestroyWindow, glfwTerminate).
    // m_inventory, m_craftingSystem, m_physicsEngine (unique_ptrs) automatically call their destructors.
    std::cout << "Shader, Window, and Game Systems cleaned up via unique_ptr." << std::endl;

    std::cout << "Application shutdown complete." << std::endl;
}

void Application::initializeVoxelSystem() {
    const int chunkSize = CHUNK_SIZE; // Use defined constant
    
    // Ensure BiomeManager is initialized
    if (!m_biomeManager) {
        std::cerr << "BiomeManager not initialized! Falling back to simple terrain." << std::endl;
        return;
    }
    
    // Generate a small area of chunks around the player
    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) { // Fixed loop condition: changed x <= 2 to z <= 2
            glm::ivec2 chunkPos(x, z);
            
            std::cout << "Loading chunk at: (" << chunkPos.x << ", " << chunkPos.y << ")" << std::endl;
            auto chunk = std::make_unique<VoxelChunk>(chunkSize);
            
            // Set the chunk's world position (for biome and terrain generation)
            chunk->setWorldPosition(chunkPos);
            
            // Provide BiomeManager reference if available
            if (m_biomeManager) {
                chunk->setBiomeManager(m_biomeManager.get());
            }
            
            // Generate terrain using the biome system
            chunk->generateTerrain(); // Use the biome system to generate terrain
            
            // Use optimized greedy meshing for better performance
            chunk->generateOptimizedMesh();
            
            // Add to loaded chunks map
            m_loadedChunks[chunkPos] = std::move(chunk);
        }
    }

    std::cout << "Initial chunks loaded and meshed." << std::endl;
}

void Application::updateChunks() {
    // Calculate the current chunk coordinates that the player is in
    int playerChunkX = static_cast<int>(std::floor(m_cameraPosition.x / CHUNK_SIZE));
    int playerChunkZ = static_cast<int>(std::floor(m_cameraPosition.z / CHUNK_SIZE));
    
    // Set of chunks that should be loaded
    std::unordered_set<glm::ivec2, ivec2Hash> chunksToKeep;
    
    // Determine chunks that should be loaded based on render distance
    for (int x = playerChunkX - RENDER_DISTANCE; x <= playerChunkX + RENDER_DISTANCE; x++) {
        for (int z = playerChunkZ - RENDER_DISTANCE; z <= playerChunkZ + RENDER_DISTANCE; z++) {
            glm::ivec2 chunkPos(x, z);
            
            // Check if this chunk is within the square render distance
            float chunkDist = glm::length(glm::vec2(x - playerChunkX, z - playerChunkZ));
            if (chunkDist <= RENDER_DISTANCE) {
                chunksToKeep.insert(chunkPos);
                
                // If chunk doesn't exist, create it
                if (m_loadedChunks.find(chunkPos) == m_loadedChunks.end()) {
                    auto chunk = std::make_unique<VoxelChunk>(CHUNK_SIZE);
                    chunk->setWorldPosition(chunkPos);
                    
                    // Provide BiomeManager reference if available
                    if (m_biomeManager) {
                        chunk->setBiomeManager(m_biomeManager.get());
                    }
                    
                    // Set texture atlas reference if available
                    if (m_textureAtlas) {
                        chunk->setTextureAtlas(m_textureAtlas.get());
                    }
                    
                    // Generate terrain and mesh
                    chunk->generateTerrain();
                    chunk->generateOptimizedMesh();
                    
                    // Add to loaded chunks map
                    m_loadedChunks[chunkPos] = std::move(chunk);
                    
                    std::cout << "Loaded new chunk at (" << chunkPos.x << ", " << chunkPos.y << ")" << std::endl;
                }
            }
        }
    }
    
    // Find chunks to unload (chunks not in chunksToKeep)
    std::vector<glm::ivec2> chunksToUnload;
    for (const auto& pair : m_loadedChunks) {
        if (chunksToKeep.find(pair.first) == chunksToKeep.end()) {
            chunksToUnload.push_back(pair.first);
        }
    }
    
    // Unload chunks that are too far away
    for (const auto& chunkPos : chunksToUnload) {
        std::cout << "Unloading chunk at (" << chunkPos.x << ", " << chunkPos.y << ")" << std::endl;
        m_loadedChunks.erase(chunkPos);
    }
    
    // Update meshes for chunks that need it
    for (auto& pair : m_loadedChunks) {
        VoxelChunk* chunk = pair.second.get();
        if (chunk && chunk->needsMeshUpdate()) {
            // Use greedy meshing for better performance
            chunk->generateOptimizedMesh();
            chunk->clearMeshUpdateFlag();
            
            // Update chunk physics representation if needed
            if (m_physicsEngine) {
                // TODO: Implement updateChunkCollider in PhysicsEngine class
                // m_physicsEngine->updateChunkCollider(pair.first, chunk);
            }
        }
    }
}

void Application::handleVoxelManipulation() {
    if (!m_Window) return;
    GLFWwindow* window = m_Window->getNativeWindow();
    
    // Check for left mouse button press (break voxel)
    bool leftMousePressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    bool rightMousePressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    
    // Only process clicks, not holds
    bool leftMouseClick = leftMousePressed && !m_leftMouseDown;
    bool rightMouseClick = rightMousePressed && !m_rightMouseDown;
    
    // Update mouse button state
    m_leftMouseDown = leftMousePressed;
    m_rightMouseDown = rightMousePressed;
    
    // If neither mouse button was clicked, return early
    if (!leftMouseClick && !rightMouseClick) {
        return;
    }
    
    // Calculate ray direction from camera
    glm::vec3 rayDir;
    rayDir.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    rayDir.y = sin(glm::radians(m_cameraPitch));
    rayDir.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    rayDir = glm::normalize(rayDir);
    
    // Maximum distance for raycast
    const float maxDist = 10.0f; // Maximum interaction distance
    
    // Perform raycast
    auto raycastResult = raycastVoxel(m_cameraPosition, rayDir, maxDist);
    
    // If raycast hit a voxel
    if (raycastResult) {
        glm::ivec3 localPos;
        
        if (leftMouseClick) {
            // Break voxel
            VoxelChunk* chunk = getChunkFromWorldPos(raycastResult->voxelPos, localPos);
            if (chunk) {
                // Set the voxel to air (type 0)
                chunk->setVoxel(localPos.x, localPos.y, localPos.z, 0);
                // Chunk will be automatically marked for mesh update by setVoxel
                std::cout << "Broke voxel at [" << raycastResult->voxelPos.x << ", " 
                          << raycastResult->voxelPos.y << ", " << raycastResult->voxelPos.z << "]" << std::endl;
            }
        } else if (rightMouseClick) {
            // Place voxel on the face that was hit
            glm::ivec3 placePos = raycastResult->voxelPos + raycastResult->faceNormal;
            VoxelChunk* chunk = getChunkFromWorldPos(placePos, localPos);
            
            if (chunk) {
                // Make sure we're not placing a block inside the player
                glm::vec3 playerMin = m_cameraPosition - glm::vec3(0.3f, 1.7f, 0.3f); // Player bounding box
                glm::vec3 playerMax = m_cameraPosition + glm::vec3(0.3f, 0.3f, 0.3f);
                glm::vec3 blockPos = glm::vec3(placePos) + glm::vec3(0.5f); // Center of the block
                
                // Simple AABB collision check
                bool collides = !(blockPos.x - 0.5f > playerMax.x || blockPos.x + 0.5f < playerMin.x ||
                                 blockPos.y - 0.5f > playerMax.y || blockPos.y + 0.5f < playerMin.y ||
                                 blockPos.z - 0.5f > playerMax.z || blockPos.z + 0.5f < playerMin.z);
                
                if (!collides) {
                    // Set the voxel to a solid type (1 for now, could be selected by player)
                    int voxelType = 1; // Stone
                    
                    // Get the voxel type from inventory if implemented
                    if (m_inventory) {
                        try {
                            int selectedItem = std::stoi(m_inventory->getSelectedItem());
                            if (selectedItem > 0) {
                                voxelType = selectedItem;
                            }
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "Invalid item selected: " << m_inventory->getSelectedItem() << std::endl;
                        } catch (const std::out_of_range& e) {
                            std::cerr << "Selected item out of range: " << m_inventory->getSelectedItem() << std::endl;
                        }
                    }
                    
                    chunk->setVoxel(localPos.x, localPos.y, localPos.z, voxelType);
                    // Chunk will be automatically marked for mesh update by setVoxel
                    std::cout << "Placed voxel at [" << placePos.x << ", " 
                              << placePos.y << ", " << placePos.z << "]" << std::endl;
                } else {
                    std::cout << "Cannot place block inside player" << std::endl;
                }
            }
        }
    }
}

bool Application::isChunkInView(const std::unique_ptr<VoxelChunk>& chunk) {
    // Get the chunk position from our map
    glm::ivec2 chunkPos(-1, -1); // Default invalid position
    
    // Find the chunk position by searching through the map
    for (const auto& pair : m_loadedChunks) {
        if (pair.second.get() == chunk.get()) {
            chunkPos = pair.first;
            break;
        }
    }
    
    if (chunkPos.x == -1 && chunkPos.y == -1) {
        // Chunk not found in our map, shouldn't happen
        return false;
    }
    
    // Calculate AABB for the chunk
    float chunkWorldX = chunkPos.x * CHUNK_SIZE;
    float chunkWorldZ = chunkPos.y * CHUNK_SIZE;
    
    // Calculate the min and max corners of the chunk's bounding box
    glm::vec3 minCorner(chunkWorldX, 0.0f, chunkWorldZ);
    glm::vec3 maxCorner(chunkWorldX + CHUNK_SIZE, CHUNK_SIZE, chunkWorldZ + CHUNK_SIZE);
    
    // Check if the chunk's AABB is inside the view frustum
    return m_viewFrustum.isAABBVisible(minCorner, maxCorner);
}

// Implementation of raycast for voxel manipulation
std::optional<Application::RaycastResult> Application::raycastVoxel(const glm::vec3& start, const glm::vec3& direction, float maxDist) {
    // Initialize variables for the raycast algorithm
    glm::vec3 rayPos = start;
    glm::vec3 rayDir = glm::normalize(direction);
    
    // Step size for the ray (how far to move along the ray each iteration)
    float stepSize = 0.1f; // Small steps for accuracy
    
    // Previous position for tracking face normals
    glm::vec3 prevPos = rayPos;
    
    // Raycast loop
    for (float dist = 0.0f; dist < maxDist; dist += stepSize) {
        // Calculate new position along the ray
        rayPos = start + rayDir * dist;
        
        // Convert to voxel coordinates
        glm::ivec3 voxelPos = glm::ivec3(
            static_cast<int>(std::floor(rayPos.x)),
            static_cast<int>(std::floor(rayPos.y)),
            static_cast<int>(std::floor(rayPos.z))
        );
        
        // Get the chunk and local position for this world position
        glm::ivec3 localPos;
        VoxelChunk* chunk = getChunkFromWorldPos(voxelPos, localPos);
        
        // If we found a chunk and the position is valid
        if (chunk) {
            // Check if there's a solid voxel at this position
            int voxelType = chunk->getVoxel(localPos.x, localPos.y, localPos.z);
            
            // If we've hit a solid voxel (not air)
            if (voxelType != 0) {
                // Calculate face normal (which face was hit)
                glm::vec3 delta = rayPos - prevPos;
                glm::ivec3 faceNormal(0, 0, 0);
                
                // Determine which axis had the largest change
                if (std::abs(delta.x) >= std::abs(delta.y) && std::abs(delta.x) >= std::abs(delta.z)) {
                    faceNormal.x = (delta.x > 0) ? -1 : 1;
                } else if (std::abs(delta.y) >= std::abs(delta.x) && std::abs(delta.y) >= std::abs(delta.z)) {
                    faceNormal.y = (delta.y > 0) ? -1 : 1;
                } else {
                    faceNormal.z = (delta.z > 0) ? -1 : 1;
                }
                
                // Return the hit result
                return RaycastResult{voxelPos, faceNormal};
            }
        }
        
        // Update previous position
        prevPos = rayPos;
    }
    
    // No hit found within maxDist
    return std::nullopt;
}

// Get chunk from world position
VoxelChunk* Application::getChunkFromWorldPos(const glm::ivec3& worldPos, glm::ivec3& localPos) {
    // Calculate chunk coordinates from world position
    int chunkX = std::floor(static_cast<float>(worldPos.x) / CHUNK_SIZE);
    int chunkZ = std::floor(static_cast<float>(worldPos.z) / CHUNK_SIZE);
    
    // Calculate local coordinates within the chunk
    localPos.x = ((worldPos.x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE; // Handle negative coordinates
    localPos.y = worldPos.y; // Y coordinate is unchanged (height)
    localPos.z = ((worldPos.z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE; // Handle negative coordinates
    
    // Check if Y is within chunk bounds
    if (localPos.y < 0 || localPos.y >= CHUNK_SIZE) {
        return nullptr;
    }
    
    // Look up the chunk in our loaded chunks map
    glm::ivec2 chunkPos(chunkX, chunkZ);
    auto it = m_loadedChunks.find(chunkPos);
    
    // Return the chunk if found, nullptr otherwise
    return (it != m_loadedChunks.end()) ? it->second.get() : nullptr;
}
