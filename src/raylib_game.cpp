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

#define MAX_INPUT_CHARS 27
#define MAX_INPUT_NUM_CHARS 3 // (999x999)

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

Color backgroundColor = RAYWHITE; //background color for the map


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

// Button color definitions
const Color btn_MAIN_COLOR = LIGHTGRAY;
const Color btn_HOVER_COLOR = GRAY;
const Color btn_DOWN_COLOR = DARKGRAY;

const int btn_FONT_SIZE = 20;

// Text field color definitions
const Color intxt_MAIN_COLOR = GRAY;
const Color intxt_HOVER_COLOR = DARKGRAY;

const Color TYPING_TEXT_COLOR = BLACK;

const int intxt_FONT_SIZE = 22;

const int LABEL_FONT_SIZE = 22;

// debug text
const Vector2 ERROR_TEXT_LOCATION = {230, 300};
const int ERROR_TEXT_SIZE = 20;
const Color ERROR_TEXT_COLOR = MAROON;

const Vector2 SUCCESS_TEXT_LOCATION = {230, 300};
const int SUCCESS_TEXT_SIZE = 20;
const Color SUCCESS_TEXT_COLOR = GREEN;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
Vector2i toScreen(int x, int y);                                                                 // transform tile coordinate to screen coordinate, so we know where to draw it
bool loadTileset(std::string fileName, bool localPath = false);                                  // load a tileset from a tileset metadata file.
void displayScreenTitle(const char *text);                                                       // Display a title at the centre top of the screen
void drawButton(Rectangle &buttonRect, Color buttonColor, const char *buttonText, int textSize); // draw a button based on a rectangle, a color and some text
Color updateButtonColor(bool hoverBool);                                                         // update a buttons color depending on if its hovered or clicked

Color updateFieldColor(bool hoverBool);                                                                                                                                                                       // update a fields color depending on if its hovered
void drawTextField(Rectangle &fieldRect, Color fieldColor, const char *labelText, int labelSize, const char *typedText, int &typedTextLetterCount, bool hovered, int framesCounter, bool isNumField = false); // draw a text field

