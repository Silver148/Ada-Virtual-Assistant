VERSION = 1.3.1
CROSS ?= 0
CMAKE_GENERATOR = "Unix Makefiles"
MAKE_CMD = $(MAKE)

ifeq ($(SYSTEM), Windows_NT)
	CROSS = 1
else
	CROSS = 0
endif

ifeq ($(CROSS), 1)
    EXE = Ada.exe
    SYSTEM = Windows (MinGW)
    CPP = x86_64-w64-mingw32-g++
    CC  = x86_64-w64-mingw32-gcc
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

CXXFLAGS = -std=c++17 -Wall -O0

ifeq ($(SYSTEM), Windows (MinGW))
    INCS = -Icurl/include/ -Iinclude -ISDL2-Mingw/x86_64-w64-mingw32/include -Imd4c/src
    LIBS = -static-libgcc -static-libstdc++ -L$(CURL_LIB_DIR) -lcurl -L$(SDL2_LIB_DIR) -lSDL2 \
           -lSDL2_image -lSDL2_mixer -lSDL2_ttf -L$(MD4C_LIB_DIR) -lmd4c -mwindows -lole32 -lsapi
else
    INCS = -Iinclude -Imd4c/src $(shell pkg-config --cflags libnotify)
    LIBS = -Wl,-rpath,'$$ORIGIN' -lm -lpthread -lcurl -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf \
           -L$(MD4C_LIB_DIR) -lmd4c $(shell pkg-config --libs libnotify)
endif

OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
OBJ_LIST = $(OBJECTS) $(RES_OBJ)

all: compile_md4c $(EXE) pack_exe

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
	@echo "Compiling md4c dependence on $(SYSTEM)..."
	mkdir -p md4c/build
ifeq ($(SYSTEM), Windows (MinGW))
	cmake -B md4c/build -G $(CMAKE_GENERATOR) \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CPP) \
		md4c
else
	cmake -B md4c/build -G "Unix Makefiles" md4c
endif
	$(MAKE_CMD) -C md4c/build

pack_exe:
	mkdir -p Ada_packed
	cp $(EXE) Ada_packed/$(EXE)
	cp -rf fonts Ada_packed/fonts
	cp Ada_SpriteSheet.png Ada_packed/Ada_SpriteSheet.png
ifeq ($(SYSTEM), Windows (MinGW))
	cp curl/bin/libcurl-x64.dll Ada_packed/libcurl-x64.dll
	cp -f SDL2-Mingw/x86_64-w64-mingw32/bin/*.dll Ada_packed/
else
	rm -f Ada_packed/libmd4c.so Ada_packed/libmd4c.so.0
	cp -f $(MD4C_LIB_DIR)/libmd4c.so Ada_packed/libmd4c.so
	ln -s libmd4c.so Ada_packed/libmd4c.so.0
	cp -rf bin Ada_packed/bin
	sudo setcap cap_sys_nice=eip ./Ada_packed/$(EXE)
	tar -czvf Ada-$(VERSION).tar.gz -C Ada_packed .
endif

make_debian_package:
	mkdir -p ada_deb/opt/ada/
	mkdir -p ada_deb/DEBIAN
	cp -r Ada_packed/* ada_deb/opt/ada/

	@printf '#!/bin/bash\nset -e\n\n# 1. Create the symbolic link\nln -sf /opt/ada/Ada /usr/local/bin/ada\n\n# 2. Attempt setcap\nsetcap cap_sys_nice=eip /opt/ada/Ada 2>/dev/null || echo "Note: Could not apply setcap, using standard permissions."\n\nexit 0' > ada_deb/DEBIAN/postinst
	@chmod 755 ada_deb/DEBIAN/postinst

	@printf "Package: ada-assistant\nVersion: $(VERSION)\nSection: utils\nPriority: optional\nArchitecture: amd64\nDepends: libcurl4, libsdl2-2.0-0\nMaintainer: Juan Yaguaro (aka silverhacker)\nDescription: AI-powered virtual assistant.\n" > ada_deb/DEBIAN/control

	dpkg-deb --root-owner-group --build ada_deb ada-assistant_$(VERSION)_amd64.deb

	rm -rf ada_deb
	@echo "Debian Package successfully created: ada-assistant_$(VERSION)_amd64.deb"

install_debian_package:
	sudo apt install ./ada-assistant_$(VERSION)_amd64.deb

clean:
	@rm -rf $(OBJ_DIR) $(EXE) *.exe
	@rm -rf md4c/build
	@rm -rf Ada_packed
	@rm -f *.deb
	@rm -f *.tar.gz
	@echo "Cleaned!"

.PHONY: all clean compile_md4c pack_exe