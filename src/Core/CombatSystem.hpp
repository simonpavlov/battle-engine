#pragma once

#include <Core/CombatReaction.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <memory>
#include <vector>

namespace sw::core {

struct Engine;

struct ICombatSystem : ISystem {
    virtual void registerAttackKind(AttackKind kind) = 0;

    virtual std::vector<UnitId> selectTargets(UnitId self, AttackKind kind, DistanceBand base) = 0;

    virtual ~ICombatSystem() = default;
};

using ICombatSystemPtr = std::unique_ptr<ICombatSystem>;

ICombatSystemPtr makeCoreCombatSystem(Engine& engine);

}  // namespace sw::core
