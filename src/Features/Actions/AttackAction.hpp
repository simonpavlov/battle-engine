#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Combat/CombatSystem.hpp>
#include <Core/Modules/Rng/RngSystem.hpp>
#include <Core/Modules/Vitals/HealthSystem.hpp>
#include <functional>
#include <utility>

namespace sw::feature {

// TODO: move to core
struct AttackAction : core::IAction {
    using PropertyFn = std::function<core::AttackProperty(core::UnitId)>;
    using FireFn = std::function<bool(core::UnitId)>;

    AttackAction(core::Engine& engine, core::AttackKind kind, PropertyFn get_attack_property, FireFn can_fire = {}) :
            engine(engine),
            kind(kind),
            get_attack_property(std::move(get_attack_property)),
            can_fire(std::move(can_fire)) {}

    core::Engine& engine;
    core::AttackKind kind;
    PropertyFn get_attack_property;
    FireFn can_fire;

    bool tryExecute(core::UnitId self_id) override {
        if (can_fire && !can_fire(self_id)) {
            return false;
        }

        const core::AttackProperty property = get_attack_property(self_id);
        if (property.damage.value <= 0) {
            return false;
        }
        auto& combat_system = engine.systems.getSystem<core::ICombatSystem>();
        std::vector<core::UnitId> targets = combat_system.selectTargets(self_id, kind, property.band);
        if (targets.empty()) {
            return false;
        }

        const core::UnitId target = engine.systems.getSystem<core::IRngSystem>().pick(targets);
        engine.systems.getSystem<core::IHealthSystem>().applyDamage(self_id, target, property.damage.value);
        return true;
    }
};

}  // namespace sw::feature
