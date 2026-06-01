#pragma once

#include <Core/StrongType.hpp>
#include <Core/Unit.hpp>
#include <cstdint>
#include <variant>

namespace sw::core {

// An attack kind is an open id: Core defines the two built-ins below, and a Feature may introduce
// more by registering them with the CombatSystem. Reactions interpret the kind (e.g. melee-immunity).
using AttackKind = StrongType<uint32_t>;
inline constexpr AttackKind kMeleeAttackKind{0};
inline constexpr AttackKind kRangedAttackKind{1};

using Distance = StrongType<uint32_t>;
using Damage = StrongType<int>;

struct DistanceBand {
    Distance min;
    Distance max;
};

// What an attack delivers from a given attacker: its reach and the damage it would inflict.
struct AttackProperty {
    DistanceBand band;
    Damage damage;
};

// This unit is not a legal target for the attack kind it was asked about.
struct Untargetable {};

// How a unit responds to being considered as a target: either the effective band after its own
// modifier (e.g. a flyer reduces a ranged attacker's reach against it), or Untargetable to opt out.
using TargetResponse = std::variant<DistanceBand, Untargetable>;

struct IOnTargetReaction : IReaction {
    virtual TargetResponse onTargeted(AttackKind kind, DistanceBand base) = 0;
};

}  // namespace sw::core
