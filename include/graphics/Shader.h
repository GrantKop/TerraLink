#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../utils/GLUtils.h"

class Shader {
    public:
        Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
        ~Shader();

        int ID;

        bool use();

        void setMat4(const std::string& name, const glm::mat4& mat);
        void setVec3(const std::string& name, const glm::vec3& vec);

    private:
        std::string readFile(const char* filename);

        int genShader(const char* filepath, GLenum type);

        int genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath);
};

#endif
