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
#include <emscripten/emscripten.h> // Emscripten library
#endif

#include <stdio.h>  // Required for: printf()
#include <stdlib.h> // Required for:
#include <string>   // Required for:
#include <vector>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

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
typedef struct Vector2i
{
    int x;
    int y;

    Vector2i &operator+=(const Vector2i &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
} Vector2i;

typedef struct TileDef
{
    std::string name;
    Rectangle src;
} TileDef;

Vector2i worldSize = {50, 50};
Vector2i origin = {5, 1};

Vector2i tileSize = {0, 0};
Vector2 mousePos = {0, 0};
int *pWorld = nullptr;         // 2D world array
bool *pCollisionMap = nullptr; // 2D collsion map array

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
Vector2i toScreen(int x, int y);        // transform tile coordinate to screen coordinate, so we know where to draw it
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

    // Load cursor and cheat tiles seperately.
    // TODO: Make this whole secion a lot cleaner.
    Texture2D cursorTexture = LoadTexture(TextFormat("%sresources/cursor.png", GetApplicationDirectory()));
    if (!IsTextureValid(cursorTexture))
    {
        TraceLog(LOG_ERROR, "Could not load cursor tile");
        CloseWindow();
        return 1;
    }

    Image cheatTileImage = LoadImage(TextFormat("%sresources/cheat_tile.png", GetApplicationDirectory()));
    if (!IsImageValid(cheatTileImage))
    {
        TraceLog(LOG_ERROR, "Could not load cheat tile");
        CloseWindow();
        return 1;
    }

    // Load collision indicator tile
    Texture2D collisionTexture = LoadTexture(TextFormat("%sresources/collision.png", GetApplicationDirectory()));
    if (!IsTextureValid(collisionTexture))
    {
        TraceLog(LOG_ERROR, "Could not load collision tile");
        CloseWindow();
        return 1;
    }

    // Load tileset
    if (!loadTileset("basic1.tileset"))
    {
        CloseWindow();
        return 1;
    }

    const Rectangle rectDimensions = {0, 0, tileSize.x, tileSize.y};

    // Initialise world
    pWorld = new int[worldSize.x * worldSize.y]{0};
    pCollisionMap = new bool[worldSize.x * worldSize.y]{false};

    // Initialise mouse info
    Vector2i selected = {0, 0};

    // Initialise camera info
    Camera2D camera = {};
    camera.zoom = 1.0f;

    // Initialise tile selection info
    int currentTile = 0;

    // Info for positioning of the GUI
    int box_size = 128;
    int box_offset = 8;

    int boxPositionX = box_offset;
    int boxPositionY = screenHeight - box_size - box_offset;

    // Layer boolean
    bool collisionLayer = false; // false = Map layer, true = Collision layer
    bool collisionBrush = true;

    // Rectangle displaying currentLayer
    Rectangle layerRect = {boxPositionX, boxPositionY - 30, box_size, 30};

    // Info for displaying the layer GUI image
    Vector2 layerGUIImagePos = {
        boxPositionX + (box_size - 64) / 2.0f,
        boxPositionY + box_size - box_size / 3.0f - 32};

    //---------------------------------------------------------

    // Main Loop
    // --------------------------------------------------------
    while (!WindowShouldClose())
    {
        // -----Update-----
        // Get mouse position
        mousePos = GetMousePosition();

        // left and right arrow keys switch current tile
        if (!collisionLayer)
        {
            if (IsKeyPressed(KEY_RIGHT))
                currentTile = (currentTile + 1) % totalTiles;
            if (IsKeyPressed(KEY_LEFT))
                currentTile = (currentTile - 1 + totalTiles) % totalTiles;
        }

        // up arrow key switched between collision and map layers
        if (IsKeyPressed(KEY_UP))
        {
            collisionLayer = !collisionLayer;
        }

        // Panning with the right mouse button
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 delta = GetMouseDelta(); // change in mouse position over one frame
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom; // divide so we pan relative to zoom
        }

        // Zooming with mouse wheel
        float wheel = GetMouseWheelMove();

        if (wheel != 0)
        {
            Vector2 mouseWorld = GetScreenToWorld2D(mousePos, camera);
            camera.offset = mousePos;   // zoom around the mouse
            camera.target = mouseWorld; // pin to same spot on map
            camera.zoom = std::clamp(camera.zoom * powf(1.25f, wheel), 0.25f, 8.0f);
        }

        // Calculate mouse position in relation to world camera
        Vector2 mouseWorld = GetScreenToWorld2D(mousePos, camera);
        int mx = (int)floorf(mouseWorld.x);
        int my = (int)floorf(mouseWorld.y);

        // Calculate selected cell and offset based on mouse position in the world
        Vector2i selectedCell = {(int)floorf((float)mx / tileSize.x), (int)floorf((float)my / tileSize.y)};
        Vector2i offset = {mx - selectedCell.x * tileSize.x, my - selectedCell.y * tileSize.y};

        // Look up the colour under the mouse in the cheat tile
        Color pixelColor = GetImageColor(cheatTileImage, (int)rectDimensions.x + offset.x, (int)rectDimensions.y + offset.y);

        // calculate the selected tile based on the selected cell (as a 2d grid), taking into account the origin as an offset.
        selected = {
            (selectedCell.y - origin.y) + (selectedCell.x - origin.x),
            (selectedCell.y - origin.y) - (selectedCell.x - origin.x)};

        // Offset selected tile based on what color is highlighted on the cheat tile
        if (ColorIsEqual(pixelColor, Color{255, 0, 0, 255}))
            selected += {-1, +0}; // RED    (top-left corner)
        if (ColorIsEqual(pixelColor, Color{0, 255, 0, 255}))
            selected += {+0, -1}; // GREEN  (top-right corner)
        if (ColorIsEqual(pixelColor, Color{0, 0, 255, 255}))
            selected += {+0, +1}; // BLUE   (bottom-left corner)
        if (ColorIsEqual(pixelColor, Color{240, 255, 0, 255}))
            selected += {+1, +0}; // YELLOW (bottom-right corner)

        // Placing tiles with the left mouse button

        bool inBounds = (selected.x >= 0 && selected.x < worldSize.x && selected.y >= 0 && selected.y < worldSize.y);
        if (inBounds)
        {
            int idx = selected.y * worldSize.x + selected.x;
            if (collisionLayer)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    collisionBrush = !pCollisionMap[idx];
                }

                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    pCollisionMap[idx] = collisionBrush;
                }
            }
            else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                pWorld[idx] = currentTile;
            }
        }

        // ------Draw------
        BeginDrawing();

        {                              // Drawing
            ClearBackground(RAYWHITE); // Render background

            BeginMode2D(camera);
            {
                // Iteterate through every cell, from top top bottom, since bottom is closer and should be rendered later.
                for (int y = 0; y < worldSize.y; y++)
                {
                    for (int x = 0; x < worldSize.x; x++)
                    {
                        Vector2i screenPos = toScreen(x, y);

                        // figure out which tile to draw
                        Rectangle src = tileData[pWorld[y * worldSize.x + x]].src;
                        Vector2 pos = {
                            (float)screenPos.x,
                            (float)screenPos.y - (src.height - tileSize.y)};
                        DrawTextureRec(tilesetTexture, src, pos, WHITE);

                        // Draw the collision layer on top of the texture layer, if collision mode is on
                        if (collisionLayer)
                        {
                            if (pCollisionMap[y * worldSize.x + x])
                            {
                                DrawTextureRec(collisionTexture, rectDimensions, pos, WHITE);
                            }
                        }
                    }
                }

                // selection cursor
                Vector2i selectedPos = toScreen(selected.x, selected.y);
                DrawTextureRec(cursorTexture, rectDimensions, Vector2{(float)selectedPos.x, (float)selectedPos.y}, WHITE);

                // DrawRectangleLines(selectedCell.x * tileSize.x, selectedCell.y * tileSize.y, tileSize.x, tileSize.y, RED);
            }
            EndMode2D();

            DrawText(TextFormat("Selected : %d, %d", selected.x, selected.y), 4, 4, 22, DARKGRAY);

            // white box
            DrawRectangle(boxPositionX, boxPositionY, box_size, box_size, RAYWHITE);

            // box border
            DrawRectangleLines(boxPositionX, boxPositionY, box_size, box_size, BLACK);

            TileDef &tile = tileData[currentTile];
            // Draw sprite of the actual tile in use
            Vector2 tilePos = {
                boxPositionX + (box_size - tile.src.width) / 2.0f,
                boxPositionY + box_size - box_size / 3.0f - tile.src.height};

            if (collisionLayer)
            {
                DrawTextureRec(collisionTexture, rectDimensions, layerGUIImagePos, WHITE);
                DrawText("Collisions", boxPositionX + box_offset, boxPositionY + box_size - box_offset * 3, 18, BLACK);
            }
            else
            {
                DrawTextureRec(tilesetTexture, tile.src, tilePos, WHITE);
                DrawText((std::to_string(currentTile) + ". " + tile.name).c_str(), boxPositionX + box_offset, boxPositionY + box_size - box_offset * 3, 18, BLACK);
            }

            // Draw box for switching layer
            DrawRectangle(layerRect.x, layerRect.y, layerRect.width, layerRect.height, RAYWHITE);
            DrawRectangleLines(layerRect.x, layerRect.y, layerRect.width, layerRect.height, BLACK);
            DrawText(TextFormat("Layer: %s", ((collisionLayer) ? "Collision" : "Map")),
                     layerRect.x + 6, layerRect.y + 6, 16, BLACK);
        }

        EndDrawing();
    }
    // --------------------------------------------------------

    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadTexture(tilesetTexture);
    UnloadTexture(cursorTexture);
    UnloadTexture(collisionTexture);
    UnloadImage(cheatTileImage);
    delete[] pWorld;
    delete[] pCollisionMap;

    CloseWindow(); // Close window and OpenGL context

    //--------------------------------------------------------------------------------------

    return 0;
}

