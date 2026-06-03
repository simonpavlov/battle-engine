#include "Picking.hpp"

#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <cmath>

#include "Renderer.hpp"

namespace sw::graphics {

namespace {

constexpr float kRayParallelEpsilon = 1e-6F;

}  // namespace

std::optional<sw::core::UnitId> pickUnit(
    sw::core::Engine& engine, const EventFeed& feed, const Camera3D& camera, Vector2 mouse
) {
    const Ray ray = GetScreenToWorldRay(mouse, camera);
    auto& position = engine.systems.getSystem<sw::core::IPositionSystem>();

    std::optional<sw::core::UnitId> best;
    float bestDistance = 0.0F;

    for (const auto& [id, entry] : feed.roster()) {
        const auto pos = position.getPosition(id);
        const Vector3 c = cellToWorld(pos.x, pos.y);
        const float lift = unitLift(entry.type);
        const BoundingBox box{
            Vector3{c.x - kUnitHalf, lift, c.z - kUnitHalf},
            Vector3{c.x + kUnitHalf, lift + kUnitSize, c.z + kUnitHalf},
        };

        const RayCollision hit = GetRayCollisionBox(ray, box);
        if (hit.hit && (!best || hit.distance < bestDistance)) {
            best = id;
            bestDistance = hit.distance;
        }
    }

    return best;
}

std::optional<sw::core::components::Position> pickCell(
    const Camera3D& camera, Vector2 mouse, std::uint32_t boardWidth, std::uint32_t boardHeight
) {
    const Ray ray = GetScreenToWorldRay(mouse, camera);
    if (std::fabs(ray.direction.y) < kRayParallelEpsilon) {
        return std::nullopt;
    }

    const float distance = -ray.position.y / ray.direction.y;
    if (distance <= 0.0F) {
        return std::nullopt;
    }

    const float worldX = ray.position.x + (ray.direction.x * distance);
    const float worldZ = ray.position.z + (ray.direction.z * distance);
    if (worldX < 0.0F || worldZ < 0.0F) {
        return std::nullopt;
    }

    const auto cellX = static_cast<std::uint32_t>(worldX);
    const auto cellY = static_cast<std::uint32_t>(worldZ);
    if (cellX >= boardWidth || cellY >= boardHeight) {
        return std::nullopt;
    }

    return sw::core::components::Position{cellX, cellY};
}

}  // namespace sw::graphics
