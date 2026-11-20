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

#include <iomanip>

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
char const *mapPattern[1] = {"*.bormap"};

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

int framesCounter = 0;

// file data
std::string mapFileName = "";
std::filesystem::path mapFilePath = "";

// World and map data
Vector2i worldSize = {30, 30};
Vector2i origin = {5, 1};
std::string worldName = "";
bool worldNameValid = false;

Color backgroundColor = RAYWHITE; // background color for the map

int *pWorld = nullptr;         // 2D world array
bool *pCollisionMap = nullptr; // 2D collsion map array

// Tileset data
Texture2D tilesetTexture;
int totalTiles;
std::vector<TileDef> tileData; // Table for tileset. I need to do this manually it seems.
Vector2i tileSize = {0, 0};
const char *basicTilesetPath = "basic1.tileset"; // initialise with a pre-packed tileset
std::string mapTilesetPath = basicTilesetPath;
int defaultTile = 0;

// Position of the mouse, initialised to 0, 0 and updated every frame
Vector2 mousePos = {0, 0};

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
Vector2i toScreen(int x, int y);                                // transform tile coordinate to screen coordinate, so we know where to draw it
bool loadTileset(std::string fileName, bool localPath = false); // load a tileset from a tileset metadata file.
void displayScreenTitle(const char *text);                      // Display a title at the centre top of the screen

void initialiseNewWorld(Vector2i worldSize, int default_tile); // initilaise a new, empty world with a default tile.

bool saveMapToFile();
bool loadMapFromFile(char const *fileName);

