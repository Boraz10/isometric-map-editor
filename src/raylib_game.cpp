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
#include <string.h>                         // Required for:

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

Vector2i worldSize =  {14, 10 };
Vector2i tileSize = { 64, 32 };
Vector2i origin = { 5, 1 };

int *pWorld = nullptr; // 2D world array

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

Texture2D tileset;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
Rectangle getTileByIndex(int index); // Calculate which tile to display based on provided index
Vector2i toScreen(int x, int y); // transform tile coordinate to screen coordinate, so we know where to draw it


//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    
    InitWindow(screenWidth, screenHeight, "Map making program");
    SetTargetFPS(60);
    
    // load tileset
    // Keep a CPU-side copy (Image) so we can read pixel colours from it,
    // and upload it to the GPU (Texture2D) for drawing.
    Image tilesetImage = LoadImage(TextFormat("%sresources/tileset.png", GetApplicationDirectory()));
    if (!IsImageValid(tilesetImage)) {
        TraceLog(LOG_ERROR, "Could not load tileset");
        CloseWindow();
        return 1;
    }
    tileset = LoadTextureFromImage(tilesetImage);
    int totalTiles = (tileset.width / tileSize.x) * (tileset.height / tileSize.y);
    
    
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
                ++pWorld[selected.y * worldSize.x + selected.x] % totalTiles;
            }
        }

        // Make calculations
        Vector2i selectedCell = { mousePos.x / tileSize.x, mousePos.y / tileSize.y }; // Using integer division
        Vector2i offset = { mousePos.x % tileSize.x, mousePos.y % tileSize.y };


        // Look up the colour under the mouse in the "cheat" tile (index 5) of the tileset
        Rectangle cheatTile = getTileByIndex(5);
        Color pixelColor = GetImageColor(tilesetImage, (int)cheatTile.x + offset.x, (int)cheatTile.y + offset.y);

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
                    // TODO: I need to account for tiles which have a height greater than 1
                    DrawTextureRec(tileset, getTileByIndex(pWorld[y * worldSize.x + x]), Vector2{(float)screenPos.x, (float)screenPos.y}, WHITE);
                    
                }
            }

            // drawing selection info

            // mouse selection overlay thing
            Vector2i selectedPos = toScreen(selected.x, selected.y);
            DrawTextureRec(tileset, getTileByIndex(1), Vector2{(float)selectedPos.x, (float)selectedPos.y}, WHITE);

            // DrawRectangleLines(selectedCell.x * tileSize.x, selectedCell.y * tileSize.y, tileSize.x, tileSize.y, RED);

            std::string mouseInfo = "Mouse  : " + std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y);
            std::string cellInfo = "Cell     : " + std::to_string(selectedCell.x) + ", " + std::to_string(selectedCell.y);
            std::string selectedInfo = "Selected     : " + std::to_string(selected.x) + ", " + std::to_string(selected.y);

            DrawText(mouseInfo.c_str(), 4, 4, 22, DARKGRAY);
            DrawText(cellInfo.c_str(), 4, 34, 22, DARKGRAY);
            DrawText(selectedInfo.c_str(), 4, 64, 22, DARKGRAY);

        }

        EndDrawing();


    }
    // --------------------------------------------------------

  
    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadTexture(tileset);
    UnloadImage(tilesetImage);
    delete[] pWorld;

    CloseWindow();          // Close window and OpenGL context

    //--------------------------------------------------------------------------------------

    return 0;
}


Rectangle getTileByIndex(int index) {
    int tilesPerRow = tileset.width / tileSize.x;
    int idX = index % tilesPerRow;
    int idY = index / tilesPerRow;

    return Rectangle {
        (float)(idX * tileSize.x),
        (float)(idY * tileSize.y),
        (float)tileSize.x,
        (float)tileSize.y,
    };
}

Vector2i toScreen(int x, int y) {
    return Vector2i {
        (origin.x * tileSize.x) + (x - y) * (tileSize.x / 2),
        (origin.y * tileSize.y) + (x + y) * (tileSize.y / 2),
    };
}