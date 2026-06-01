#pragma once

#include <Core/Agility.hpp>
#include <Core/CollisionReaction.hpp>
#include <Core/Engine.hpp>
#include <Core/Health.hpp>
#include <Core/HealthSystem.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/Range.hpp>
#include <Core/RngSystem.hpp>
#include <Core/Strength.hpp>
#include <Core/Unit.hpp>
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
    // unit_type.actions.push_back(std::make_unique<>(engine));
    unit_type.actions.push_back(std::make_unique<MoveAction>(engine));

    // TODO: extract boilerplate to UnitType method: SetReaction<>(), assert inside
    const auto [it, inserted] = unit_type.reactions.emplace(
        std::type_index(typeid(core::ICollisionReaction)), std::make_unique<core::DefaultCollisionReaction>()
    );
    assert(inserted && "reaction already registered for this interface");

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
