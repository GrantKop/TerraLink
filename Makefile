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
