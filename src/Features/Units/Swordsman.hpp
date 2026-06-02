#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Features/Actions/AttackAction.hpp>
#include <Features/Actions/MoveAction.hpp>
#include <memory>

namespace sw::feature {

inline const core::UnitTypeId& swordsmanTypeId() {
    static const core::UnitTypeId id{"Swordsman"};
    return id;
}

inline core::UnitType makeSwordsmanType(core::ComponentsLocator& components, core::SystemsLocator& systems) {
    auto unit_type = core::UnitType{
        .id = swordsmanTypeId(),
        .actions = {},
        .reactions = {},
    };
    // Melee Strike: adjacent attack for Strength.
    unit_type.addAction<AttackAction>(systems, core::kMeleeAttackKind, [&components](core::UnitId self) {
        const auto strength = components.getComponent<core::components::Strength>().get(self);
        return core::AttackProperty{
            .band = {.min = core::Distance{1}, .max = core::Distance{1}},
            .damage = core::Damage{strength.value},
        };
    });
    unit_type.addAction<MoveAction>(systems);
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<core::DefaultCollisionReaction>());
    return unit_type;
}

namespace commands {

struct SpawnSwordsman {
    static constexpr const char* name = "SPAWN_SWORDSMAN";

    core::UnitId unitId{};
    core::components::Position pos{};
    core::components::HealthPoints hp{};
    core::components::Strength strength{};

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit("unitId", unitId);
        visitor.visit("x", pos.x);
        visitor.visit("y", pos.y);
        visitor.visit("hp", hp);
        visitor.visit("strength", strength);
    }
};

}  // namespace commands

inline void spawnSwordsman(
    core::ComponentsLocator& components, core::SystemsLocator& systems, commands::SpawnSwordsman cmd
) {
    components.getComponent<core::components::Position>().add(cmd.unitId, std::move(cmd.pos));
    components.getComponent<core::components::Health>().add(cmd.unitId, core::components::Health{cmd.hp, cmd.hp});
    components.getComponent<core::components::Strength>().add(cmd.unitId, std::move(cmd.strength));
    systems.getSystem<core::IUnitSystem>().addUnit(swordsmanTypeId(), cmd.unitId);
}

}  // namespace sw::feature
