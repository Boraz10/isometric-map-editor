#pragma once
#include "raylib.h"
#include <string>

// Maximum characters to be input
#define MAX_INPUT_CHARS 27
#define MAX_INPUT_NUM_CHARS 3 // (999x999)
#define MAX_INPUT_HEX_CHARS 6 // FFFFFF

// Button color definitions
#define btn_MAIN_COLOR  LIGHTGRAY
#define btn_HOVER_COLOR GRAY
#define btn_DOWN_COLOR  DARKGRAY

#define btn_FONT_SIZE_STANDARD 20
#define btn_FONT_SIZE_SMALL 3

// Text field color definitions
#define intxt_MAIN_COLOR GRAY
#define intxt_HOVER_COLOR DARKGRAY

#define TYPING_TEXT_COLOR BLACK

#define intxt_FONT_SIZE 22

#define LABEL_FONT_SIZE 22

// Button, which can be clicked. Functionality to be handeled in main file. Types:
//     0: standard (font size 20)
//     1: small (font size 5)

// TODO: I might be able to take a reference to mousepos into the contructor, which would clean things up a little bit
class Button {
    public:
        Button(Rectangle rectangle, std::string buttonText, int buttonType=0);
        void draw();
        void updateButtonText(std::string& text);
        void checkHovered(Vector2& mousePos);
        bool clicked();

    private:
        Rectangle buttonRect = {-1, -1, -1, -1};
        std::string buttonText = "";
        int textSize = -1;
        Color buttonColor = btn_MAIN_COLOR;
        int fontSize = -1;

        bool hovered = false;

};

// Field to input text. Comes with a label. Can be one of a few types:
//     0: standard (all chars, max 27 chars)
//     1: number (0-9, max 3 chars)
//     2: hex (0-9 and A-F, max 6 chars)
class TextField {
    public:
        TextField(Rectangle rectangle, std::string labelText, int fieldType=0);
        void draw(int &frames);
        void checkHovered(Vector2& mousePos);
        bool hovered = false;

    private:
        Rectangle fieldRect = {-1, -1, -1, -1};
        std::string label = "";
        int labelSize = -1;
        std::string text = "";
        int textLength = -1;
        Color fieldColor = intxt_MAIN_COLOR;


        bool checkCharacterRange(int key);
};

// A simple rectangle with text centered in the middle
class TextBox {
    public:
        TextBox(Rectangle rectangle, std::string text, int fontSize, Color background, Color border);
        void draw();
        void updateText(std::string& newText);
    private:
        Rectangle textBoxRect;
        std::string text;
        int textSize;
        int fontSize;
        Color backgroundColor;
        Color borderColor;
};