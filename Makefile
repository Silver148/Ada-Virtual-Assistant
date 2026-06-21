ifeq ($(OS),Windows_NT)
    SYSTEM = Windows (MinGW)
    CPP = g++
    CC  = gcc
    WINDRES = windres
    CMAKE_GENERATOR = "MinGW Makefiles"
    MAKE_CMD = mingw32-make
else
    SYSTEM = Linux
    CPP = x86_64-w64-mingw32-g++
    CC  = x86_64-w64-mingw32-gcc
    WINDRES = x86_64-w64-mingw32-windres
    CMAKE_GENERATOR = "Unix Makefiles"
    MAKE_CMD = $(MAKE)
endif

EXE = Ada.exe
SRC_DIR = src
OBJ_DIR = obj
CURL_LIB_DIR = curl/lib/
SDL2_LIB_DIR = SDL2-Mingw/x86_64-w64-mingw32/lib/
MD4C_LIB_DIR = md4c/build/src/

CXXFLAGS = -std=c++17 -Wall -O2

INCS = -Icurl/include/ -Iinclude -ISDL2-Mingw/x86_64-w64-mingw32/include -Imd4c/src

LIBS = -static-libgcc -static-libstdc++ -L$(CURL_LIB_DIR) -lcurl -L$(SDL2_LIB_DIR) -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -L$(MD4C_LIB_DIR) -lmd4c -mwindows -lole32 -lsapi

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
RES_OBJ = $(OBJ_DIR)/resource.o

all: compile_md4c $(OBJ_DIR) $(EXE) pack_exe

$(EXE): $(OBJECTS) $(RES_OBJ) Makefile
	$(CPP) $(OBJECTS) $(RES_OBJ) -o $(EXE) $(LIBS)
	@echo "Build complete :D!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CPP) $(CXXFLAGS) $(INCS) -c $< -o $@

$(RES_OBJ): resource.rc
	@echo "Compiling win resources ($<)..."
	$(WINDRES) resource.rc -o $(RES_OBJ)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

compile_md4c:
	@echo "Compiling md4c dependence on $(SYSTEM)..."
	mkdir -p md4c/build
	cmake -B md4c/build -G $(CMAKE_GENERATOR) \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CPP) \
		md4c
	$(MAKE_CMD) -C md4c/build

pack_exe:
	mkdir -p Ada
	cp $(EXE) Ada/$(EXE)
	cp -rf fonts Ada/fonts
	cp Ada_SpriteSheet.png Ada/Ada_SpriteSheet.png
	cp curl/bin/libcurl-x64.dll Ada/libcurl-x64.dll
	cp -f SDL2-Mingw/x86_64-w64-mingw32/bin/*.dll Ada/

clean:
	@rm -rf $(OBJ_DIR) $(EXE)
	@rm -rf md4c/build
	@rm -rf Ada
	@echo "Cleaned!"

.PHONY: all clean