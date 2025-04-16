#ifndef SHADER_H
#define SHADER_H

#include <string> // For std::string
#include <glad/glad.h> // Include GLAD for OpenGL types like GLuint
#include <glm/glm.hpp> // Include GLM for matrix/vector types used in uniform setters

class Shader {
public:
    // Constructor: Reads shader source files, compiles, and links them.
    // Takes paths to the vertex and fragment shader files.
    Shader(const char* vertexPath, const char* fragmentPath);

    // Destructor: Cleans up the OpenGL shader program resource.
    ~Shader();

    // Prevent copying and assignment to avoid issues with OpenGL resource management
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    // Optional: Implement move constructor/assignment if needed later

    // Activates the shader program for use in rendering.
    void use() const;

    // Retrieves the OpenGL ID of the linked shader program.
    GLuint getID() const { return m_ID; }

    // --- Utility functions for setting uniform variables in the shader ---
    // These functions find the uniform location by name and set its value.
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    // Stores the OpenGL ID for the linked shader program.
    // Initialized to 0, indicating an invalid or uninitialized program.
    GLuint m_ID = 0;

    // Private helper function to check for compilation or linking errors.
    // Called internally by the constructor.
    void checkCompileErrors(GLuint shaderOrProgram, std::string type);
};

#endif // SHADER_H
