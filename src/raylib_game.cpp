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

#include "components.h"
#include "utils.h"

#include "tinyfiledialogs.h"

#include <iostream>

#include <filesystem>

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
// Types and structure definitinions
//----------------------------------------------------------------------------------

char const *tilesetPattern[1] = {"*.tileset"};

typedef enum AppScreen
{
    MAIN_MENU = 0,
    CREATE_MENU = 1,
    LOAD_MENU = 2,
    SAVE_MENU = 3,
    EDITOR = 4

} AppScreen;

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

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

// World and map data
Vector2i worldSize = {30, 30};
Vector2i origin = {5, 1};

Color backgroundColor = RAYWHITE; // background color for the map

int *pWorld = nullptr;         // 2D world array
bool *pCollisionMap = nullptr; // 2D collsion map array

// Tileset data
Texture2D tilesetTexture;
int totalTiles;
std::vector<TileDef> tileData; // Table for tileset. I need to do this manually it seems.
Vector2i tileSize = {0, 0};
const char *basicTilesetPath = "basic1.tileset"; // initialise with a pre-packed tileset
int defaultTile = 0;

// Position of the mouse, initialised to 0, 0 and updated every frame
Vector2 mousePos = {0, 0};


//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
Vector2i toScreen(int x, int y);                                                                 // transform tile coordinate to screen coordinate, so we know where to draw it
bool loadTileset(std::string fileName, bool localPath = false);                                  // load a tileset from a tileset metadata file.
void displayScreenTitle(const char *text);                                                       // Display a title at the centre top of the screen

