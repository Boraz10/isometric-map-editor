#pragma once
#include "raylib.h"

typedef struct Timer {
    float lifeTime;   // Time remaining in seconds
} Timer;

// Start or reset a timer with a specific duration
inline void StartTimer(Timer *timer, float lifetime) {
    if (timer != NULL) timer->lifeTime = lifetime;
}

// Update the timer every frame using delta time
inline void UpdateTimer(Timer *timer) {
    if (timer != NULL && timer->lifeTime > 0) {
        timer->lifeTime -= GetFrameTime();
    }
}

// Check if the timer has finished
inline bool TimerDone(Timer timer) {
    return timer.lifeTime <= 0;
}