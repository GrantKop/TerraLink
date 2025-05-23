#include "utils/GLUtils.h"
#include "core/game/Game.h"

bool loadGlad() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void initGLFW(unsigned int versionMajor, unsigned int versionMinor) {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool createWindow(GLFWwindow*& window, const char* title, unsigned int width, unsigned int height, GLFWframebuffersizefun framebufferSizeCallback) {

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!loadGlad()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, width, height);

    std::string iconPath = (std::filesystem::current_path().parent_path().parent_path() / "assets/icon/icon.png").string();

    if (!std::filesystem::exists(iconPath)) {
        std::cerr << "Icon file not found at: " << iconPath << "\n";
    }

    GLFWimage images[1];
    images[0].pixels = stbi_load(iconPath.c_str(), &images[0].width, &images[0].height, 0, 4);

    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images);
        stbi_image_free(images[0].pixels);
    } else {
        std::cerr << "Warning: Could not load window icon: " << iconPath << std::endl;
    }

    return true;
}

int checkGLError() {
    GLenum err;

    while ((err = glGetError()) != GL_NO_ERROR) {
        switch (err) {
            case GL_INVALID_ENUM:
                std::cerr << "OpenGL error: " << err << " | GL_INVALID_ENUM";
                return err;
            case GL_INVALID_VALUE:
                std::cerr << "OpenGL error: " << err << " | GL_INVALID_VALUE";
                return err;
            case GL_INVALID_OPERATION:
                std::cerr << "OpenGL error: " << err << " | GL_INVALID_OPERATION";
                return err;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cerr << "OpenGL error: " << err << " | GL_INVALID_FRAMEBUFFER_OPERATION";
                return err;
            case GL_OUT_OF_MEMORY:
                std::cerr << "OpenGL error: " << err << " | GL_OUT_OF_MEMORY";
                return err;
            default:
                std::cerr << "OpenGL error: " << err << " | Unknown error";
                return err;
        }
    }
    return err;
}
