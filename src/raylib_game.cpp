/*******************************************************************************************
*
*   raylib game template
*
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include <iostream>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string>                         // Required for:
#include <vector>

#include <fstream>
#include <sstream>

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
//----------------------------------------------------------------------------------
typedef struct Vector2i {
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
} Vector2i;

typedef struct TileDef {
    std::string name;
    Rectangle src;
} TileDef;

Vector2i worldSize =  { 14, 10 };
Vector2i origin = { 5, 1 };


Vector2i tileSize = { 0, 0 };
int *pWorld = nullptr; // 2D world array

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

Texture2D tilesetTexture;
int totalTiles;

// Table for tileset. I need to do this manually it seems. 
std::vector<TileDef> tileData;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
Vector2i toScreen(int x, int y); // transform tile coordinate to screen coordinate, so we know where to draw it
bool loadTileset(std::string fileName); // load a tileset from a tileset metadata file.


//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    
    InitWindow(screenWidth, screenHeight, "Map making program");
    SetTargetFPS(60);



    // not world tiles, so we can keep these seperate. 
    // TODO, maybe put these in a seperate file. It could make it a little easier. 
    
    Texture2D cursorTexture = LoadTexture(TextFormat("%sresources/cursor.png", GetApplicationDirectory()));
    if (!IsTextureValid(cursorTexture)) {
        TraceLog(LOG_ERROR, "Could not load cursor tile");
        CloseWindow();
        return 1;
    }
 
    Image cheatTileImage = LoadImage(TextFormat("%sresources/cheat_tile.png", GetApplicationDirectory()));
    if (!IsImageValid(cheatTileImage)) {
        TraceLog(LOG_ERROR, "Could not load cheat tile");
        CloseWindow();
        return 1;
    }

    const Rectangle basicRect = {0,  0, 64, 32 };

    // Load tileset
    if (!loadTileset("basic1.tileset")) {
        CloseWindow();
        return 1;
    }
    
    // Initialise world
    pWorld = new int[worldSize.x * worldSize.y]{ 0 };
    
    // Initialise mouse info
    Vector2i mousePos = { 0, 0 };
    Vector2i selected = { 0, 0 };
    
    //---------------------------------------------------------


    // Main Loop
    // --------------------------------------------------------
    while (!WindowShouldClose()) {
        // -----Update-----
        // Poll inputs
        mousePos = {(int)GetMousePosition().x , (int)GetMousePosition().y};

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // cehck bounds of the world space 
            if (selected.x >= 0 && selected.x < worldSize.x && selected.y >= 0 && selected.y < worldSize.y) {
                // increment the integer stored in the world array. 
                // modul0 makes it wrap around which is handy
                int &t = pWorld[selected.y * worldSize.x + selected.x];
                t = (t + 1) % totalTiles;
            }
        }

        // Make calculations
        Vector2i selectedCell = { mousePos.x / tileSize.x, mousePos.y / tileSize.y }; // Using integer division
        Vector2i offset = { mousePos.x % tileSize.x, mousePos.y % tileSize.y };


        // Look up the colour under the mouse in the "cheat" tile (index 5) of the tileset
        Color pixelColor = GetImageColor(cheatTileImage, (int)basicRect.x + offset.x, (int)basicRect.y + offset.y);

        selected = {
            (selectedCell.y - origin.y) + (selectedCell.x - origin.x),
            (selectedCell.y - origin.y) - (selectedCell.x - origin.x)
        };

        if (ColorIsEqual(pixelColor, Color{255,0,0,255}))   selected += { -1, +0 }; // RED    (top-left corner)
        if (ColorIsEqual(pixelColor, Color{0,255,0,255}))   selected += { +0, -1 }; // GREEN  (top-right corner)
        if (ColorIsEqual(pixelColor, Color{0,0,255,255}))   selected += { +0, +1 }; // BLUE   (bottom-left corner)
        if (ColorIsEqual(pixelColor, Color{240,255,0,255})) selected += { +1, +0 }; // YELLOW (bottom-right corner)

        // ------Draw------
        BeginDrawing();

        { // Drawing
            ClearBackground(RAYWHITE); // Render background

            // Iteterate through every cell, from top top bottom, since bottom is closer and should be rendered later. 
            for (int y = 0; y < worldSize.y; y++) {
                for (int x = 0; x < worldSize.x; x++) {
                    Vector2i screenPos  = toScreen(x, y);

                    // figure out which tile to draw based on the index in the array.
                    Rectangle src = tileData[pWorld[y * worldSize.x + x]].src;
                    Vector2 pos = {
                        (float)screenPos.x,
                        (float)screenPos.y - (src.height - tileSize.y)
                    };
                    DrawTextureRec(tilesetTexture, src, pos, WHITE);
                    
                }
            }

            // drawing selection info

            // mouse selection overlay thing
            Vector2i selectedPos = toScreen(selected.x, selected.y);
            DrawTextureRec(cursorTexture, basicRect, Vector2{(float)selectedPos.x, (float)selectedPos.y}, WHITE);

            // DrawRectangleLines(selectedCell.x * tileSize.x, selectedCell.y * tileSize.y, tileSize.x, tileSize.y, RED);

            // std::string mouseInfo = "Mouse  : " + std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y);
            std::string selectedInfo = "Selected : " + std::to_string(selected.x) + ", " + std::to_string(selected.y);

            // DrawText(mouseInfo.c_str(), 4, 4, 22, DARKGRAY);
            DrawText(selectedInfo.c_str(), 4, 4, 22, DARKGRAY);

        }

        EndDrawing();


    }
    // --------------------------------------------------------

  
    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadTexture(tilesetTexture);
    UnloadTexture(cursorTexture);
    UnloadImage(cheatTileImage);
    delete[] pWorld;

    CloseWindow();          // Close window and OpenGL context

    //--------------------------------------------------------------------------------------

    return 0;
}

Vector2i toScreen(int x, int y) {
    return Vector2i {
        (origin.x * tileSize.x) + (x - y) * (tileSize.x / 2),
        (origin.y * tileSize.y) + (x + y) * (tileSize.y / 2),
    };
}

bool loadTileset(std::string fileName) {
    std::string dir = std::string(GetApplicationDirectory()) + "resources/tilesets/";


    std::ifstream file(dir + fileName);

    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Could not open tileset file: %s", (dir + fileName).c_str());
        return false;
    }

    // unload previous tileset, just in case
    tileData.clear();
    if (IsTextureValid(tilesetTexture)) UnloadTexture(tilesetTexture);

    std::string line;
    
    // Line 1 is the tileset image source
    std::getline(file, line);
    std::istringstream imageLine(line);
    std::string imageName;
    imageLine >> imageName;

    tilesetTexture = LoadTexture((dir + imageName).c_str());
    if (!IsTextureValid(tilesetTexture)) {
        TraceLog(LOG_ERROR, "%s: could not load image '%s'", fileName.c_str(), imageName.c_str());
        return false;
    }

    // Line 2 is the tile size footprint
    std::getline(file, line);
    std::istringstream sizeLine(line);
    if (!(sizeLine >> tileSize.x >> tileSize.y || tileSize.x <= 0 || tileSize.y <= 0)) {
        TraceLog(LOG_ERROR, "%s: line 2 should be the tile size, e.g. '64 32'", fileName.c_str());
        return false;
    }

    // the remaining lines populate the tiledefs struct in format: x y w h
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        TileDef t;
        if (iss >> t.name >> t.src.x >> t.src.y >> t.src.width >> t.src.height) {
            tileData.push_back(t);
        }
    }
 
    totalTiles = (int)tileData.size();
    return totalTiles > 0;

}