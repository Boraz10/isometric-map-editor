#pragma once
#include "raylib.h"
#include <string>

// Maximum characters to be input
#define MAX_INPUT_CHARS 27
#define MAX_INPUT_NUM_CHARS 3 // (999x999)
#define MAX_INPUT_HEX_CHARS 6 // FFFFFF
#define MAX_INPUT_CHARS_UNRESTRICTED 999 // for filepaths

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

class Component {
    protected:
        Rectangle rect;
        Vector2& mousePos;
        int& frameCounter;
    public:
        Component(Rectangle rect, Vector2& mousePos, int& frameCounter): rect(rect), mousePos(mousePos), frameCounter(frameCounter) {}
        bool hovered = false;
        virtual void draw() = 0;
        virtual void checkHovered() = 0;

        virtual ~Component() = default;
};


class Button: public Component {
    public:
        Button(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string buttonText, int buttonType=0);
        void draw() override;
        void checkHovered() override;
        void updateButtonText(std::string &text);
        bool clicked();
    private:
        std::string buttonText = "";
        int textSize = -1;
        Color buttonColor = btn_MAIN_COLOR;
        int fontSize = -1;


};

// Field to input text. Comes with a label. Can be one of a few types:
//     0: standard (all chars, max 27 chars)
//     1: number (0-9, max 3 chars)
//     2: hex (0-9 and A-F, max 6 chars)
//     3: unrestricted (all printable chars, max 999 chars, e.g. for file paths)
class TextField: public Component {
    public:
        TextField(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string labelText="", int fieldType=0, std::string defaultText="");
        void draw() override;
        void checkHovered() override;
        std::string getText();
        void setDefaultText(std::string& newText);

    private:
        std::string label = "";
        int labelSize = -1;
        std::string text = "";
        int textLength = -1;
        Color fieldColor = intxt_MAIN_COLOR;


        bool checkCharacterRange(int key);
};

// A simple rectangle with text centered in the middle
class TextBox: public Component {
    public:
        TextBox(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string text, int fontSize, Color background, Color border);
        void draw() override;
        void checkHovered() override;
        void updateText(std::string& newText);
    private:
        std::string text;
        int textSize;
        int fontSize;
        Color backgroundColor;
        Color borderColor;
};

class BackButton: public Component {
    public:
        BackButton(int posX, int posY, Vector2& mousePos, int& frameCounter);
        void draw() override;
        void checkHovered() override;
        bool clicked();

    private:
        Color buttonColor = btn_MAIN_COLOR;
};