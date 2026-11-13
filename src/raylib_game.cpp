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
    tileset = LoadTexture(TextFormat("%sresources/tileset.png", GetApplicationDirectory()));
    if (!IsTextureValid(tileset)) {
        TraceLog(LOG_ERROR, "Could not load tileset");
        CloseWindow();
        return 1;
    }
    

    // Initialise world
    pWorld = new int[worldSize.x * worldSize.y]{ 0 };

    //---------------------------------------------------------


    // Main Loop
    // --------------------------------------------------------
    while (!WindowShouldClose()) {
        // -----Update-----
        // Poll inputs

        // ------Draw------
        BeginDrawing();
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
            
        EndDrawing();


    }
    // --------------------------------------------------------

  
    // De-Initialization
    //--------------------------------------------------------------------------------------

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