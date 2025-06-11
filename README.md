# TerraLink

**TerraLink** is a procedural terrain engine written in C++ with OpenGL. It features real-time chunk generation, multithreaded streaming, and multiplayer networking using UDP/TCP. Designed to be scalable and performant, TerraLink is a project I completed for 5 credit hours that explores the combination of procedural graphics, multithreading, and networking.

---

## Features

- 🌍 Procedural terrain generation using Perlin noise
- 🧵 Multithreaded background chunk creation, meshing, and uploading with no mutex locks
- 🌐 Multiplayer support with UDP chunk streaming and TCP client tracking
- 💾 Persistent chunk saving and loading using Zstandard compression
- 🎮 OpenGL-based rendering using a handful of shader programs
- 🌤️ Multi-Biome blending and domain-warped noise to prevent jumps in terrain

---

## Screenshots
![image](https://github.com/user-attachments/assets/04f9812d-8f4a-45aa-b12b-3bd1721c6117)
![image](https://github.com/user-attachments/assets/f72bb1cd-3d2f-462f-9bd3-76ea93c215fa)
![image](https://github.com/user-attachments/assets/1d3ff36a-a5cc-4768-943a-8f588c9961e7)
![image](https://github.com/user-attachments/assets/43e0031b-99d4-4b7e-89e8-b8b0fa2fcd64)

---

## Prerequisites

- [CMake](https://cmake.org/)
- [Git](https://git-scm.com/)
- [CPack](https://cmake.org/cmake/help/latest/module/CPack.html)

All other dependencies (GLFW, GLAD, GLM, stb, etc.) are installed automatically using **[vcpkg](https://github.com/microsoft/vcpkg)**.

---

## Building the Project

Clone the repository and use the provided `Makefile`:

```bash
# Install dependencies, configure and build
make setup

# Run the executable (inside src/main.cpp, DEV_MODE must be set to TRUE)
make run

# If you want to make the installer, inside src/main.cpp, DEV_MODE must be set to FALSE
make installer
