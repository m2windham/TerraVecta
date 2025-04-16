#include "include/Application.h"
#include <iostream>
#include <memory>
#include <exception>

int main() {
    try {
        // Create and run the application
        Application app(1280, 720, "TerraVecta - Voxel Engine");
        app.run();
    } catch (const std::exception& e) {
        // Catch potential exceptions during Application construction (e.g., window creation failure)
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        // Catch any other unknown exceptions
        std::cerr << "FATAL ERROR: An unknown exception occurred." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
