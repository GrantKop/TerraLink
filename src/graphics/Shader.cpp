#include "graphics/Shader.h"

Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, std::string geometryShaderPath) {
    int shaderID = genShaderProgram(vertexShaderPath.c_str(), fragmentShaderPath.c_str());

    if (shaderID != -1) {
        this->ID = shaderID;
    } else {
        std::cerr << "Shader creation failed, shader ID set to 0." << std::endl;
        this->ID = 0;
    }
}

Shader::~Shader() {}

void Shader::use() {
    glUseProgram(this->ID);
}

void Shader::deleteShader() {
    glDeleteProgram(this->ID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
    use();
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) {
    use();
    glUniformMatrix3fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) {
    use();
    glUniform4fv(glGetUniformLocation(this->ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setVec4(const std::string& name, const glm::vec4& vec) {
    use();
    glUniform4fv(glGetUniformLocation(this->ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUniform3(const std::string& name, const glm::mat3& mat) {
    use();
    glUniformMatrix3fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUniform4(const std::string& name, const glm::mat4& mat) {
    use();
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUniform3(const std::string& name, const glm::vec3& vec) {
    use();
    glUniform3fv(glGetUniformLocation(this->ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUniform4(const std::string& name, const glm::vec4& vec) {
    use();
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

GLuint Shader::genShader(const char* filepath, GLenum type) {
    std::string shaderSrc = readFile(filepath);
    if (shaderSrc.empty()) {
        std::cerr << "Failed to read " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader source." << std::endl;
        return -1;
    }

    const GLchar* shader = shaderSrc.c_str();

    GLuint shaderObj = glCreateShader(type);
    if (shaderObj == 0) {
        std::cerr << "Failed to create " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader object." << std::endl;
        return -1;
    }

    glShaderSource(shaderObj, 1, &shader, NULL);
    glCompileShader(shaderObj);

    GLint success;
    char infoLog[1024];
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(shaderObj, 1024, NULL, infoLog);
        std::cerr << "Error in "<< (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << " shader compilation.\n " << infoLog << std::endl;
        return -1;
    }

    return shaderObj;
}

GLuint Shader::genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath) {
    GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        std::cerr << "Failed to create shader program." << std::endl;
        return -1;
    }

    GLuint vertexShader = genShader(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = genShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1) {
        return -1;
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    char infoLog[1024];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cerr << "Error in shader linking.\n " << infoLog << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
