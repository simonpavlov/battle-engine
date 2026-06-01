#pragma once

#include <Core/Modules/Spatial/CollisionReaction.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Features/Actions/AttackAction.hpp>
#include <Features/Actions/MoveAction.hpp>
#include <cstdint>
#include <memory>

namespace sw::feature {

// TODO: change UnitTypeId underlying type to string; probalbly better static string with string_view to optimize
// performance
static const auto kSwordsmanTypeId = core::UnitTypeId{.value = 11};

inline core::UnitType makeSwordsmanType(core::Engine& engine) {
    auto unit_type = core::UnitType{
        .id = kSwordsmanTypeId,
        .name = "Swordsman",
        .actions = {},
        .reactions = {},
    };
    // Melee Strike: adjacent attack for Strength.
    unit_type.actions.push_back(std::make_unique<AttackAction>(engine, core::kMeleeAttackKind, [&engine](core::UnitId self) {
        const auto strength = engine.components.getComponent<core::Strength>().get(self).value;
        return core::AttackProperty{
            .band = {.min = core::Distance{1}, .max = core::Distance{1}},
            .damage = core::Damage{static_cast<int>(strength)},
        };
    }));
    unit_type.actions.push_back(std::make_unique<MoveAction>(engine));
    unit_type.setReaction<core::ICollisionReaction>(std::make_unique<core::DefaultCollisionReaction>());
    return unit_type;
}

inline void spawnSwordsman(core::Engine& engine, core::UnitId id, core::Position pos, int hp, uint32_t strength) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Health>().add(id, core::Health{hp});
    engine.components.getComponent<core::Strength>().add(id, core::Strength{strength});
    engine.addUnit(kSwordsmanTypeId, id);
}

}  // namespace sw::feature
