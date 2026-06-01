#pragma once

#include <Core/CombatReaction.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <memory>
#include <vector>

namespace sw::core {

struct Engine;

struct ICombatSystem : ISystem {
    // Declare an attack kind so attacks of it are allowed. Asserts if the kind is already registered,
    // so two independent Features cannot silently collide on the same id.
    virtual void registerAttackKind(AttackKind kind) = 0;

    // Eligible target ids for an attack of `kind` whose base reach is `base`, excluding `self`.
    // A target must be hurtable (has Health) and, unless its IOnTargetReaction says otherwise, lie
    // within the band. `kind` must have been registered.
    virtual std::vector<UnitId> selectTargets(UnitId self, AttackKind kind, DistanceBand base) = 0;

    virtual ~ICombatSystem() = default;
};

using ICombatSystemPtr = std::unique_ptr<ICombatSystem>;

ICombatSystemPtr MakeCoreCombatSystem(Engine& engine);

}  // namespace sw::core