void initialiseNewWorld(Vector2i worldSize, int default_tile); // initilaise a new, empty world with a default tile.

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------

    InitWindow(screenWidth, screenHeight, "Map making program");
    AppScreen currentScreen = MAIN_MENU;
    SetTargetFPS(60);

    int framesCounter = 0;

    // Load cursor and cheat tiles seperately.
    // TODO: Make this whole secion a lot cleaner.
    Texture2D cursorTexture = LoadTexture(TextFormat("%sresources/program_tiles/cursor.png", GetApplicationDirectory()));
    if (!IsTextureValid(cursorTexture))
    {
        TraceLog(LOG_ERROR, "Could not load cursor tile");
        CloseWindow();
        return 1;
    }

    Image cheatTileImage = LoadImage(TextFormat("%sresources/program_tiles/cheat_tile.png", GetApplicationDirectory()));
    if (!IsImageValid(cheatTileImage))
    {
        TraceLog(LOG_ERROR, "Could not load cheat tile");
        CloseWindow();
        return 1;
    }

    // Load collision indicator tile
    Texture2D collisionTexture = LoadTexture(TextFormat("%sresources/program_tiles/collision.png", GetApplicationDirectory()));
    if (!IsTextureValid(collisionTexture))
    {
        TraceLog(LOG_ERROR, "Could not load collision tile");
        CloseWindow();
        return 1;
    }

    // Tileset path
    bool tilesetValid = false;
    bool tilesetAttemptedOpen = false;

    // Source rect for the program tiles (cursor, collision, cheat tile). tileSize is only
    // known once a tileset has been loaded, so this gets set when entering the editor.
    Rectangle rectDimensions = {0, 0, 0, 0};

    // Initialise world
    initialiseNewWorld(worldSize, defaultTile);

    // Initialise selected tile
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

    // Components for menu's

    // main menu
    Button main_newButton(Rectangle{(screenWidth / 4) - 150, (screenHeight / 2), 300, 40}, std::string("Create New"));
    Button main_loadButton(Rectangle{(screenWidth / 4) + 250, (screenHeight / 2), 300, 40}, std::string("Load Map"));

    // Create menu
    TextField create_nameField(Rectangle{(screenWidth / 2) - (350 / 2), 70, 350, 40}, std::string("Map Name: "), 0);
    TextField create_widthField(Rectangle{(screenWidth / 2) + 80, 130, 60, 40}, std::string("Width:"), 1);
    TextField create_heightField(Rectangle{(screenWidth / 2) - 150, 130, 60, 40}, std::string("Height:"), 1);
    Button create_tilesetField(Rectangle{(screenWidth / 2) - 200, 190, 400, 40}, std::string("Click here to add tileset to use"));
    TextField create_colorSelectField(Rectangle{(screenWidth / 2) + 50, 250, 150, 40}, std::string("Background Color (HEX RGB):"), 2);

    TextBox create_defaultTileBox({(screenWidth / 2) - 80, 300, 280, 40}, "", 20, WHITE, BLACK);
    Button create_defaultTileUp(Rectangle{(screenWidth / 2) - 140, 300, 50, 15}, std::string("/\\"), 1);
    Button create_defaultTileDown(Rectangle{(screenWidth / 2) - 140, 325, 50, 15}, std::string("\\/"), 1);

    Button create_createButton(Rectangle{(screenWidth / 2) - 150, screenHeight - 60, 300, 40}, std::string("Create New Map"));

    //---------------------------------------------------------
    // Main Loop
    // --------------------------------------------------------
    int currentCursor = MOUSE_CURSOR_DEFAULT;
    while (!WindowShouldClose())
    {
        // -----Update-----
        // Get mouse position
        mousePos = GetMousePosition();

        // Each screen can change this; it's applied once after the update switch
        int desiredCursor = MOUSE_CURSOR_DEFAULT;

        switch (currentScreen)
        {
        case MAIN_MENU:
        {
            main_newButton.checkHovered(mousePos);
            main_loadButton.checkHovered(mousePos);

            if (main_newButton.clicked())
            {
                // create button is clicked
                currentScreen = CREATE_MENU;
            }

            if (main_loadButton.clicked())
            {
                // load button is clicked
                currentScreen = LOAD_MENU;
            }
        }
        break;
        case CREATE_MENU:
        {
            create_nameField.checkHovered(mousePos);
            create_widthField.checkHovered(mousePos);
            create_heightField.checkHovered(mousePos);
            create_colorSelectField.checkHovered(mousePos);

            create_createButton.checkHovered(mousePos);
            create_tilesetField.checkHovered(mousePos);
            create_defaultTileUp.checkHovered(mousePos);
            create_defaultTileDown.checkHovered(mousePos);

            // update frame counter for cursor icon in text boxes (i might be able to take this into the textField class we shall see)
            if (create_nameField.hovered || create_widthField.hovered || create_heightField.hovered || create_colorSelectField.hovered)
            {
                framesCounter++;
                desiredCursor = MOUSE_CURSOR_IBEAM;
            }
            else
                framesCounter = 0;

            // create button is clicked
            if (create_createButton.clicked())
            {
                // If tileset was not selected, or its invalid, load the basic tileset
                if (!tilesetValid)
                {
                    if (!loadTileset(basicTilesetPath, true))
                    {
                        CloseWindow();
                        return 1;
                    }
                }
                
                // if size is invalid, set it to default: 20 x 20
                // TODO: Do the rest of map initilisation here.

                // tileSize is valid now that a tileset is loaded
                rectDimensions = {0, 0, (float)tileSize.x, (float)tileSize.y};

                currentScreen = EDITOR;
            }

            if (create_tilesetField.clicked())
            {
                tilesetAttemptedOpen = true;

                // tile button is clicked, open a file dialogue
                const char *pathTileset = tinyfd_openFileDialog("Select a tileset file", "", 1, tilesetPattern, ".tileset file", 0);

                // verify file
                if (pathTileset != NULL)
                {
                    tilesetValid = loadTileset(pathTileset);
                    std::string fileName = std::filesystem::path(pathTileset).filename().string();
                    create_tilesetField.updateButtonText(fileName);
                }
            }
            if (tilesetAttemptedOpen)
            {
                if (tilesetValid)
                {
                    drawSuccessText("Tileset loaded successfully");
                }

                else
                {
                    drawErrorText("Could not load tileset");
                }
            }

            // Choosing the default tile
            if (tilesetValid)
            {

                // If valid tileset, look in the TileData vector
                create_defaultTileBox.updateText(tileData.at(defaultTile).name); // I don't like how this is happening each frame

                if (create_defaultTileUp.clicked())
                {
                    defaultTile = (defaultTile + 1) % totalTiles;
                }
                if (create_defaultTileDown.clicked())
                {
                    defaultTile = (defaultTile - 1 + totalTiles) % totalTiles;
                }
            }
        }
        break;
        case LOAD_MENU:
        {
        }
        break;
        case SAVE_MENU:
        {
        }
        break;

        case EDITOR:
        {

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
        }
        break;
        }

        // Only touch the OS cursor when it actually changes
        if (desiredCursor != currentCursor)
        {
            SetMouseCursor(desiredCursor);
            currentCursor = desiredCursor;
        }

        // ------Draw------
        BeginDrawing();
        {                              // Drawing
            ClearBackground(RAYWHITE); // Render background

            switch (currentScreen)
            {
            case MAIN_MENU:
            {
                // Screen Title Text
                displayScreenTitle("Isometric Map Editor");

                // Create button
                main_newButton.draw();

                // Load button
                main_loadButton.draw();
            }
            break;
            case CREATE_MENU:
            {
                // Screen Title Text
                displayScreenTitle("Create New Map");

                // Name field       
                create_nameField.draw(framesCounter);

                // Height field
                create_heightField.draw(framesCounter);

                // width field
                create_widthField.draw(framesCounter);

                // Tileset file input
                create_tilesetField.draw();

                // Color select field
                create_colorSelectField.draw(framesCounter);

                // Default tile text
                DrawText("Default Tile:", (screenWidth / 2) - 288, 305, 22, BLACK);
                // Rectangle{(screenWidth / 2) - 140, 320, 50, 15}

                // Default tile rectangle
                create_defaultTileBox.draw();

                // Up and down buttons
                create_defaultTileUp.draw();
                create_defaultTileDown.draw();


                // Create button
                create_createButton.draw();
            }
            break;
            case LOAD_MENU:
            {
                // Screen Title Text
                displayScreenTitle("Load Existing Map");
            }
            break;
            case SAVE_MENU:
            {
                // Screen Title Text
                displayScreenTitle("Save map");
            }
            break;
            case EDITOR:
            {
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
                                    // Draw on the tile's footprint, not the raised position used for tall sprites
                                    DrawTextureRec(collisionTexture, rectDimensions, Vector2{(float)screenPos.x, (float)screenPos.y}, WHITE);
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
            break;
            }
        }

        EndDrawing();
    }
    //--------------------------------------------------------------------------------------
    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadTexture(tilesetTexture);
    UnloadTexture(cursorTexture);
    UnloadImage(cheatTileImage);
    UnloadTexture(collisionTexture);
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

bool loadTileset(std::string fileName, bool localPath)
{
    std::string dir = "";
    std::string localFileName = "";
    if (localPath)
    {
        dir = std::string(GetApplicationDirectory()) + "resources/tilesets/";
        localFileName = fileName;
    }
    else
    {
        std::filesystem::path p(fileName);
        dir = p.parent_path().string() + "/";
        localFileName = p.filename().string();
    }

    std::ifstream file(dir + localFileName);

    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Could not open tileset file: %s", (dir + localFileName).c_str());
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

void displayScreenTitle(const char *text)
{
    int titleFontSize = 30;
    int textWidth = MeasureText(text, titleFontSize);
    Vector2 titlePos = {(screenWidth - textWidth) / 2, 20};

    DrawText(text, titlePos.x, titlePos.y, titleFontSize, BLACK);
}

void initialiseNewWorld(Vector2i worldSize, int defaultTile)
{
    pWorld = new int[worldSize.x * worldSize.y]{defaultTile};
    pCollisionMap = new bool[worldSize.x * worldSize.y]{false};
}