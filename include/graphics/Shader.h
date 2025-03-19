#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../utils/GLUtils.h"

class Shader {
    public:
        Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, std::string geometryShaderPath = "");
        ~Shader();

        GLuint ID;

        void use();
        void deleteShader();

        void setMat4(const std::string& name, const glm::mat4& mat);
        void setMat3(const std::string& name, const glm::mat3& mat);
        void setVec4(const std::string& name, const glm::vec4& vec);
        void setVec3(const std::string& name, const glm::vec3& vec);

        void setUniform4(const std::string& name, const glm::mat4& mat);
        void setUniform3(const std::string& name, const glm::mat3& mat);
        void setUniform4(const std::string& name, const glm::vec4& vec);
        void setUniform3(const std::string& name, const glm::vec3& vec);

        void setFloat(const std::string& name, float value);
        void setInt(const std::string& name, int value);

    private:
        std::string readFile(const char* filename);

        GLuint genShader(const char* filepath, GLenum type);

        GLuint genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath = "");
};

#endif
