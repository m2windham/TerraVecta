#include "include/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector> // For error log buffer

// Constructor: Reads shader files, compiles them, and links the program
Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // Ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        std::cerr << "  Vertex Path: " << vertexPath << std::endl;
        std::cerr << "  Fragment Path: " << fragmentPath << std::endl;
        // Consider throwing an exception here or setting an error state
        // For now, just return, leaving m_ID as 0
        return;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    GLuint vertex, fragment;
    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // Shader Program
    m_ID = glCreateProgram();
    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (m_ID != 0) { // Check if linking was successful before printing success
        std::cout << "Shader program created successfully (ID: " << m_ID << ")" << std::endl;
    } else {
         std::cerr << "Shader program creation failed (linking error likely)." << std::endl;
    }
}

// Destructor
Shader::~Shader() {
    if (m_ID != 0) {
        glDeleteProgram(m_ID);
        // Optional: Add a confirmation message if desired
        // std::cout << "Shader program destroyed (ID: " << m_ID << ")" << std::endl;
    }
}

// Use/activate the shader program
void Shader::use() const {
    if (m_ID != 0) {
        glUseProgram(m_ID);
    } else {
        // Avoid spamming errors if creation failed
        // std::cerr << "ERROR::SHADER::USE::Program ID is 0 (Shader likely failed to compile/link)" << std::endl;
    }
}

// --- Utility uniform functions ---
// (Implementations retrieve uniform location and set the value)

void Shader::setBool(const std::string &name, bool value) const {
    if (m_ID != 0) glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const {
    if (m_ID != 0) glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}
void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
    if (m_ID != 0) glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec2(const std::string &name, float x, float y) const {
    if (m_ID != 0) glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
}
void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
    if (m_ID != 0) glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    if (m_ID != 0) glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}
void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
    if (m_ID != 0) glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const {
    if (m_ID != 0) glUniform4f(glGetUniformLocation(m_ID, name.c_str()), x, y, z, w);
}
void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const {
    if (m_ID != 0) glUniformMatrix2fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const {
    if (m_ID != 0) glUniformMatrix3fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    if (m_ID != 0) glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

// Utility function for checking shader compilation/linking errors.
void Shader::checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLint logLength = 0;
    // Use a sufficiently large buffer or dynamically allocate based on logLength
    // std::vector is safer for dynamic allocation
    std::vector<GLchar> infoLog;

    if (type != "PROGRAM") { // Check shader compilation
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            infoLog.resize(logLength + 1); // Resize vector (+1 for null terminator)
            glGetShaderInfoLog(shader, logLength, NULL, infoLog.data());
            infoLog[logLength] = '\0'; // Ensure null termination
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog.data() << "\n -- --------------------------------------------------- -- " << std::endl;
            m_ID = 0; // Mark shader as invalid on compile error
        }
    } else { // Check shader program linking
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            infoLog.resize(logLength + 1); // Resize vector (+1 for null terminator)
            glGetProgramInfoLog(shader, logLength, NULL, infoLog.data());
            infoLog[logLength] = '\0'; // Ensure null termination
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog.data() << "\n -- --------------------------------------------------- -- " << std::endl;
            m_ID = 0; // Mark shader as invalid on link error
        }
    }
}
