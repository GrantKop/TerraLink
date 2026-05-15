#ifndef BLOCK_NAME_HUD_H
#define BLOCK_NAME_HUD_H

#include <string>

// HUD state for the "selected block" label.
//
// Plain logic only: there are no GL or windowing calls in this class.  It
// owns a timer that is reset whenever the selected block ID changes, and
// produces an alpha value used by the text renderer to fade the label out.
//
// Lifetime curve (TOTAL = 3s):
//   timeLeft in (1.0s, 3.0s] -> alpha = 1.0 (solid)
//   timeLeft in [0.0s, 1.0s] -> alpha = timeLeft / 1.0 (linear fade to 0)
class BlockNameHUD {
public:
    // Advances the timer.  If selectedID differs from the last seen ID, the
    // current name is replaced and the timer is reset to TOTAL_SECONDS.
    void update(float deltaTime, int selectedID, const std::string& name);

    // Returns true while the label still has any visible alpha.
    bool visible() const;

    // 0..1 alpha to multiply the text color by.
    float alpha() const;

    const std::string& name() const { return currentName; }

    static constexpr float TOTAL_SECONDS = 3.0f;
    static constexpr float FADE_SECONDS  = 1.0f;

private:
    int         lastSeenID = -1;
    std::string currentName;
    float       timeLeft = 0.0f;
};

#endif