void drawErrorText(const char *text);   // Draws errror text in a specific location
void drawSuccessText(const char *text); // Draws success text in a specific location

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

    const Rectangle rectDimensions = {0, 0, tileSize.x, tileSize.y};

    // Initialise world
    initialiseNewWorld(worldSize, defaultTile);

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

    // Info for all the buttons and fields (TODO: I am preloading this, I think there is a much better and cleaner way to do this)
    Color main_newButtonColor = btn_MAIN_COLOR;
    Color main_loadButtonColor = btn_MAIN_COLOR;

    // main menu

    bool main_isNewHovered = false;
    bool main_isLoadHovered = false;

    Rectangle main_newButton = {(screenWidth / 4) - 150, (screenHeight / 2), 300, 40};
    Rectangle main_loadButton = {(screenWidth / 4) + 250, (screenHeight / 2), 300, 40};

    const char *main_newButtonText = "Create New";
    const char *main_loadButtonText = "Load Map";

    int main_newButtonTextSize = MeasureText(main_newButtonText, btn_FONT_SIZE);
    int main_loadButtonTextSize = MeasureText(main_loadButtonText, btn_FONT_SIZE);

    // Create menu
    bool create_isNameFieldHovered = false;
    bool create_isWidthFieldHovered = false;
    bool create_isHeightFieldHovered = false;
    bool create_isTileFileSelectorHovered = false;

    bool create_isCreateButtonHovered = false;

    Rectangle create_nameField = {(screenWidth / 2) - (350 / 2), 90, 350, 40};
    char *create_nameFieldLabel = "Map Name:";
    int create_nameFieldLabelSize = MeasureText(create_nameFieldLabel, LABEL_FONT_SIZE);
    char create_name[MAX_INPUT_CHARS + 1] = "\0";
    int create_nameLetterCount = 0;
    Color create_nameFieldColor = intxt_MAIN_COLOR;

    Rectangle create_widthField = {(screenWidth / 2) + 80, 150, 60, 40};
    char *create_widthFieldLabel = "Width:";
    int create_widthFieldLabelSize = MeasureText(create_widthFieldLabel, LABEL_FONT_SIZE);
    char create_width[MAX_INPUT_NUM_CHARS + 1] = "\0";
    int create_widthLetterCount = 0;
    Color create_widthFieldColor = intxt_MAIN_COLOR;

    Rectangle create_heightField = {(screenWidth / 2) - 150, 150, 60, 40};
    char *create_heightFieldLabel = "Height:";
    int create_heightFieldLabelSize = MeasureText(create_heightFieldLabel, LABEL_FONT_SIZE);
    char create_height[MAX_INPUT_NUM_CHARS + 1] = "\0";
    int create_heightLetterCount = 0;
    Color create_heightFieldColor = intxt_MAIN_COLOR;

    Rectangle create_tilesetField{(screenWidth / 2) - 200, 210, 400, 40};
    char *create_tilesetFieldText = "Click here to add tileset to use";
    int create_tilesetFieldTextSize = MeasureText(create_tilesetFieldText, btn_FONT_SIZE);
    Color create_tilesetFieldColor = intxt_MAIN_COLOR;

    Rectangle create_colorSelectField = {(screenWidth / 2) - 150, 150, 60, 40};
    char *create_colorSelectFieldLabel = "Background Color (HEX RGB):";
    int create_colorSelectFieldLabelSize = MeasureText(create_heightFieldLabel, LABEL_FONT_SIZE);
    char create_colorSelection[MAX_INPUT_NUM_CHARS + 1] = "\0";
    int create_colorSelectionLetterCount = 0;
    Color create_colorSelectColor = intxt_MAIN_COLOR;

    Rectangle create_defaultTileField;

    Rectangle create_backgroundColorField;

    Rectangle create_createButton = {(screenWidth / 2) - 150, screenHeight - 50, 300, 40};
    const char *create_createButtonText = "Create New Map";
    int create_createButtonTextSize = MeasureText(create_createButtonText, btn_FONT_SIZE);
    Color create_createButtonColor = btn_MAIN_COLOR;

    //---------------------------------------------------------
    // Main Loop
    // --------------------------------------------------------
    while (!WindowShouldClose())
    {
        // -----Update-----
        // Get mouse position
        mousePos = GetMousePosition();

        switch (currentScreen)
        {
        case MAIN_MENU:
        {
            main_isNewHovered = CheckCollisionPointRec(mousePos, main_newButton);
            main_isLoadHovered = CheckCollisionPointRec(mousePos, main_loadButton);

            main_newButtonColor = updateButtonColor(main_isNewHovered);
            main_loadButtonColor = updateButtonColor(main_isLoadHovered);

            if (main_isNewHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                // create button is clicked
                currentScreen = CREATE_MENU;
            }

            if (main_isLoadHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                // load button is clicked
                currentScreen = LOAD_MENU;
            }
        }
        break;
        case CREATE_MENU:
        {
            create_isNameFieldHovered = CheckCollisionPointRec(mousePos, create_nameField);
            create_isWidthFieldHovered = CheckCollisionPointRec(mousePos, create_widthField);
            create_isHeightFieldHovered = CheckCollisionPointRec(mousePos, create_heightField);
            create_isCreateButtonHovered = CheckCollisionPointRec(mousePos, create_createButton);
            create_isTileFileSelectorHovered = CheckCollisionPointRec(mousePos, create_tilesetField);

            create_createButtonColor = updateButtonColor(create_isCreateButtonHovered);
            create_nameFieldColor = updateFieldColor(create_isNameFieldHovered);
            create_heightFieldColor = updateFieldColor(create_isHeightFieldHovered);
            create_widthFieldColor = updateFieldColor(create_isWidthFieldHovered);
            create_tilesetFieldColor = updateButtonColor(create_isTileFileSelectorHovered);

            // TODO: There may be a way to take some of this out into a function.
            if (create_isNameFieldHovered)
            {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);

                int key = GetCharPressed();
                while (key > 0)
                {
                    // NOTE: Only allow keys in range [32..125]
                    if ((key >= 32) && (key <= 125) && (create_nameLetterCount < MAX_INPUT_CHARS))
                    {
                        create_name[create_nameLetterCount] = (char)key;
                        create_name[create_nameLetterCount + 1] = '\0';
                        create_nameLetterCount++;
                    }

                    key = GetCharPressed(); // Check next character in the queue
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    create_nameLetterCount--;
                    if (create_nameLetterCount < 0)
                        create_nameLetterCount = 0;
                    create_name[create_nameLetterCount] = '\0';
                }
            }

            else if (create_isWidthFieldHovered)
            {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
                int key = GetCharPressed();
                while (key > 0)
                {
                    // NOTE: Only allow keys in range [48..57], which is 0 - 9
                    if ((key >= 48) && (key <= 57) && (create_widthLetterCount < MAX_INPUT_NUM_CHARS))
                    {
                        create_width[create_widthLetterCount] = (char)key;
                        create_width[create_widthLetterCount + 1] = '\0';
                        create_widthLetterCount++;
                    }

                    key = GetCharPressed(); // Check next character in the queue
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    create_widthLetterCount--;
                    if (create_widthLetterCount < 0)
                        create_widthLetterCount = 0;
                    create_width[create_widthLetterCount] = '\0';
                }
            }
            else if (create_isHeightFieldHovered)
            {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
                int key = GetCharPressed();
                while (key > 0)
                {
                    // NOTE: Only allow keys in range [48..57], which is 0 - 9
                    if ((key >= 48) && (key <= 57) && (create_heightLetterCount < MAX_INPUT_NUM_CHARS))
                    {
                        create_height[create_heightLetterCount] = (char)key;
                        create_height[create_heightLetterCount + 1] = '\0';
                        create_heightLetterCount++;
                    }

                    key = GetCharPressed(); // Check next character in the queue
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    create_heightLetterCount--;
                    if (create_heightLetterCount < 0)
                        create_heightLetterCount = 0;
                    create_height[create_heightLetterCount] = '\0';
                }
            }

            else if (create_isCreateButtonHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                // create button is clicked
                // If tileset was not selected, or its invalid, load the basic tileset
                if (!tilesetValid) {
                    if (!loadTileset(basicTilesetPath, true))
                    {
                        CloseWindow();
                        return 1;
                    }
                }

                // if size is invalid, set it to default: 20 x 20

                currentScreen = EDITOR;
            }
            else
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);

            if (create_isHeightFieldHovered || create_isWidthFieldHovered || create_isNameFieldHovered)
                framesCounter++;
            else
                framesCounter = 0;

            if (create_isTileFileSelectorHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                tilesetAttemptedOpen = true;

                // tile button is clicked, open a file dialogue
                const char *pathTileset = tinyfd_openFileDialog("Select a tileset file", "", 1, tilesetPattern, ".tileset file", 0);

                // verify file
                if (pathTileset != NULL) {
                    tilesetValid = loadTileset(pathTileset);
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
                drawButton(main_newButton, main_newButtonColor, main_newButtonText, main_newButtonTextSize);

                // Load button
                drawButton(main_loadButton, main_loadButtonColor, main_loadButtonText, main_loadButtonTextSize);
            }
            break;
            case CREATE_MENU:
            {
                // Screen Title Text
                displayScreenTitle("Create New Map");

                // Name field
                drawTextField(create_nameField, create_nameFieldColor, create_nameFieldLabel, create_nameFieldLabelSize,
                              create_name, create_nameLetterCount, create_isNameFieldHovered, framesCounter);

                // Height field
                drawTextField(create_heightField, create_heightFieldColor, create_heightFieldLabel, create_heightFieldLabelSize,
                              create_height, create_heightLetterCount, create_isHeightFieldHovered, framesCounter, true);

                // width field
                drawTextField(create_widthField, create_widthFieldColor, create_widthFieldLabel, create_widthFieldLabelSize,
                              create_width, create_widthLetterCount, create_isWidthFieldHovered, framesCounter, true);

                // Tileset file input
                drawButton(create_tilesetField, create_tilesetFieldColor, create_tilesetFieldText, create_tilesetFieldTextSize);

                // Create button
                drawButton(create_createButton, create_createButtonColor, create_createButtonText, create_createButtonTextSize);
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
    Vector2 titlePos = {(screenWidth - textWidth) / 2, 40};

    DrawText(text, titlePos.x, titlePos.y, titleFontSize, BLACK);
}

void drawButton(Rectangle &buttonRect, Color buttonColor, const char *buttonText, int textSize)
{
    // width and height are consistent
    DrawRectangleRec(buttonRect, buttonColor);
    DrawRectangleLinesEx(buttonRect, 2, BLACK);
    DrawText(buttonText, (buttonRect.x + (buttonRect.width - textSize) / 2), buttonRect.y + 10, 20, BLACK);
}

Color updateButtonColor(bool hoverBool)
{
    if (hoverBool)
    {
        return (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ? btn_DOWN_COLOR : btn_HOVER_COLOR;
    }

    return btn_MAIN_COLOR;
}

Color updateFieldColor(bool hoverBool)
{
    if (hoverBool)
        return intxt_HOVER_COLOR;
    return intxt_MAIN_COLOR;
}

void drawTextField(Rectangle &fieldRect, Color fieldColor, const char *labelText, int labelSize, const char *typedText, int &typedTextLetterCount, bool hovered, int framesCounter, bool isNumField)
{
    int max_chars = (isNumField) ? MAX_INPUT_NUM_CHARS : MAX_INPUT_CHARS;

    DrawText(labelText, fieldRect.x - labelSize - 10, fieldRect.y + 5, LABEL_FONT_SIZE, BLACK);
    DrawRectangleRec(fieldRect, fieldColor);
    DrawRectangleLinesEx(fieldRect, 2.0, BLACK);

    DrawText(typedText, (int)fieldRect.x + 5, (int)fieldRect.y + 8, intxt_FONT_SIZE, TYPING_TEXT_COLOR);

    // Draw blinking line
    if (hovered)
    {
        if (typedTextLetterCount < max_chars)
        {
            if (((framesCounter / 20) % 2) == 0)
                DrawText("_", (int)fieldRect.x + 8 + MeasureText(typedText, intxt_FONT_SIZE), (int)fieldRect.y + 12, intxt_FONT_SIZE, TYPING_TEXT_COLOR);
        }
        // TODO: I should probably take this out into a function
        else
            drawErrorText("Reached Character Limit");
    }
}

void drawErrorText(const char *text)
{
    DrawText(text, ERROR_TEXT_LOCATION.x, ERROR_TEXT_LOCATION.y, ERROR_TEXT_SIZE, ERROR_TEXT_COLOR);
}

void drawSuccessText(const char *text)
{
    DrawText(text, SUCCESS_TEXT_LOCATION.x, SUCCESS_TEXT_LOCATION.y, SUCCESS_TEXT_SIZE, SUCCESS_TEXT_COLOR);
}

void initialiseNewWorld(Vector2i worldSize, int defaultTile) {
    pWorld = new int[worldSize.x * worldSize.y]{ defaultTile };
    pCollisionMap = new bool[worldSize.x * worldSize.y]{false};
}