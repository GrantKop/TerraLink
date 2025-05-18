BUILD_DIR = build
TOOLCHAIN_FILE = ../vcpkg/scripts/buildsystems/vcpkg.cmake

.PHONY: configure build clean run vcpkg gdb debug

setup: vcpkg configure build

all: configure build

rebuild: clean configure build

configure:
	@printf "\nConfiguring project...\n\n"
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE)

build:
	@printf "\nBuilding project...\n\n"
	@cd $(BUILD_DIR) && cmake --build .

debug:
	@printf "\nBuilding project in Debug mode...\n\n"
	@cd $(BUILD_DIR) && cmake --debug-output .

clean:
	@if [ -d build ]; then \
		/usr/bin/find build -mindepth 1 ! -name .gitkeep -delete; \
	fi

clear: clean

run: build
	@if [ -d build ] && [ -d build/Debug ]; then \
		cd build/Debug && ./TerraLink.exe $(ARGS); \
	fi

vcpkg:
	@git clone https://github.com/microsoft/vcpkg.git
	@./vcpkg/bootstrap-vcpkg.bat
	@cd vcpkg && ./vcpkg install glfw3 glad glm stb nlohmann-json zstd

gdb: 
	@cd build/Debug && gdb TerraLink.exe

installer:
	@echo "Configuring CMake for Release build..."
	@cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=build/install
	@echo "Building project in Release mode..."
	@cmake --build build --config Release
	@echo "Installing project..."
	@cmake --install build --config Release
	@echo "Project installed to build/install"
	@cd build && "C:/Program Files/CMake/bin/cpack.exe" -G NSIS

install: installer
