#pragma once
#include "raylib.h"
#include <string>

// There is a single message buffer, so showing a new message replaces the old one

#define MESSAGE_LOCATION_Y 350 // centred horizontally
#define MESSAGE_TEXT_SIZE 20
#define MESSAGE_DURATION 3.0f 

#define ERROR_TEXT_COLOR MAROON
#define SUCCESS_TEXT_COLOR GREEN

struct StatusMessage
{
    std::string text;
    Color color;
    double hideAt;
};

inline StatusMessage statusMessage = {"", BLACK, 0.0};

inline void showMessage(const std::string &text, Color color, float duration = MESSAGE_DURATION)
{
    statusMessage = {text, color, GetTime() + duration};
}

inline void showErrorMessage(const std::string &text, float duration = MESSAGE_DURATION)
{
    showMessage(text, ERROR_TEXT_COLOR, duration);
}

inline void showSuccessMessage(const std::string &text, float duration = MESSAGE_DURATION)
{
    showMessage(text, SUCCESS_TEXT_COLOR, duration);
}

inline void clearMessage()
{
    statusMessage.text.clear();
}

inline void drawMessage()
{
    if (statusMessage.text.empty() || GetTime() > statusMessage.hideAt)
        return;

    int textWidth = MeasureText(statusMessage.text.c_str(), MESSAGE_TEXT_SIZE);
    DrawText(statusMessage.text.c_str(), (GetScreenWidth() - textWidth) / 2, MESSAGE_LOCATION_Y, MESSAGE_TEXT_SIZE, statusMessage.color);
}
