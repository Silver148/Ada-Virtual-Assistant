VERSION = 1.8.1
CROSS ?= 0
CMAKE_GENERATOR = "Unix Makefiles"
MAKE_CMD = $(MAKE)
DEBUG ?= 0

ifeq ($(OS),Windows_NT)
	CROSS = 1
else
	CROSS = 0
endif

ifeq ($(CROSS), 1)
    EXE = Ada.exe
    SYSTEM = Windows (MinGW)
ifeq ($(OS),Windows_NT)
	CPP = g++
    CC  = gcc   
else
	CPP = x86_64-w64-mingw32-g++-posix
    CC  = x86_64-w64-mingw32-gcc-posix
endif

    WINDRES = x86_64-w64-mingw32-windres
    SOURCES = $(filter-out $(SRC_DIR)/Audio_Engine.cpp, $(wildcard $(SRC_DIR)/*.cpp))
    RES_OBJ = $(OBJ_DIR)/resource.o
else
    EXE = Ada
    SYSTEM = Linux
    CPP = g++
    CC  = gcc
    SOURCES = $(filter-out $(SRC_DIR)/SearchEXE.cpp, $(wildcard $(SRC_DIR)/*.cpp))
    RES_OBJ =
endif

SRC_DIR = src
OBJ_DIR = obj
CURL_LIB_DIR = curl/lib/
SDL2_LIB_DIR = SDL2-Mingw/x86_64-w64-mingw32/lib/
MD4C_LIB_DIR = md4c/build/src/
VOSK_LINUX = vosk_linux/
VOSK_WINDOWS = vosk_win64/

CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2

ifeq ($(DEBUG), 1)
    CXXFLAGS += -O0 -g -DDEBUG_MODE
else
    CXXFLAGS += -O3 -DNDEBUG
endif

ifeq ($(DEBUG), 1)
    CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

ifeq ($(SYSTEM), Windows (MinGW))
    INCS = -Icurl/include/ -Iinclude -ISDL2-Mingw/x86_64-w64-mingw32/include -Imd4c/src \
            -I$(VOSK_WINDOWS)
    LIBS =  -static-libgcc -static-libstdc++ \
            -L$(CURL_LIB_DIR) -lcurl -L$(SDL2_LIB_DIR) -lSDL2 -lSDL2_image -lSDL2_mixer \
            -lSDL2_ttf -L$(MD4C_LIB_DIR) -L$(VOSK_WINDOWS) -lvosk -lmd4c \
			-mwindows -lshell32 -lole32 -lsapi -lurlmon -luuid
else
    INCS = -Iinclude -Imd4c/src $(shell pkg-config --cflags libnotify) -I$(VOSK_LINUX)
    LIBS = -lm -lpthread -lcurl -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf \
           -L$(MD4C_LIB_DIR) -lmd4c $(shell pkg-config --libs libnotify) -L$(VOSK_LINUX) -lvosk -lttspico \
		   -Wl,-rpath,'$$ORIGIN'
endif

OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
OBJ_LIST = $(OBJECTS) $(RES_OBJ)

all: compile_md4c compile_llama-server $(EXE) pack_exe

$(EXE): $(OBJECTS) $(RES_OBJ) Makefile
	$(CPP) $(OBJ_LIST) -o $(EXE) $(LIBS)
	@echo "Build complete :D!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CPP) $(CXXFLAGS) $(INCS) -c $< -o $@

ifeq ($(SYSTEM), Windows (MinGW))
$(RES_OBJ): resource.rc
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling win resources ($<)..."
	$(WINDRES) resource.rc -o $(RES_OBJ)
endif

compile_md4c:
	@if [ -d "md4c/build" ]; then \
		echo "Dependency md4c already compiled. Skipping."; \
	else \
		echo "Compiling md4c dependence on $(SYSTEM)..."; \
		mkdir -p md4c/build; \
		if [ "$(SYSTEM)" = "Windows (MinGW)" ]; then \
			cmake -B md4c/build -G $(CMAKE_GENERATOR) \
				-DCMAKE_SYSTEM_NAME=Windows \
				-DCMAKE_C_COMPILER=$(CC) \
				-DCMAKE_CXX_COMPILER=$(CPP) \
				md4c; \
		else \
			cmake -B md4c/build -G "Unix Makefiles" md4c; \
		fi; \
		$(MAKE_CMD) -j3 -C md4c/build; \
	fi

compile_llama-server:
	@if [ -d "llama.cpp/build" ]; then \
		echo "Universal llama-server already compiled. Skipping."; \
	else \
		echo "Compiling UNIVERSAL llama-server (Smart Dynamic CPU) on $(SYSTEM)..."; \
		mkdir -p llama.cpp/build; \
		if [ "$(SYSTEM)" = "Windows (MinGW)" ]; then \
			cmake -B llama.cpp/build -G $(CMAKE_GENERATOR) \
				-DCMAKE_SYSTEM_NAME=Windows \
				-DCMAKE_C_COMPILER=$(CC) \
				-DCMAKE_CXX_COMPILER=$(CPP) \
				-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
				-DCMAKE_BUILD_TYPE=Release \
				-DGGML_OPENMP=ON \
				-DGGML_BACKEND_DL=ON \
				-DGGML_CPU_ALL_VARIANTS=ON \
				-DGGML_CUDA=OFF \
				-DBUILD_SHARED_LIBS=ON \
				-DLLAMA_BUILD_EXAMPLES=OFF \
				-DLLAMA_BUILD_TESTS=OFF \
				-DLLAMA_BUILD_TOOLS=ON \
				-DLLAMA_BUILD_SERVER=ON \
				-DLLAMA_BUILD_APP=OFF \
				llama.cpp; \
		else \
			cmake -B llama.cpp/build -G "Unix Makefiles" \
				-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
				-DCMAKE_BUILD_TYPE=Release \
				-DGGML_OPENMP=ON \
				-DGGML_BACKEND_DL=ON \
				-DGGML_CPU_ALL_VARIANTS=ON \
				-DGGML_CUDA=OFF \
				-DBUILD_SHARED_LIBS=ON \
				-DLLAMA_BUILD_EXAMPLES=OFF \
				-DLLAMA_BUILD_TESTS=OFF \
				-DLLAMA_BUILD_TOOLS=ON \
				-DLLAMA_BUILD_SERVER=ON \
				-DLLAMA_BUILD_APP=OFF \
				llama.cpp; \
		fi; \
		$(MAKE_CMD) -j3 -C llama.cpp/build llama-server; \
	fi


pack_exe:
	mkdir -p Ada_packed
	cp $(EXE) Ada_packed/$(EXE)
	cp -rf fonts Ada_packed/fonts
	cp Ada_SpriteSheet.png Ada_packed/Ada_SpriteSheet.png
	cp -rf vosk_model Ada_packed/vosk_model
	cp -f llama.cpp/build/bin/* Ada_packed/
ifeq ($(SYSTEM), Windows (MinGW))
	cp curl/bin/libcurl-x64.dll Ada_packed/libcurl-x64.dll
	cp -f SDL2-Mingw/x86_64-w64-mingw32/bin/*.dll Ada_packed/
	cp -f vosk_win64/*.dll Ada_packed/
else
	rm -f Ada_packed/libmd4c.so Ada_packed/libmd4c.so.0
	cp -f $(MD4C_LIB_DIR)/libmd4c.so Ada_packed/libmd4c.so
	ln -s libmd4c.so Ada_packed/libmd4c.so.0
	cp -f vosk_linux/libvosk.so Ada_packed/libvosk.so

	@chmod +x Ada_packed/Ada
	cp -f launcher/ada-launcher Ada_packed/ada-launcher
	chmod +x Ada_packed/ada-launcher

	cp -f Ada-ICON.png Ada_packed/Ada-ICON.png

	tar -czvf Ada-$(VERSION)-linux-amd64.tar.gz -C Ada_packed .
endif

make_debian_package:
	mkdir -p ada_deb/opt/ada/
	mkdir -p ada_deb/DEBIAN
	mkdir -p ada_deb/usr/share/applications/
	mkdir -p ada_deb/usr/share/man/man1
	
	cp -r Ada_packed/* ada_deb/opt/ada/
	cp desktop/ada-virtual-assistant.desktop ada_deb/usr/share/applications/

	@mkdir -p ada_deb/usr/share/man/man1
	cp deb_man/ada.1 ada_deb/usr/share/man/man1/ada.1
	@build_date="$$(LC_ALL=C date '+%B %d, %Y')"; \
		sed -i "1c.TH ADA 1 \"$$build_date\" \"v$(VERSION)\" \"Ada Manual\"" \
		ada_deb/usr/share/man/man1/ada.1
	gzip -n -f ada_deb/usr/share/man/man1/ada.1
	@ln -sf ada.1.gz ada_deb/usr/share/man/man1/ada-assistant.1.gz

	@printf '#!/bin/bash\nset -e\nln -sf /opt/ada/ada-launcher /usr/local/bin/ada\n' > ada_deb/DEBIAN/postinst
	@chmod 755 ada_deb/DEBIAN/postinst

	@printf '#!/bin/bash\nset -e\nrm -f /usr/local/bin/ada\n' > ada_deb/DEBIAN/prerm
	@printf '%s\n' \
		'getent passwd | while IFS=: read -r _ _ _ _ _ home _; do' \
		'    if [ -n "$$home" ]; then' \
		'        rm -f -- "$$home/.config/AdaOffline.Q4_K_M.gguf" 2>/dev/null' \
		'        rm -rf -- "$$home/.config/ada" 2>/dev/null' \
		'    fi' \
		'done' \
		>> ada_deb/DEBIAN/prerm

	@printf 'rm -f /usr/share/applications/ada-virtual-assistant.desktop\n' >> ada_deb/DEBIAN/prerm
	@chmod 755 ada_deb/DEBIAN/prerm

	@printf '%s\n' \
		'Package: ada-assistant' \
		"Version: $(VERSION)" \
		'Section: utils' \
		'Priority: optional' \
		'Architecture: amd64' \
		'Depends: libcurl4, libsdl2-2.0-0, libsdl2-image-2.0-0, libsdl2-ttf-2.0-0, libttspico0t64, libttspico-data' \
		'Maintainer: Juan Yaguaro (aka silverhacker) <silverhckr6@gmail.com>' \
		'Description: AI-powered virtual assistant.' \
		> ada_deb/DEBIAN/control

	dpkg-deb --root-owner-group --build ada_deb ada-assistant_$(VERSION)_amd64.deb

	rm -rf ada_deb
	@echo "Debian Package successfully created: ada-assistant_$(VERSION)_amd64.deb"

pack_for_windows:
	zip -r Ada-$(VERSION)-Windows-x64.zip Ada_packed/
	makensis -DPRODUCT_VERSION=$(VERSION) win_setup/setup.nsi
	@echo "Ada $(VERSION) was packed for Windows :D!"

install_debian_package:
	sudo apt install ./ada-assistant_$(VERSION)_amd64.deb

clean_all:
	@rm -rf md4c/build
	@rm -rf llama.cpp/build
	@rm -rf $(OBJ_DIR) *.exe Ada
	@rm -rf Ada_packed
	@rm -f *.deb
	@rm -f *.tar.gz
	@rm -f *.zip
	@echo "Cleaned!"

clean_app:
	@rm -rf $(OBJ_DIR) *.exe Ada
	@rm -rf Ada_packed
	@rm -f *.deb
	@rm -f *.tar.gz
	@rm -f *.zip
	@echo "Cleaned!"

.PHONY: all clean compile_md4c pack_exe
