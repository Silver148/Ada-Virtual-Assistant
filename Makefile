EXE = Ada.exe
CPP = g++
SRC_DIR = src
OBJ_DIR = obj
CURL_LIB_DIR = curl/lib/
SDL2_LIB_DIR = SDL2-Mingw/x86_64-w64-mingw32/lib/
MD4C_LIB_DIR = md4c/build/src/

CXXFLAGS = -std=c++17 -Wall -O2

INCS = -Icurl/include/ -Iinclude -ISDL2-Mingw/x86_64-w64-mingw32/include -Imd4c/src

LIBS = -L$(CURL_LIB_DIR) -lcurl -L$(SDL2_LIB_DIR) -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -L$(MD4C_LIB_DIR) -lmd4c -mwindows -lole32 -lsapi

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
RES_OBJ = $(OBJ_DIR)/resource.o

all: compile_md4c $(OBJ_DIR) $(EXE) pack_exe

$(EXE): $(OBJECTS) $(RES_OBJ)
	$(CPP) $(OBJECTS) $(RES_OBJ) -o $(EXE) $(LIBS)
	@echo "Build complete :D!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CPP) $(CXXFLAGS) $(INCS) -c $< -o $@

$(RES_OBJ): resource.rc
	@echo "Compiling win resources ($<)..."
	windres resource.rc -o $(RES_OBJ)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

compile_md4c:
	@echo "compiling md4c..."
	mkdir -p md4c/build
	cmake -B md4c/build -G "MinGW Makefiles" md4c
	make -C md4c/build

pack_exe:
	mkdir -p Ada
	cp $(EXE) Ada/$(EXE)
	cp -rf fonts Ada/fonts
	cp Ada_SpriteSheet.png Ada/Ada_SpriteSheet.png
	cp curl/bin/libcurl-x64.dll Ada/libcurl-x64.dll
	cp -f SDL2-Mingw/x86_64-w64-mingw32/bin/*.dll Ada/

clean:
	@rm -rf $(OBJ_DIR) $(EXE)
	@rm -rf $(MD4C_LIB_DIR)/src *.a
	@rm -rf $(MD4C_LIB_DIR)/CMakeFiles
	@rm -rf Ada
	@echo "Cleaned!"

.PHONY: all clean