#pragma once

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

#include "raylib.h"

namespace sw::graphics {

inline constexpr float kCameraDistance = 10.0F;
inline constexpr float kCameraFovy = 45.0F;
inline constexpr float kDefaultSpeed = 2.0F;
inline constexpr float kFrameDefaultExtent = 10.0F;
inline constexpr float kFrameDistanceFactor = 1.1F;
inline constexpr float kFrameHeightFactor = 1.25F;
inline constexpr float kFramePanFactor = 0.18F;
inline constexpr float kFrameHalf = 0.5F;

struct AddUnitForm {
    static constexpr int kDefaultHp = 10;
    static constexpr int kDefaultStrength = 5;
    static constexpr int kDefaultAgility = 5;
    static constexpr int kDefaultRange = 3;

    int typeIndex = 0;
    int hp = kDefaultHp;
    int strength = kDefaultStrength;
    int agility = kDefaultAgility;
    int range = kDefaultRange;
};

enum class Placement : std::uint8_t {
    None,
    AddPosition,
    AddDestination,
    SetDestination,
};

struct VisualizerState {
    Camera3D camera{
        .position = Vector3{kCameraDistance, kCameraDistance, kCameraDistance},
        .target = Vector3{0.0F, 0.0F, 0.0F},
        .up = Vector3{0.0F, 1.0F, 0.0F},
        .fovy = kCameraFovy,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool showUnits = true;
    bool showProperties = true;
    bool showLog = true;

    float speed = kDefaultSpeed;
    bool playing = false;
    bool stepRequested = false;

    bool loadRequested = false;
    std::string loadPath;

    std::string capturePath;

    sw::core::UnitId selected{};
    bool hasSelection = false;

    Placement placement = Placement::None;
    std::optional<sw::core::components::Position> pendingPosition;

    bool dragging = false;
    sw::core::UnitId dragId{};
    Vector2 dragStart{};

    std::uint64_t nextId = 1;
    float uiScale = 1.0F;
    bool uiScaleFromEnv = false;

    AddUnitForm addForm;
};

inline void frameCamera(VisualizerState& state, std::uint32_t width, std::uint32_t height) {
    const float w = width > 0 ? static_cast<float>(width) : kFrameDefaultExtent;
    const float h = height > 0 ? static_cast<float>(height) : kFrameDefaultExtent;
    const float extent = std::max(w, h);
    const float d = extent * kFrameDistanceFactor;
    const float pan = extent * kFramePanFactor;
    state.camera.target = Vector3{(w * kFrameHalf) + pan, 0.0F, (h * kFrameHalf) + pan};
    state.camera.position =
        Vector3{state.camera.target.x + d, d * kFrameHeightFactor, state.camera.target.z + d};
}

}  // namespace sw::graphics
