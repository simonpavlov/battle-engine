#pragma once

#include <Core/CollisionReaction.hpp>
#include <Core/Engine.hpp>
#include <Core/Health.hpp>
#include <Core/HealthSystem.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/RngSystem.hpp>
#include <Core/Strength.hpp>
#include <Core/Unit.hpp>
#include <Features/MoveAction.hpp>
#include <cassert>
#include <memory>
#include <typeindex>
#include <vector>

namespace sw::feature {

// TODO: change UnitTypeId underlying type to string; probalbly better static string with string_view to optimize
// performance
static const auto kSwordsmanTypeId = core::UnitTypeId{.value = 11};

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
        health_system.applyDamage(self_id, target, static_cast<int>(strength));
        return true;
    }
};

inline core::UnitType MakeSwordsmanType(core::Engine& engine) {
    auto type = core::UnitType{
        .id = kSwordsmanTypeId,
        .name = "Swordsman",
    };
    type.actions.push_back(std::make_unique<AttackAction>(engine));
    type.actions.push_back(std::make_unique<MoveAction>(engine));
    const auto [it, inserted] = type.reactions.emplace(
        std::type_index(typeid(core::ICollisionReaction)), std::make_unique<core::DefaultCollisionReaction>()
    );
    assert(inserted && "reaction already registered for this interface");
    return type;
}

inline void spawnSwordsman(core::Engine& engine, core::UnitId id, core::Position pos, int hp, uint32_t strength) {
    engine.components.getComponent<core::Position>().add(id, std::move(pos));
    engine.components.getComponent<core::Health>().add(id, core::Health{hp});
    engine.components.getComponent<core::Strength>().add(id, core::Strength{strength});
    engine.addUnit(kSwordsmanTypeId, id);
}

}  // namespace sw::feature
