#include "IconSet.hpp"

namespace sw::graphics {

namespace {

bool loadIfExists(const std::string& path, Texture2D& out) {
    if (!FileExists(path.c_str())) {
        return false;
    }
    out = LoadTexture(path.c_str());
    return out.id > 0;
}

}  // namespace

void IconSet::load(const std::string& baseDir) {
    const std::string dir = baseDir + "icons/";
    const bool ok = loadIfExists(dir + "load.png", loadIcon_) &&
                    loadIfExists(dir + "reload.png", reload_) &&
                    loadIfExists(dir + "play.png", play_) &&
                    loadIfExists(dir + "pause.png", pause_) &&
                    loadIfExists(dir + "step.png", step_) &&
                    loadIfExists(dir + "panel_units.png", panelUnits_) &&
                    loadIfExists(dir + "panel_properties.png", panelProperties_) &&
                    loadIfExists(dir + "panel_log.png", panelLog_);
    isLoaded_ = ok;
    if (!ok) {
        unload();
    }
}

void IconSet::unload() {
    UnloadTexture(loadIcon_);
    UnloadTexture(reload_);
    UnloadTexture(play_);
    UnloadTexture(pause_);
    UnloadTexture(step_);
    UnloadTexture(panelUnits_);
    UnloadTexture(panelProperties_);
    UnloadTexture(panelLog_);
    loadIcon_ = Texture2D{};
    reload_ = Texture2D{};
    play_ = Texture2D{};
    pause_ = Texture2D{};
    step_ = Texture2D{};
    panelUnits_ = Texture2D{};
    panelProperties_ = Texture2D{};
    panelLog_ = Texture2D{};
    isLoaded_ = false;
}

bool IconSet::loaded() const {
    return isLoaded_;
}

const Texture2D& IconSet::folder() const {
    return loadIcon_;
}

const Texture2D& IconSet::reload() const {
    return reload_;
}

const Texture2D& IconSet::play() const {
    return play_;
}

const Texture2D& IconSet::pause() const {
    return pause_;
}

const Texture2D& IconSet::step() const {
    return step_;
}

const Texture2D& IconSet::panelUnits() const {
    return panelUnits_;
}

const Texture2D& IconSet::panelProperties() const {
    return panelProperties_;
}

const Texture2D& IconSet::panelLog() const {
    return panelLog_;
}

}  // namespace sw::graphics
