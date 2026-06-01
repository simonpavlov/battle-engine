#pragma once

#include <Core/Agility.hpp>
#include <Core/CollisionReaction.hpp>
#include <Core/CombatReaction.hpp>
#include <Core/Engine.hpp>
#include <Core/Health.hpp>
#include <Core/Position.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/Range.hpp>
#include <Core/Strength.hpp>
#include <Core/Unit.hpp>
#include <Features/AttackAction.hpp>
#include <Features/MoveAction.hpp>
#include <cstdint>
#include <memory>

namespace sw::feature {

static const auto kHunterTypeId = core::UnitTypeId{333};

inline core::UnitType makeHunterType(core::Engine& engine) {
    auto unit_type = core::UnitType{
        .id = kHunterTypeId,
        .name = "Hunter",
    };

    // Rapid Shot: ranged 2..Range for Agility, but only when no unit stands in an adjacent cell.
    unit_type.actions.push_back(
        std::make_unique<AttackAction>(
            engine,
            core::kRangedAttackKind,
            [&engine](core::UnitId self) {
                const auto range = engine.components.getComponent<core::Range>().get(self).range;
                const auto agility = engine.components.getComponent<core::Agility>().get(self).value;
                return core::AttackProperty{
                    .band = {.min = core::Distance{2}, .max = core::Distance{range}},
                    .damage = core::Damage{static_cast<int>(agility)},
                };
            },
            [&engine](core::UnitId self) {
                auto& position_system = engine.systems.getSystem<core::IPositionSystem>();
                return position_system.unitsInRange(position_system.getPosition(self), 1, 1, self).empty();
            }
        )
    );

    // Shadow Strike: melee fallback for Strength.
    unit_type.actions.push_back(
        std::make_unique<AttackAction>(engine, core::kMeleeAttackKind, [&engine](core::UnitId self) {
            const auto strength = engine.components.getComponent<core::Strength>().get(self).value;
            return core::AttackProperty{
                .band = {.min = core::Distance{1}, .max = core::Distance{1}},
                .damage = core::Damage{static_cast<int>(strength)},
            };
        })
    );

    unit_type.actions.push_back(std::make_unique<MoveAction>(engine));
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<core::DefaultCollisionReaction>());
    return unit_type;
}

inline void spawnHunter(
    core::Engine& engine,
    core::UnitId id,
    core::Position pos,
    int hp,
    uint32_t strength,
    uint32_t agility,
    uint32_t range
) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Health>().add(id, core::Health{hp});
    engine.components.getComponent<core::Strength>().add(id, core::Strength{strength});
    engine.components.getComponent<core::Agility>().add(id, core::Agility{agility});
    engine.components.getComponent<core::Range>().add(id, core::Range{range});
    engine.addUnit(kHunterTypeId, id);
}

}  // namespace sw::feature
