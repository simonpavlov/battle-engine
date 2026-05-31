#pragma once

#include <Core/CollisionReaction.hpp>
#include <Core/Destination.hpp>
#include <Core/Engine.hpp>
#include <Core/Health.hpp>
#include <Core/HealthSystem.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/RngSystem.hpp>
#include <Core/Strength.hpp>
#include <Core/Unit.hpp>
#include <memory>
#include <typeindex>
#include <vector>

namespace sw::feature {

static const auto kSwordsmanTypeId = core::UnitTypeId{.value = 11};

// "Occupies a cell": others may not move onto this unit's tile.
struct RestrictMoveReaction : core::ICollisionReaction {
    core::CollideReaction OnCollide() override {
        return core::CollideReaction::RestrictMove;
    }
};

// Crushing blow: hit one random unit in an adjacent cell for `Strength` damage.
struct AttackAction : core::IAction {
    explicit AttackAction(core::Engine& engine) :
            engine(engine) {}

    core::Engine& engine;

    bool tryExecute(core::UnitId self_id) override {
        auto& position_system = engine.systems.getSystem<core::IPositionSystem>();
        auto& health_system = engine.systems.getSystem<core::IHealthSystem>();
        auto& rng_system = engine.systems.getSystem<core::IRngSystem>();

        const core::Position self_position = position_system.getPosition(self_id);

        std::vector<core::UnitId> targets;
        for (const core::UnitId candidate : position_system.unitsInRange(self_position, 1, 1, self_id)) {
            if (health_system.has(candidate)) {
                targets.push_back(candidate);
            }
        }
        if (targets.empty()) {
            return false;
        }

        const core::UnitId target = rng_system.pick(targets);
        const uint32_t strength = engine.components.getComponent<core::Strength>().get(self_id).value;
        health_system.applyDamage(target, static_cast<int>(strength));
        return true;
    }
};

// Step one cell (8-directional) toward the MARCH destination, if any.
struct MoveAction : core::IAction {
    explicit MoveAction(core::Engine& engine) :
            engine(engine) {}

    core::Engine& engine;

    static uint32_t stepToward(uint32_t from, uint32_t to) {
        if (to > from) {
            return from + 1;
        }
        if (to < from) {
            return from - 1;
        }
        return from;
    }

    bool tryExecute(core::UnitId self_id) override {
        auto& destinations = engine.components.getComponent<core::Destination>();
        if (!destinations.has(self_id)) {
            return false;
        }
        core::Destination& destination = destinations.get(self_id);
        if (!destination.active) {
            return false;
        }

        auto& position_system = engine.systems.getSystem<core::IPositionSystem>();
        const core::Position current = position_system.getPosition(self_id);
        if (current == destination.target) {
            destination.active = false;
            return false;
        }

        const core::Position next{
            stepToward(current.x, destination.target.x),
            stepToward(current.y, destination.target.y),
        };
        const bool moved = position_system.move(self_id, next);
        if (moved && position_system.getPosition(self_id) == destination.target) {
            destination.active = false;
        }
        return moved;
    }
};

inline core::UnitType MakeSwordsmanType(core::Engine& engine) {
    auto type = core::UnitType{
        .id = kSwordsmanTypeId,
    };
    // "Crushing blow if you can, else move" falls out of action order.
    type.actions.push_back(std::make_unique<AttackAction>(engine));
    type.actions.push_back(std::make_unique<MoveAction>(engine));
    type.reactions.emplace(std::type_index(typeid(core::ICollisionReaction)),
                           std::make_unique<RestrictMoveReaction>());
    return type;
}

inline void spawnSwordsman(core::Engine& engine, core::UnitId id, core::Position pos, int hp,
                           uint32_t strength) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Health>().add(id, core::Health{hp});
    engine.components.getComponent<core::Strength>().add(id, core::Strength{strength});
    engine.addUnit(kSwordsmanTypeId, id);
}

}
