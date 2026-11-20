#include "components.h"
#include "utils.h"


Button::Button(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string buttonText, int buttonType)
: Component(rectangle, mousePos, frameCounter) {
    this->buttonText = buttonText;

    this->fontSize = (buttonType == 1)
                         ? btn_FONT_SIZE_SMALL
                         : btn_FONT_SIZE_STANDARD;

    this->textSize = MeasureText(buttonText.c_str(), fontSize);
}
void Button::draw()
{
    DrawRectangleRec(rect, buttonColor);
    DrawRectangleLinesEx(rect, 2, BLACK);
    // TODO: again, I should really be checking on the button type, not the text size
    if (fontSize == btn_FONT_SIZE_SMALL)
    {
        DrawText(buttonText.c_str(), (rect.x + (rect.width - textSize) / 2), rect.y + 3, fontSize, BLACK);
    }
    else
    {
        DrawText(buttonText.c_str(), (rect.x + (rect.width - textSize) / 2), rect.y + 10, fontSize, BLACK);
    }
}
void Button::updateButtonText(std::string &text)
{
    this->buttonText = text;
    this->textSize = MeasureText(buttonText.c_str(), fontSize);
}
void Button::checkHovered()
{
    if (CheckCollisionPointRec(mousePos, rect))
    {
        this->buttonColor = btn_HOVER_COLOR;
        this->hovered = true;
    }
    else
    {
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



TextField::TextField(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string labelText, int fieldType, std::string defaultText)
: Component(rectangle, mousePos, frameCounter)
{
    this->label = labelText;
    this->labelSize = MeasureText(labelText.c_str(), LABEL_FONT_SIZE);
    this->textLength =
        (fieldType == 3) ? MAX_INPUT_CHARS_UNRESTRICTED : (fieldType == 2) ? MAX_INPUT_HEX_CHARS
                                                      : (fieldType == 1)   ? MAX_INPUT_NUM_CHARS
                                                                           : MAX_INPUT_CHARS;

    this->text = defaultText;
}
void TextField::draw()
{
    DrawText(label.c_str(), rect.x - labelSize - 10, rect.y + 5, LABEL_FONT_SIZE, BLACK);
    DrawRectangleRec(rect, fieldColor);
    DrawRectangleLinesEx(rect, 2.0, BLACK);

    DrawText(text.c_str(), (int)rect.x + 5, (int)rect.y + 8, intxt_FONT_SIZE, TYPING_TEXT_COLOR);

    // Draw blinking line
    if (hovered && text.length() < textLength)
    {
        if (((frameCounter / 20) % 2) == 0)
            DrawText("_", (int)rect.x + 8 + MeasureText(text.c_str(), intxt_FONT_SIZE), (int)rect.y + 12, intxt_FONT_SIZE, TYPING_TEXT_COLOR);
    }
}

void TextField::checkHovered()
{
    if (CheckCollisionPointRec(mousePos, rect))
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

        if (this->text.length() >= this->textLength)
            showErrorMessage("Reached Character Limit", 0.1f);
    }
    else
    {
        this->hovered = false;
        this->fieldColor = intxt_MAIN_COLOR;
    }
}

bool TextField::checkCharacterRange(int key)
{
    // I should really be checking based on the text type and not the length.
    if (this->textLength == MAX_INPUT_CHARS)
    {
        // Standard: keys in range [32..125]
        if ((key >= 32) && (key <= 125))
        {
            return true;
        }
        return false;
    }

    if (this->textLength == MAX_INPUT_NUM_CHARS)
    {
        // Number: keys in range [48..57] (0 - 9)
        if ((key >= 48) && (key <= 57))
        {
            return true;
        }
        return false;
    }

    if (this->textLength == MAX_INPUT_HEX_CHARS)
    {
        // Hex: keys in range [48..57] (0 - 9) and [65..70] (A - F)
        if (((key >= 48) && (key <= 57)) || ((key >= 65) && (key <= 70)) || ((key >= 97) && (key <= 102)))
        {
            return true;
        }
        return false;
    }

    if (this->textLength == MAX_INPUT_CHARS_UNRESTRICTED)
    {
        // Unrestricted: every printable ASCII character [32..126]
        return (key >= 32) && (key <= 126);
    }
    return false;
}

std::string TextField::getText()
{
    return this->text;
}

void TextField::setDefaultText(std::string &newText)
{
    this->text = newText;
}

TextBox::TextBox(Rectangle rectangle, Vector2& mousePos, int& frameCounter, std::string text, int fontSize, Color background, Color border)
: Component(rectangle, mousePos, frameCounter)
{
    this->text = text;
    this->textSize = MeasureText(text.c_str(), fontSize);
    this->fontSize = fontSize;
    this->backgroundColor = background;
    this->borderColor = border;
}
void TextBox::draw()
{
    DrawRectangleRec(rect, backgroundColor);
    DrawRectangleLinesEx(rect, 2, borderColor);
    DrawText(text.c_str(), (rect.x + (rect.width - textSize) / 2), rect.y + 10, fontSize, BLACK);
}

void TextBox::checkHovered() {}

void TextBox::updateText(std::string &newText)
{
    this->text = newText;
    this->textSize = MeasureText(text.c_str(), fontSize);
}

BackButton::BackButton(int posX, int posY, Vector2& mousePos, int& frameCounter)
: Component({(float)posX, (float)posY, 50, 50}, mousePos, frameCounter) {
    this->buttonColor = btn_MAIN_COLOR;
}
void BackButton::draw() {
    DrawRectangleRec(rect, buttonColor);
    DrawRectangleLinesEx(rect, 2, BLACK);

    DrawText("Back", (rect.x + (rect.width - 40) / 2), rect.y + 15, 18, BLACK);
}

void BackButton::checkHovered() {
    if (CheckCollisionPointRec(mousePos, rect))
    {
        this->buttonColor = btn_HOVER_COLOR;
        this->hovered = true;
    }
    else
    {
        this->buttonColor = btn_MAIN_COLOR;
        this->hovered = false;
    }

}

bool BackButton::clicked() {
    if (hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        this->buttonColor = btn_DOWN_COLOR;
    }
    if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        return true;
    }
    return false;

}