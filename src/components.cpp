#include "components.h"
#include "utils.h"

Button::Button(Rectangle rectangle, std::string buttonText, int buttonType)
{
    this->buttonRect = rectangle;
    this->buttonText = buttonText;

    this->fontSize = (buttonType == 1)
                   ? btn_FONT_SIZE_SMALL
                   : btn_FONT_SIZE_STANDARD;

    this->textSize = MeasureText(buttonText.c_str(), fontSize);
}
void Button::draw()
{
    DrawRectangleRec(buttonRect, buttonColor);
    DrawRectangleLinesEx(buttonRect, 2, BLACK);
    // TODO: again, I should really be checking on the button type, not the text size
    if (fontSize == btn_FONT_SIZE_SMALL) {
        DrawText(buttonText.c_str(), (buttonRect.x + (buttonRect.width - textSize) / 2), buttonRect.y + 3, fontSize, BLACK);
    }
    else {
        DrawText(buttonText.c_str(), (buttonRect.x + (buttonRect.width - textSize) / 2), buttonRect.y + 10, fontSize, BLACK);
    }
}
void Button::updateButtonText(std::string &text)
{
    this->buttonText = text;
    this->textSize = MeasureText(buttonText.c_str(), fontSize);
}
void Button::checkHovered(Vector2 &mousePos)
{
    if (CheckCollisionPointRec(mousePos, buttonRect))
    {
        this->buttonColor = btn_HOVER_COLOR;
        this->hovered = true;
    }
    else {
        this->buttonColor = btn_MAIN_COLOR;
        this->hovered = false;
    }
}

bool Button::clicked()
{
     if (hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        this->buttonColor = btn_DOWN_COLOR;
    }

    if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        return true;
    }
    return false;
}




TextField::TextField(Rectangle rectangle, std::string labelText, int fieldType)
{
    this->fieldRect = rectangle;
    this->label = labelText;
    this->labelSize = MeasureText(labelText.c_str(), LABEL_FONT_SIZE);
    this->textLength =
        (fieldType == 2) ? MAX_INPUT_HEX_CHARS : (fieldType == 1) ? MAX_INPUT_NUM_CHARS
                                                                  : MAX_INPUT_CHARS;
}
void TextField::draw(int &frames)
{
    DrawText(label.c_str(), fieldRect.x - labelSize - 10, fieldRect.y + 5, LABEL_FONT_SIZE, BLACK);
    DrawRectangleRec(fieldRect, fieldColor);
    DrawRectangleLinesEx(fieldRect, 2.0, BLACK);

    DrawText(text.c_str(), (int)fieldRect.x + 5, (int)fieldRect.y + 8, intxt_FONT_SIZE, TYPING_TEXT_COLOR);

    // Draw blinking line
    if (hovered)
    {
        if (text.length() < textLength)
        {
            if (((frames / 20) % 2) == 0)
                DrawText("_", (int)fieldRect.x + 8 + MeasureText(text.c_str(), intxt_FONT_SIZE), (int)fieldRect.y + 12, intxt_FONT_SIZE, TYPING_TEXT_COLOR);
        }
        else
            drawErrorText("Reached Character Limit");
    }
}

void TextField::checkHovered(Vector2 &mousePos)
{
    if (CheckCollisionPointRec(mousePos, fieldRect))
    {
        this->hovered = true;
        this->fieldColor = intxt_HOVER_COLOR;

        int key = GetCharPressed();
        while (key > 0)
        {
            if (checkCharacterRange(key) && (this->text.length() < this->textLength))
                this->text += (char)key;

            key = GetCharPressed(); // Check next character in the queue
        }

        if (IsKeyPressed(KEY_BACKSPACE))
        {
            if (this->text.length() > 0)
                this->text.pop_back();
        }
    }
    else
    {
        this->hovered = false;
        this->fieldColor = intxt_MAIN_COLOR;
    }
}

bool TextField::checkCharacterRange(int key) {
    // I should really be checking based on the text type and not the length. 
    if (this->textLength == MAX_INPUT_CHARS) {
        // Standard: keys in range [32..125]
        if ((key >= 32) && (key <= 125)) {
            return true;
        }
        return false;
    }

    if (this->textLength == MAX_INPUT_NUM_CHARS) {
        // Number: keys in range [48..57] (0 - 9)
        if ((key >= 48) && (key <= 57)) {
            return true;
        }
        return false;
    }

    if (this->textLength == MAX_INPUT_HEX_CHARS) {
        // Hex: keys in range [48..57] (0 - 9) and [65..70] (A - F)
        if (((key >= 48) && (key <= 57)) || ((key >= 65) && (key <= 70)) || ((key >= 97) && (key <= 102))) {
            return true;
        }
        return false;
    }
    return false;
}

std::string TextField::getText() {
    return this->text;
}



TextBox::TextBox(Rectangle rectangle, std::string text, int fontSize, Color background, Color border) {
    this->textBoxRect = rectangle;
    this->text = text;
    this->textSize = MeasureText(text.c_str(), fontSize);
    this->fontSize = fontSize;
    this->backgroundColor = background;
    this->borderColor = border;
}
void TextBox::draw() {
    DrawRectangleRec(textBoxRect, backgroundColor);
    DrawRectangleLinesEx(textBoxRect, 2, borderColor);
    DrawText(text.c_str(), (textBoxRect.x + (textBoxRect.width - textSize) / 2), textBoxRect.y + 10, fontSize, BLACK);
}

void TextBox::updateText(std::string& newText) {
    this->text = newText;
    this->textSize = MeasureText(text.c_str(), fontSize);
}