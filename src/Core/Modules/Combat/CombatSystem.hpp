#pragma once

#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <memory>
#include <vector>

namespace sw::core {

struct Engine;

struct ICombatSystem : ISystem {
    virtual void registerAttackKind(AttackKind kind) = 0;

    virtual std::vector<UnitId> selectTargets(UnitId self, AttackKind kind, DistanceBand base) = 0;

    ~ICombatSystem() override = default;
};

using ICombatSystemPtr = std::unique_ptr<ICombatSystem>;

ICombatSystemPtr makeCoreCombatSystem(Engine& engine);

}  // namespace sw::core
