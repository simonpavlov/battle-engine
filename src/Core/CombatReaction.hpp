#pragma once

#include <Core/StrongType.hpp>
#include <Core/Unit.hpp>
#include <cstdint>
#include <variant>

namespace sw::core {

using AttackKind = StrongType<uint32_t>;
inline constexpr AttackKind kMeleeAttackKind{0};
inline constexpr AttackKind kRangedAttackKind{1};

using Distance = StrongType<uint32_t>;
using Damage = StrongType<int>;

struct DistanceBand {
    Distance min;
    Distance max;
};

struct AttackProperty {
    DistanceBand band;
    Damage damage;
};

struct Untargetable {};

using TargetResponse = std::variant<DistanceBand, Untargetable>;

struct IOnTargetReaction : IReaction {
    virtual TargetResponse onTargeted(AttackKind kind, DistanceBand base) = 0;
};

}  // namespace sw::core
