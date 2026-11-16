#include "raylib.h"

typedef struct Timer {
    float lifeTime;   // Time remaining in seconds
} Timer;

// Start or reset a timer with a specific duration
void StartTimer(Timer *timer, float lifetime) {
    if (timer != NULL) timer->lifeTime = lifetime;
}

// Update the timer every frame using delta time
void UpdateTimer(Timer *timer) {
    if (timer != NULL && timer->lifeTime > 0) {
        timer->lifeTime -= GetFrameTime();
    }
}

// Check if the timer has finished
bool TimerDone(Timer timer) {
    return timer.lifeTime <= 0;
}