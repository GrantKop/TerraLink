#include "graphics/Shader.h"

Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    int shaderID = genShaderProgram(vertexShaderPath.c_str(), fragmentShaderPath.c_str());

    if (shaderID != -1) {
        this->ID = shaderID;
    } 
}

Shader::~Shader() {
    glDeleteProgram(this->ID);
}

bool Shader::use() {
    glUseProgram(this->ID);
    return true;
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
    glUseProgram(this->ID);
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) {
    glUseProgram(this->ID);
    glUniform4fv(glGetUniformLocation(this->ID, name.c_str()), 1, glm::value_ptr(vec));
}

std::string Shader::readFile(const char* filename) {
    std::ifstream file;
    std::stringstream buf;
    std::string ret = "";

    file.open(filename);

    if (file.is_open()) {
        buf << file.rdbuf();
        ret = buf.str();
    }
    else {
        std::cerr << "Failed to open " << filename << std::endl;
    }

    return ret;
}

int Shader::genShader(const char* filepath, GLenum type) {
    std::string shaderSrc = readFile(filepath);
    if (shaderSrc.empty()) {
        std::cerr << "Failed to read " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader source." << std::endl;
        return -1;
    }

    const GLchar* shader = shaderSrc.c_str();

    int shaderObj = glCreateShader(type);
    if (shaderObj == 0) {
        std::cerr << "Failed to create " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader object." << std::endl;
        return -1;
    }

    glShaderSource(shaderObj, 1, &shader, NULL);
    glCompileShader(shaderObj);

    int success;
    char infoLog[512];
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderObj, 512, NULL, infoLog);
        std::cerr << "Error in "<< (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader compilation.\n " << infoLog << std::endl;
        return -1;
    }

    return shaderObj;
}

int Shader::genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
    int shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        std::cerr << "Failed to create shader program." << std::endl;
        return -1;
    }

    int vertexShader = genShader(vertexShaderPath, GL_VERTEX_SHADER);
    int fragmentShader = genShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1) {
        return -1;
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Error in shader linking.\n " << infoLog << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
