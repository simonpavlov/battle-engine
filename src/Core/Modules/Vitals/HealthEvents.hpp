#pragma once

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Vitals/Health.hpp>

namespace sw::core::events {

struct Attacked {
    static constexpr const char* name = "UNIT_ATTACKED";

    UnitId source;
    UnitId target;
    Damage damage;
    components::HealthPoints target_hp;

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit("attackerUnitId", source);
        visitor.visit("targetUnitId", target);
        visitor.visit("damage", damage);
        visitor.visit("targetHp", target_hp);
    }
};

}  // namespace sw::core::events
