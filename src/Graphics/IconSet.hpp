#pragma once

#include <string>

#include "raylib.h"

namespace sw::graphics {

class IconSet {
public:
    void load(const std::string& baseDir);
    void unload();
    bool loaded() const;

    const Texture2D& folder() const;
    const Texture2D& reload() const;
    const Texture2D& play() const;
    const Texture2D& pause() const;
    const Texture2D& step() const;
    const Texture2D& panelUnits() const;
    const Texture2D& panelProperties() const;
    const Texture2D& panelLog() const;

private:
    Texture2D loadIcon_{};
    Texture2D reload_{};
    Texture2D play_{};
    Texture2D pause_{};
    Texture2D step_{};
    Texture2D panelUnits_{};
    Texture2D panelProperties_{};
    Texture2D panelLog_{};
    bool isLoaded_ = false;
};

}  // namespace sw::graphics
