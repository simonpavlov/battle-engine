#pragma once

#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Combat/CombatSystem.hpp>
#include <Core/Modules/Rng/RngSystem.hpp>
#include <Core/Modules/Vitals/HealthSystem.hpp>
#include <functional>
#include <utility>

namespace sw::feature {

class AttackAction : public core::IAction {
public:
    using PropertyFn = std::function<core::AttackProperty(core::UnitId)>;
    using FireFn = std::function<bool(core::UnitId)>;

    AttackAction(
        core::SystemsLocator& systems, core::AttackKind kind, PropertyFn get_attack_property, FireFn can_fire = {}
    ) :
            systems_(systems),
            kind_(kind),
            get_attack_property_(std::move(get_attack_property)),
            can_fire_(std::move(can_fire)) {}

    bool tryExecute(core::UnitId self_id) override {
        if (can_fire_ && !can_fire_(self_id)) {
            return false;
        }

        const core::AttackProperty property = get_attack_property_(self_id);
        if (property.damage.value <= 0) {
            return false;
        }
        auto& combat_system = systems_.getSystem<core::ICombatSystem>();
        std::vector<core::UnitId> targets = combat_system.selectTargets(self_id, kind_, property.band);
        if (targets.empty()) {
            return false;
        }

        const core::UnitId target = systems_.getSystem<core::IRngSystem>().pick(targets);
        systems_.getSystem<core::IHealthSystem>().applyDamage(self_id, target, property.damage.value);
        return true;
    }

private:
    core::SystemsLocator& systems_;
    core::AttackKind kind_;
    PropertyFn get_attack_property_;
    FireFn can_fire_;
};

}  // namespace sw::feature