Vector2i toScreen(int x, int y)
{
    return Vector2i{
        (origin.x * tileSize.x) + (x - y) * (tileSize.x / 2),
        (origin.y * tileSize.y) + (x + y) * (tileSize.y / 2),
    };
}

bool loadTileset(std::string fileName)
{
    std::string dir = std::string(GetApplicationDirectory()) + "resources/tilesets/";

    std::ifstream file(dir + fileName);

    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Could not open tileset file: %s", (dir + fileName).c_str());
        return false;
    }

    // unload previous tileset, just in case
    tileData.clear();
    if (IsTextureValid(tilesetTexture))
        UnloadTexture(tilesetTexture);

    std::string line;

    // Line 1 is the tileset image source
    std::getline(file, line);
    std::istringstream imageLine(line);
    std::string imageName;
    imageLine >> imageName;

    tilesetTexture = LoadTexture((dir + imageName).c_str());
    if (!IsTextureValid(tilesetTexture))
    {
        TraceLog(LOG_ERROR, "%s: could not load image '%s'", fileName.c_str(), imageName.c_str());
        return false;
    }

    // Line 2 is the tile size footprint
    std::getline(file, line);
    std::istringstream sizeLine(line);
    if (!(sizeLine >> tileSize.x >> tileSize.y) || tileSize.x <= 0 || tileSize.y <= 0)
    {
        TraceLog(LOG_ERROR, "%s: line 2 should be the tile size, e.g. '64 32'", fileName.c_str());
        return false;
    }

    // the remaining lines populate the tiledefs struct in format: x y w h
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        TileDef t;
        if (iss >> t.name >> t.src.x >> t.src.y >> t.src.width >> t.src.height)
        {
            tileData.push_back(t);
        }
    }

    totalTiles = (int)tileData.size();
    return totalTiles > 0;
}