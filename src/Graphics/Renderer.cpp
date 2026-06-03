#include "Renderer.hpp"

#include <Core/Modules/Spatial/Destination.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <algorithm>

#include "Picking.hpp"
#include "raylib.h"
#include "rlgl.h"

namespace sw::graphics {

namespace {

constexpr float kHpBarY = 1.2F;
constexpr float kHalf = 0.5F;
constexpr float kHighlightPadding = 0.1F;
constexpr int kCylinderSides = 16;

constexpr unsigned char kPlaneShade = 40;
constexpr unsigned char kBackShade = 180;
constexpr Color kPlaneColor{kPlaneShade, kPlaneShade, kPlaneShade, 255};
constexpr Color kHpBackColor{0, 0, 0, kBackShade};
constexpr Color kPositionBoxColor = GREEN;
constexpr Color kDestinationBoxColor = ORANGE;

constexpr float kProjectileRadius = 0.12F;
constexpr float kRingMaxRadius = 1.0F;
constexpr float kDamageScaleDivisor = 10.0F;
constexpr float kDamageScaleMin = 0.5F;
constexpr float kDamageScaleMax = 2.0F;
constexpr float kTracerAlpha = 0.4F;
constexpr float kImpactStart = 0.85F;
constexpr int kImpactRings = 6;
constexpr int kImpactSlices = 8;
constexpr Color kProjectileColor = GOLD;
constexpr Color kStrikeColor = GOLD;
constexpr Color kImpactColor = ORANGE;

constexpr Color kAttackerFlashColor = WHITE;
constexpr Color kTargetFlashColor = RED;
constexpr float kAttackerFlashStrength = 0.8F;
constexpr float kTargetFlashStrength = 0.85F;

float damageScale(int damage) {
    return std::clamp(static_cast<float>(damage) / kDamageScaleDivisor, kDamageScaleMin, kDamageScaleMax);
}

float effectProgress(double start) {
    return static_cast<float>((GetTime() - start) / EventFeed::kAttackEffectDuration);
}

float liftFor(const EventFeed& feed, sw::core::UnitId id) {
    const auto it = feed.roster().find(id);
    return it != feed.roster().end() ? unitLift(it->second.type) : 0.0F;
}

Vector3 lerp3(Vector3 a, Vector3 b, float t) {
    return Vector3{a.x + ((b.x - a.x) * t), a.y + ((b.y - a.y) * t), a.z + ((b.z - a.z) * t)};
}

Color blendColor(Color base, Color flash, float f) {
    f = std::clamp(f, 0.0F, 1.0F);
    const auto mix = [f](unsigned char a, unsigned char b) {
        return static_cast<unsigned char>(static_cast<float>(a) + ((static_cast<float>(b) - static_cast<float>(a)) * f));
    };
    return Color{mix(base.r, flash.r), mix(base.g, flash.g), mix(base.b, flash.b), base.a};
}

Color attackTint(const EventFeed& feed, sw::core::UnitId id, Color base) {
    float attackerFlash = 0.0F;
    float targetFlash = 0.0F;
    for (const auto& effect : feed.attacks()) {
        const float t = effectProgress(effect.start);
        if (t < 0.0F || t > 1.0F) {
            continue;
        }
        const float strength = 1.0F - t;
        if (effect.source == id) {
            attackerFlash = std::max(attackerFlash, strength);
        }
        if (effect.target == id) {
            targetFlash = std::max(targetFlash, strength);
        }
    }
    Color color = base;
    if (attackerFlash > 0.0F) {
        color = blendColor(color, kAttackerFlashColor, attackerFlash * kAttackerFlashStrength);
    }
    if (targetFlash > 0.0F) {
        color = blendColor(color, kTargetFlashColor, targetFlash * kTargetFlashStrength);
    }
    return color;
}

void drawImpact(Vector3 center, float radius, float alpha) {
    rlDisableDepthTest();
    DrawSphereWires(center, radius, kImpactRings, kImpactSlices, Fade(kImpactColor, alpha));
    rlEnableDepthTest();
}

void drawAttacks(const EventFeed& feed) {
    for (const auto& effect : feed.attacks()) {
        const float t = effectProgress(effect.start);
        if (t < 0.0F || t > 1.0F) {
            continue;
        }

        const float scale = damageScale(effect.damage);
        const Vector3 fromBase = cellToWorld(effect.from.x, effect.from.y);
        const Vector3 toBase = cellToWorld(effect.to.x, effect.to.y);
        const Vector3 from{fromBase.x, liftFor(feed, effect.source) + kUnitHalf, fromBase.z};
        const Vector3 to{toBase.x, liftFor(feed, effect.target) + kUnitHalf, toBase.z};

        if (effect.ranged) {
            const Vector3 projectile = lerp3(from, to, t);
            DrawLine3D(from, projectile, Fade(kStrikeColor, kTracerAlpha));
            DrawSphere(projectile, kProjectileRadius * scale, kProjectileColor);
            if (t > kImpactStart) {
                const float impact = (t - kImpactStart) / (1.0F - kImpactStart);
                drawImpact(to, kRingMaxRadius * scale * impact, 1.0F - impact);
            }
        } else {
            DrawLine3D(from, to, Fade(kStrikeColor, 1.0F - t));
            drawImpact(to, kRingMaxRadius * scale * t, 1.0F - t);
        }
    }
}

void drawCellBox(sw::core::components::Position cell, Color color) {
    const Vector3 base = cellToWorld(cell.x, cell.y);
    const Vector3 body{base.x, kUnitHalf, base.z};
    const float wire = kUnitSize + kHighlightPadding;
    DrawCubeWires(body, wire, wire, wire, color);
}

void drawDestinations(sw::core::Engine& engine, const EventFeed& feed) {
    auto& position = engine.systems.getSystem<sw::core::IPositionSystem>();
    auto& destinations = engine.components.getComponent<sw::core::components::Destination>();
    for (const auto& [id, entry] : feed.roster()) {
        (void)entry;
        if (!destinations.has(id) || !destinations.get(id).active) {
            continue;
        }
        const auto target = destinations.get(id).target;
        const Vector3 from = cellToWorld(position.getPosition(id).x, position.getPosition(id).y);
        const Vector3 to = cellToWorld(target.x, target.y);
        DrawLine3D(Vector3{from.x, kUnitHalf, from.z}, Vector3{to.x, kUnitHalf, to.z}, kDestinationBoxColor);
        drawCellBox(target, kDestinationBoxColor);
    }
}

void drawDragPreview(const VisualizerState& state, std::uint32_t width, std::uint32_t height) {
    if (!state.dragging) {
        return;
    }
    if (const auto cell = pickCell(state.camera, GetMousePosition(), width, height)) {
        drawCellBox(*cell, YELLOW);
    }
}

void drawPlacementPreview(const VisualizerState& state, std::uint32_t width, std::uint32_t height) {
    if (state.placement == Placement::None) {
        return;
    }
    const auto hovered = pickCell(state.camera, GetMousePosition(), width, height);
    if (state.placement == Placement::AddDestination) {
        if (state.pendingPosition) {
            drawCellBox(*state.pendingPosition, kPositionBoxColor);
        }
        if (hovered) {
            drawCellBox(*hovered, kDestinationBoxColor);
        }
        return;
    }
    if (hovered) {
        const Color color =
            state.placement == Placement::SetDestination ? kDestinationBoxColor : kPositionBoxColor;
        drawCellBox(*hovered, color);
    }
}

struct UnitVisual {
    Color color;
    bool cylinder;
    float lift;
};

UnitVisual visualFor(const std::string& type) {
    if (type == "Hunter") {
        return UnitVisual{DARKGREEN, true, 0.0F};
    }
    if (type == "Raven") {
        return UnitVisual{SKYBLUE, false, kRavenLift};
    }
    return UnitVisual{BROWN, false, 0.0F};
}

}  // namespace

void Renderer::draw3D(sw::core::Engine& engine, const EventFeed& feed, const VisualizerState& state) const {
    std::uint32_t width = feed.boardWidth();
    std::uint32_t height = feed.boardHeight();
    if (width == 0) {
        width = fallbackBoard_;
    }
    if (height == 0) {
        height = fallbackBoard_;
    }

    const auto fwidth = static_cast<float>(width);
    const auto fheight = static_cast<float>(height);
    const Vector3 center{fwidth * kHalf, 0.0F, fheight * kHalf};
    constexpr float kGridY = 0.01F;

    DrawPlane(center, Vector2{fwidth, fheight}, kPlaneColor);

    for (std::uint32_t i = 0; i <= width; ++i) {
        const auto fx = static_cast<float>(i);
        DrawLine3D(Vector3{fx, kGridY, 0.0F}, Vector3{fx, kGridY, fheight}, GRAY);
    }
    for (std::uint32_t j = 0; j <= height; ++j) {
        const auto fz = static_cast<float>(j);
        DrawLine3D(Vector3{0.0F, kGridY, fz}, Vector3{fwidth, kGridY, fz}, GRAY);
    }

    auto& position = engine.systems.getSystem<sw::core::IPositionSystem>();

    for (const auto& [id, entry] : feed.roster()) {
        const auto pos = position.getPosition(id);
        const UnitVisual visual = visualFor(entry.type);
        const Color color = attackTint(feed, id, visual.color);
        const Vector3 base = cellToWorld(pos.x, pos.y);
        const Vector3 body{base.x, kUnitHalf + visual.lift, base.z};

        if (visual.cylinder) {
            DrawCylinder(
                Vector3{base.x, visual.lift, base.z}, kUnitHalf, kUnitHalf, kUnitSize, kCylinderSides, color
            );
        } else {
            DrawCube(body, kUnitSize, kUnitSize, kUnitSize, color);
        }

        if (state.hasSelection && state.selected == id) {
            const float wire = kUnitSize + kHighlightPadding;
            DrawCubeWires(body, wire, wire, wire, YELLOW);
        }
    }

    drawDestinations(engine, feed);
    drawPlacementPreview(state, width, height);
    drawDragPreview(state, width, height);
    drawAttacks(feed);
}

void Renderer::drawOverlay(sw::core::Engine& engine, const EventFeed& feed, const VisualizerState& state) const {
    auto& position = engine.systems.getSystem<sw::core::IPositionSystem>();
    auto& healthStore = engine.components.getComponent<sw::core::components::Health>();

    for (const auto& [id, entry] : feed.roster()) {
        if (!healthStore.has(id)) {
            continue;
        }
        const auto& health = healthStore.get(id);
        if (health.max_hp.value <= 0) {
            continue;
        }

        const UnitVisual visual = visualFor(entry.type);
        const auto pos = position.getPosition(id);
        const Vector3 base = cellToWorld(pos.x, pos.y);
        const Vector3 above{base.x, kHpBarY + visual.lift, base.z};
        const Vector2 screen = GetWorldToScreen(above, state.camera);

        const float ratio = std::clamp(
            static_cast<float>(health.hp.value) / static_cast<float>(health.max_hp.value), 0.0F, 1.0F
        );

        const float left = screen.x - (hpBarWidth_ * kHalf);
        const float top = screen.y - (hpBarHeight_ * kHalf);

        DrawRectangle(
            static_cast<int>(left),
            static_cast<int>(top),
            static_cast<int>(hpBarWidth_),
            static_cast<int>(hpBarHeight_),
            kHpBackColor
        );
        DrawRectangle(
            static_cast<int>(left),
            static_cast<int>(top),
            static_cast<int>(hpBarWidth_ * ratio),
            static_cast<int>(hpBarHeight_),
            GREEN
        );
    }
}

}  // namespace sw::graphics
