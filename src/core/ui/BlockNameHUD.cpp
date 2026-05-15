#include "core/ui/BlockNameHUD.h"

void BlockNameHUD::update(float deltaTime, int selectedID, const std::string& name) {
    if (selectedID != lastSeenID) {
        lastSeenID  = selectedID;
        currentName = name;
        timeLeft    = TOTAL_SECONDS;
        return;
    }

    if (timeLeft > 0.0f) {
        timeLeft -= deltaTime;
        if (timeLeft < 0.0f) timeLeft = 0.0f;
    }
}

bool BlockNameHUD::visible() const {
    return timeLeft > 0.0f && !currentName.empty();
}

float BlockNameHUD::alpha() const {
    if (timeLeft <= 0.0f) return 0.0f;
    if (timeLeft >= FADE_SECONDS) return 1.0f;
    return timeLeft / FADE_SECONDS;
}
