#pragma once

#include <Core/Foundation/Engine.hpp>
#include <cstdint>
#include <string>

#include "EventFeed.hpp"
#include "VisualizerState.hpp"
#include "raylib.h"

namespace sw::graphics {

inline constexpr float kCellCenter = 0.5F;
inline constexpr float kUnitSize = 0.8F;
inline constexpr float kUnitHalf = kUnitSize * kCellCenter;
inline constexpr float kRavenLift = 1.0F;
inline constexpr std::uint32_t kFallbackBoardSize = 10;
inline constexpr float kHpBarWidth = 40.0F;
inline constexpr float kHpBarHeight = 6.0F;

inline Vector3 cellToWorld(std::uint32_t x, std::uint32_t y) {
    return Vector3{static_cast<float>(x) + kCellCenter, 0.0F, static_cast<float>(y) + kCellCenter};
}

inline float unitLift(const std::string& type) {
    return type == "Raven" ? kRavenLift : 0.0F;
}

class Renderer {
public:
    void draw3D(sw::core::Engine& engine, const EventFeed& feed, const VisualizerState& state) const;
    void drawOverlay(sw::core::Engine& engine, const EventFeed& feed, const VisualizerState& state) const;

private:
    std::uint32_t fallbackBoard_ = kFallbackBoardSize;
    float hpBarWidth_ = kHpBarWidth;
    float hpBarHeight_ = kHpBarHeight;
};

}  // namespace sw::graphics
