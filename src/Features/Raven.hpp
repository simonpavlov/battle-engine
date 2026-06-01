#pragma once

#include <Core/Agility.hpp>
#include <Core/CollisionReaction.hpp>
#include <Core/Engine.hpp>
#include <Core/Position.hpp>
#include <Core/Speed.hpp>
#include <Core/Unit.hpp>
#include <Features/MoveAction.hpp>
#include <cassert>
#include <memory>
#include <typeindex>

namespace sw::feature {

static const auto kRavenTypeId = core::UnitTypeId{.value = 13};

struct FlyingReaction : core::ICollisionReaction {
    bool blocksMovement() override {
        return false;
    }

    bool ignoresOccupants() override {
        return true;
    }
};

inline core::UnitType MakeRavenType(core::Engine& engine) {
    auto type = core::UnitType{
        .id = kRavenTypeId,
        .name = "Raven",
    };
    type.actions.push_back(std::make_unique<MoveAction>(engine));
    const auto [it, inserted]
        = type.reactions.emplace(std::type_index(typeid(core::ICollisionReaction)), std::make_unique<FlyingReaction>());
    assert(inserted && "reaction already registered for this interface");
    return type;
}

inline void spawnRaven(core::Engine& engine, core::UnitId id, core::Position pos, uint32_t agility) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Speed>().add(id, core::Speed{2});
    engine.components.getComponent<core::Agility>().add(id, core::Agility{agility});
    engine.addUnit(kRavenTypeId, id);
}

}  // namespace sw::feature