bool isFilepathValid(std::filesystem::path &);

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------

    InitWindow(screenWidth, screenHeight, "Isometric Map Creator");
    AppScreen currentScreen = MAIN_MENU;
    SetTargetFPS(60);

    // lambda function for changing screen, which also clears the message
    auto changeScreen = [&currentScreen](AppScreen newScreen)
    {
        currentScreen = newScreen;
        clearMessage();
    };


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

    // Map path when loading
    bool loadMapPathValid = false;

    // Source rect for the program tiles (cursor, collision, cheat tile). tileSize is only
    // known once a tileset has been loaded, so this gets set when entering the editor.
    Rectangle rectDimensions = {0, 0, 0, 0};

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
    Rectangle layerRect = {(float)boxPositionX, (float)boxPositionY - 30, (float)box_size, 30};

    // Info for displaying the layer GUI image
    Vector2 layerGUIImagePos = {
        boxPositionX + (box_size - 64) / 2.0f,
        boxPositionY + box_size - box_size / 3.0f - 32};

    // Components for menu's

    // main menu
    Button main_newButton(Rectangle{(screenWidth / 4) - 150, (screenHeight / 2), 300, 40}, mousePos, framesCounter, "Create New");
    Button main_loadButton(Rectangle{(screenWidth / 4) + 250, (screenHeight / 2), 300, 40}, mousePos, framesCounter, "Load Map");

    // Create menu
    TextField create_nameField(Rectangle{(screenWidth / 2) - (350 / 2), 70, 350, 40}, mousePos, framesCounter, "Map Name: ", 0);
    TextField create_widthField(Rectangle{(screenWidth / 2) + 80, 130, 60, 40}, mousePos, framesCounter, "Width:", 1);
    TextField create_heightField(Rectangle{(screenWidth / 2) - 150, 130, 60, 40}, mousePos, framesCounter, "Height:", 1);
    Button create_tilesetField(Rectangle{(screenWidth / 2) - 200, 190, 400, 40}, mousePos, framesCounter, "Click here to add tileset to use");
    TextField create_colorSelectField(Rectangle{(screenWidth / 2) + 50, 250, 150, 40}, mousePos, framesCounter, "Background Color (HEX RGB):", 2);

    TextBox create_defaultTileBox({(screenWidth / 2) - 80, 300, 280, 40}, mousePos, framesCounter, "", 20,  WHITE, BLACK);
    Button create_defaultTileUp(Rectangle{(screenWidth / 2) - 140, 300, 50, 15}, mousePos, framesCounter, std::string("/\\"), 1);
    Button create_defaultTileDown(Rectangle{(screenWidth / 2) - 140, 325, 50, 15}, mousePos, framesCounter, std::string("\\/"), 1);

    BackButton create_backButton(screenWidth - 60, 10, mousePos, framesCounter);

    Button create_createButton(Rectangle{(screenWidth / 2) - 150, screenHeight - 60, 300, 40}, mousePos, framesCounter, "Create New Map");

    // Load menu
    //  loadMapFromFile(char const * fileName)
    TextBox load_filePickerText(Rectangle{(screenWidth / 2) - (350 / 2), 180, 350, 40}, mousePos, framesCounter, "", 20, WHITE, BLACK);
    Button load_filePicker(Rectangle{(screenWidth / 2) - (350 / 2), 230, 350, 40}, mousePos, framesCounter, "Browse...");
    Button load_loadButton(Rectangle{(screenWidth / 2) - 150, screenHeight - 60, 300, 40}, mousePos, framesCounter, "Load");

    BackButton load_backButton(screenWidth - 60, 10, mousePos, framesCounter);

    // Editor screen: Save button
    Button editor_saveButton({(screenWidth - 160), 15, 150, 40},mousePos, framesCounter, std::string("Save"));

    // Save screen
    TextField save_mapName(Rectangle{(screenWidth / 2) - (350 / 2), 130, 350, 40},mousePos, framesCounter, "Map Name:");
    TextBox save_locationText(Rectangle{(screenWidth / 2) - (350 / 2), 220, 350, 40},mousePos, framesCounter, "", 20, WHITE, BLACK);
    Button save_location(Rectangle{(screenWidth / 2) - (350 / 2), 270, 350, 40},mousePos, framesCounter, "Browse...");

    BackButton save_backButton(screenWidth - 60, 10, mousePos, framesCounter);

    Button save_saveButton(Rectangle{(screenWidth / 2) - 150, screenHeight - 60, 300, 40}, mousePos, framesCounter, "Save");

    std::vector<Component*> main_Components = {
        &main_newButton, &main_loadButton
    };

    std::vector<Component*> create_Components = {
    &create_nameField, &create_widthField, &create_heightField,
    &create_tilesetField, &create_colorSelectField, &create_defaultTileBox,
    &create_defaultTileUp, &create_defaultTileDown, &create_backButton,
   &create_createButton
    };

    std::vector<Component*> load_Components = {
    &load_filePickerText, &load_filePicker, &load_loadButton, 
    &load_backButton
    };

    std::vector<Component*> editor_Components = {
        &editor_saveButton
    };

    std::vector<Component*> save_Components = {
        &save_mapName, &save_locationText, &save_location,
        &save_backButton, &save_saveButton
    };

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
            for (Component* c : main_Components) c->checkHovered();

            if (main_newButton.clicked())
            {
                // create button is clicked
                changeScreen(CREATE_MENU);

                // load basic tileset by default, and reset path
                if (!loadTileset(basicTilesetPath, true))
                {
                    CloseWindow();
                    return 1;
                }
                mapTilesetPath = basicTilesetPath;

                defaultTile = 0;
                create_defaultTileBox.updateText(tileData.at(defaultTile).name);
            }

            if (main_loadButton.clicked())
            {
                // load button is clicked
                changeScreen(LOAD_MENU);
            }
        }
        break;
        case CREATE_MENU:
        {
            for (Component* c : create_Components) c->checkHovered();

            // update frame counter for cursor icon in text boxes (i might be able to take this into the textField class we shall see)
            if (create_nameField.hovered || create_widthField.hovered || create_heightField.hovered || create_colorSelectField.hovered)
            {
                framesCounter++;
                desiredCursor = MOUSE_CURSOR_IBEAM;
            }
            else
                framesCounter = 0;

            // back button is clicked
            if (create_backButton.clicked())
            {
                changeScreen(MAIN_MENU);
            }

            // create button is clicked
            if (create_createButton.clicked())
            {
                // Check if world name is empty. If so, we can't create a world
                if (create_nameField.getText().length() <= 0)
                {
                    worldNameValid = false;
                    showErrorMessage("Please input a valid world name");
                }
                else
                {
                    worldNameValid = true;
                    worldName = create_nameField.getText();

                    // set fileName to world name
                    mapFileName = create_nameField.getText();

                    // Try to inintialise width and height. If either is invalid, the default value (30) will be used
                    try
                    {
                        int newX = std::stoi(create_widthField.getText());
                        if (newX <= 0)
                            newX = 1;
                        worldSize.x = newX;
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::cout << "invalid width. Using default (30)" << std::endl;
                    }

                    try
                    {
                        int newY = std::stoi(create_heightField.getText());
                        if (newY <= 0)
                            newY = 1;
                        worldSize.y = newY;
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::cout << "invalid height. Using default (30)" << std::endl;
                    }

                    // Set the background color

                    // I need to parse the string typed in as R G and B
                    // Step one, set the length of the string to 6 if its below by adding trailing Fs
                    std::string hex_string = create_colorSelectField.getText();
                    if (hex_string.length() < 6)
                    {
                        hex_string.append(6 - hex_string.length(), 'F');
                    }

                    // step 2, read every 2 characters, and somehow convert them to an integer from base 16
                    // I could potentially use a for loop here but eh
                    unsigned char backgroundR = std::stoi(hex_string.substr(0, 2), nullptr, 16);
                    unsigned char backgroundG = std::stoi(hex_string.substr(2, 2), nullptr, 16);
                    unsigned char backgroundB = std::stoi(hex_string.substr(4, 2), nullptr, 16);
                    // Then, construct a color, with the alpha being 255 if nescessary
                    Color newBackgroundColor = {backgroundR, backgroundG, backgroundB, 255};
                    // Finally, assign this color to the background color variable
                    backgroundColor = newBackgroundColor;

                    // tileSize is valid now that a tileset is loaded
                    rectDimensions = {0, 0, (float)tileSize.x, (float)tileSize.y};

                    // Initialise world
                    initialiseNewWorld(worldSize, defaultTile);

                    changeScreen(EDITOR);
                }
            }

            if (create_tilesetField.clicked())
            {
                // tile button is clicked, open a file dialogue
                const char *pathTileset = tinyfd_openFileDialog("Select a tileset file", "", 1, tilesetPattern, ".tileset file", 0);

                // verify file
                if (pathTileset != NULL)
                {
                    tilesetValid = loadTileset(pathTileset);
                    mapTilesetPath = pathTileset;
                    if (!tilesetValid)
                    {
                        // Needed: a failed load can clear tileData and unload the texture
                        // part-way through, so the basic tileset has to be reloaded.
                        loadTileset(basicTilesetPath, true);
                        mapTilesetPath = basicTilesetPath;
                        showErrorMessage("Could not load tileset");
                    }
                    else
                    {
                        showSuccessMessage("Tileset loaded successfully");
                    }

                    // reset the default tile
                    defaultTile = 0;
                    create_defaultTileBox.updateText(tileData.at(defaultTile).name);
                }
            }

            // Choosing the default tile
            if (totalTiles > 0)
            {

                // If valid tileset, look in the TileData vector

                if (create_defaultTileUp.clicked())
                {
                    defaultTile = (defaultTile + 1) % totalTiles;
                    create_defaultTileBox.updateText(tileData.at(defaultTile).name);
                }
                if (create_defaultTileDown.clicked())
                {
                    defaultTile = (defaultTile - 1 + totalTiles) % totalTiles;
                    create_defaultTileBox.updateText(tileData.at(defaultTile).name);
                }
            }
        }
        break;
        case LOAD_MENU:
        {
            for (Component* c : load_Components) c->checkHovered();

            if (load_filePicker.clicked())
            {
                // tile button is clicked, open a file dialogue
                const char *pathToMap = tinyfd_openFileDialog("Select a bormap file", "", 1, mapPattern, ".bormap file", 0);

                // verify file
                if (pathToMap != NULL)
                {
                    loadMapPathValid = loadMapFromFile(pathToMap);
                    if (!loadMapPathValid)
                    {
                        showErrorMessage("Could not load map");
                    }
                    else
                    {
                        rectDimensions = {0, 0, (float)tileSize.x, (float)tileSize.y};
                        showSuccessMessage("Map loaded successfully");
                    }

                    mapFileName = std::filesystem::path(pathToMap).filename().string();
                    load_filePickerText.updateText(mapFileName);
                }
            }

            if (load_loadButton.clicked())
            {
                if (loadMapPathValid)
                {
                    changeScreen(EDITOR);
                }
                else
                {
                    showErrorMessage("Please select a valid file to load.");
                }
            }

            // back button is clicked
            if (load_backButton.clicked())
            {
                changeScreen(MAIN_MENU);
            }
        }
        break;
        case SAVE_MENU:
        {
            for (Component* c : save_Components) c->checkHovered();

            // update frame counter for the blinking cursor in the name field
            if (save_mapName.hovered)
            {
                framesCounter++;
                desiredCursor = MOUSE_CURSOR_IBEAM;
            }
            else
                framesCounter = 0;

            // back button is clicked. The name field is only read when Browse or Save is
            // clicked, so going back discards any changes made to it.
            if (save_backButton.clicked())
            {
                changeScreen(EDITOR);
            }

            if (save_location.clicked())
            {
                // open file save location
                std::string defaultName = save_mapName.getText() + ".bormap";
                char const *filepath = tinyfd_saveFileDialog("Choose a save location", defaultName.c_str(), 1, mapPattern, "bormap files");
                if (filepath != NULL)
                {
                    std::filesystem::path chosenPath = filepath;
                    bool accepted = true;

                    // if it doesn't end in .bormap, add .bormap to the end
                    if (chosenPath.extension() != ".bormap")
                    {
                        chosenPath += ".bormap";

                        // The dialog only asked about overwriting the name that was typed,
                        // not the one with .bormap added, so ask again here.
                        if (std::filesystem::exists(chosenPath))
                        {
                            accepted = tinyfd_messageBox("File already exists",
                                                         "A .bormap file with that name already exists. Overwrite it?",
                                                         "yesno", "question", 0) == 1;
                        }
                    }

                    if (accepted)
                    {
                        mapFilePath = chosenPath;

                        // set the text to the filepath
                        // (This is hacky, but it will do for now. I'll fix it up later)
                        std::string displayPath = mapFilePath.string();
                        displayPath = (displayPath.length() >= 30) ? "..." + displayPath.substr(displayPath.length() - 30) : displayPath;
                        save_locationText.updateText(displayPath);

                        std::cout << mapFilePath << std::endl;
                    }
                }
            }

            if (save_saveButton.clicked())
            {
                std::string newName = save_mapName.getText();

                if (newName.empty())
                {
                    showErrorMessage("Please input a valid map name");
                }
                else if (!isFilepathValid(mapFilePath))
                {
                    showErrorMessage(mapFilePath.empty() ? "Choose a save location" : "Can't save to that location");
                }
                else
                {
                    mapFileName = newName;
                    worldName = newName;

                    if (saveMapToFile())
                    {
                        // Return to the editor, and confirm the save there
                        changeScreen(EDITOR);
                        showSuccessMessage("Map saved");
                    }
                    else
                    {
                        showErrorMessage("Could not write map file");
                    }
                }
            }
        }
        break;

        case EDITOR:
        {

            // Save button
            for (Component* c : editor_Components) c->checkHovered();

            if (editor_saveButton.clicked())
            {
                // TODO: if the map was already loaded, it means it already has a file name, in which case we set it to that.
                // TODO: same with the file location
                save_mapName.setDefaultText(mapFileName);

                changeScreen(SAVE_MENU);
            }

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
            if (inBounds && !editor_saveButton.hovered)
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
        { // Drawing

            switch (currentScreen)
            {
            case MAIN_MENU:
            {
                ClearBackground(RAYWHITE); // Render background
                // Screen Title Text
                displayScreenTitle("Isometric Map Editor");

                for (Component* c : main_Components) c->draw();
            }
            break;
            case CREATE_MENU:
            {
                ClearBackground(RAYWHITE); // Render background
                // Screen Title Text
                displayScreenTitle("Create New Map");

                for (Component* c : create_Components) c->draw();
                DrawText("Default Tile:", (screenWidth / 2) - 288, 305, 22, BLACK);
            }
            break;
            case LOAD_MENU:
            {
                ClearBackground(RAYWHITE); // Render background
                // Screen Title Text
                displayScreenTitle("Load Existing Map");

                for (Component* c : load_Components) c->draw();
            }
            break;
            case SAVE_MENU:
            {
                ClearBackground(RAYWHITE); // Render background
                // Screen Title Text
                displayScreenTitle("Save map");

                for (Component* c : save_Components) c->draw();
            }
            break;
            case EDITOR:
            {
                ClearBackground(backgroundColor); // Render background
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

                        }
                    }

                    // second set of for loop, so it draws on top of the map no matter what
                    if (collisionLayer)
                    {
                        for (int y = 0; y < worldSize.y; y++)
                        {
                            for (int x = 0; x < worldSize.x; x++)
                            {
                                Vector2i screenPos = toScreen(x, y);
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

                // save button
                for (Component* c : editor_Components) c->draw();
                break;
            }
            }

            // Status message
            drawMessage();
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
    Vector2 titlePos = {(float)((screenWidth - textWidth) / 2), 20};

    DrawText(text, titlePos.x, titlePos.y, titleFontSize, BLACK);
}

void initialiseNewWorld(Vector2i worldSize, int defaultTile)
{
    delete[] pWorld;
    delete[] pCollisionMap;

    pWorld = new int[worldSize.x * worldSize.y]{0};
    for (int i = 0; i < worldSize.x * worldSize.y; i++)
    {
        pWorld[i] = defaultTile;
    }

    pCollisionMap = new bool[worldSize.x * worldSize.y]{false};
    for (int i = 0; i < worldSize.x * worldSize.y; i++)
    {
        pCollisionMap[i] = false;
    }
}

bool saveMapToFile()
{
    std::ofstream file(mapFilePath);

    if (!file.is_open())
    {
        std::cout << "Error opening file";
        return false;
    }

    // first line is the map name
    file << mapFileName << std::endl;
    // second line is the path to the tileset
    file << mapTilesetPath << std::endl;
    // third line is the background color
    file << std::hex << std::setfill('0')
         << std::setw(2) << (int)backgroundColor.r
         << std::setw(2) << (int)backgroundColor.g
         << std::setw(2) << (int)backgroundColor.b << std::dec << std::endl;
    // fourth line is the size of the map
    file << std::dec << worldSize.x << " " << worldSize.y << std::endl;
    // remaining lines are the rest of the map and collision data
    for (int y = 0; y < worldSize.y; y++)
    {
        for (int x = 0; x < worldSize.x; x++)
        {
            file << pWorld[y * worldSize.x + x] << " " << pCollisionMap[y * worldSize.x + x] << " ";
        }
        file << std::endl;
    }

    // for safety
    file.close();
    return !file.fail();
}

bool loadMapFromFile(char const *fileName)
{
    // read the file, line by line, and assign world info accordingly
    std::ifstream file(fileName);

    if (!file.is_open())
    {
        showErrorMessage("Could not open file");
        return false;
    }

    std::string line;
    // first line is the map name
    std::getline(file, line);
    worldName = line;

    // second line is the path to the tileset in use
    std::getline(file, line);
    mapTilesetPath = line;

    // load the tileset
    if (mapTilesetPath == basicTilesetPath)
    {
        loadTileset(mapTilesetPath, true);
    }
    else
    {
        loadTileset(mapTilesetPath, false);
    }

    // third line is the bacground colour
    std::getline(file, line);
    unsigned char backgroundR = std::stoi(line.substr(0, 2), nullptr, 16);
    unsigned char backgroundG = std::stoi(line.substr(2, 2), nullptr, 16);
    unsigned char backgroundB = std::stoi(line.substr(4, 2), nullptr, 16);
    backgroundColor = {backgroundR, backgroundG, backgroundB, 255};

    // fourth line is the size of the map
    std::getline(file, line);
    {
        std::istringstream iss(line);
        iss >> worldSize.x >> worldSize.y;
    }

    // remaining lines consist of pairs (block int, collision bool)
    delete[] pWorld;
    delete[] pCollisionMap;

    pWorld = new int[worldSize.x * worldSize.y]{0};
    pCollisionMap = new bool[worldSize.x * worldSize.y]{false};

    int rowsRead = 0;
    while (rowsRead < worldSize.y && std::getline(file, line))
    {
        std::istringstream iss(line);

        for (int x = 0; x < worldSize.x; x++)
        {
            int i = rowsRead * worldSize.x + x;
            if (!(iss >> pWorld[i] >> pCollisionMap[i]))
                return false;
        }
        rowsRead++;
    }

    file.close();
    return rowsRead == worldSize.y;
}

bool isFilepathValid(std::filesystem::path &path)
{
    std::error_code ec;

    if (!path.has_filename())
    {
        return false;
    }

    std::filesystem::path parent = path.parent_path();
    if (!parent.empty() && (!std::filesystem::exists(parent, ec) || !std::filesystem::is_directory(parent, ec)))
    {
        return false;
    }

    std::ofstream testFile(path, std::ios::app);
    return testFile.is_open();
}