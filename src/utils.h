#pragma once
#include "raylib.h"

// debug text

#define ERROR_TEXT_LOCATION_X 230
#define ERROR_TEXT_LOCATION_Y 350

#define ERROR_TEXT_SIZE 20
#define ERROR_TEXT_COLOR MAROON

#define SUCCESS_TEXT_LOCATION_X 230
#define SUCCESS_TEXT_LOCATION_Y 350
#define SUCCESS_TEXT_SIZE 20
#define SUCCESS_TEXT_COLOR GREEN

// TODO: I want one buffer for both of these, so text doesn't accidentally overlap
inline void drawErrorText(const char *text)
{
    DrawText(text, ERROR_TEXT_LOCATION_X, ERROR_TEXT_LOCATION_Y, ERROR_TEXT_SIZE, ERROR_TEXT_COLOR);
}

inline void drawSuccessText(const char *text)
{
    DrawText(text, SUCCESS_TEXT_LOCATION_X, SUCCESS_TEXT_LOCATION_Y, SUCCESS_TEXT_SIZE, SUCCESS_TEXT_COLOR);
}