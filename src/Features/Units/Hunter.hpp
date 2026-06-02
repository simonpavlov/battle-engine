#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Combat/Range.hpp>
#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Features/Actions/AttackAction.hpp>
#include <Features/Actions/MoveAction.hpp>
#include <memory>

namespace sw::feature {

inline const core::UnitTypeId& hunterTypeId() {
    static const core::UnitTypeId id{"Hunter"};
    return id;
}

inline core::UnitType makeHunterType(core::ComponentsLocator& components, core::SystemsLocator& systems) {
    auto unit_type = core::UnitType{
        .id = hunterTypeId(),
        .actions = {},
        .reactions = {},
    };

    // Rapid Shot: ranged 2..Range for Agility, but only when no unit stands in an adjacent cell.
    unit_type.addAction<AttackAction>(
        systems,
        core::kRangedAttackKind,
        [&components](core::UnitId self) {
            const auto range = components.getComponent<core::components::Range>().get(self).value;
            const auto agility = components.getComponent<core::components::Agility>().get(self).value;
            return core::AttackProperty{
                .band = {.min = core::Distance{2}, .max = core::Distance{range}},
                .damage = core::Damage{agility},
            };
        },
        [&systems](core::UnitId self) {
            auto& position_system = systems.getSystem<core::IPositionSystem>();
            return position_system
                .unitsInRange(position_system.getPosition(self), core::Distance{1}, core::Distance{1}, self)
                .empty();
        }
    );

    // Shadow Strike: melee fallback for Strength.
    unit_type.addAction<AttackAction>(systems, core::kMeleeAttackKind, [&components](core::UnitId self) {
        const auto strength = components.getComponent<core::components::Strength>().get(self).value;
        return core::AttackProperty{
            .band = {.min = core::Distance{1}, .max = core::Distance{1}},
            .damage = core::Damage{strength},
        };
    });

    unit_type.addAction<MoveAction>(systems);
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<core::DefaultCollisionReaction>());
    return unit_type;
}

namespace commands {

struct SpawnHunter {
    static constexpr const char* name = "SPAWN_HUNTER";

    core::UnitId unitId{};
    core::components::Position pos{};
    core::components::HealthPoints hp{};
    core::components::Agility agility{};
    core::components::Strength strength{};
    core::components::Range range{};

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit("unitId", unitId);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
        visitor.visit("hp", hp);
        visitor.visit("agility", agility);
        visitor.visit("strength", strength);
        visitor.visit("range", range);
    }
};

}  // namespace commands

inline void spawnHunter(core::ComponentsLocator& components, core::SystemsLocator& systems, commands::SpawnHunter cmd) {
    components.getComponent<core::components::Position>().add(cmd.unitId, std::move(cmd.pos));
    components.getComponent<core::components::Health>().add(cmd.unitId, core::components::Health{cmd.hp, cmd.hp});
    components.getComponent<core::components::Strength>().add(cmd.unitId, std::move(cmd.strength));
    components.getComponent<core::components::Agility>().add(cmd.unitId, std::move(cmd.agility));
    components.getComponent<core::components::Range>().add(cmd.unitId, std::move(cmd.range));
    systems.getSystem<core::IUnitSystem>().addUnit(hunterTypeId(), cmd.unitId);
}

}  // namespace sw::feature
