#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstdint>
#include <variant>

namespace sw::core {

using AttackKind = StrongType<uint32_t, struct AttackKindTag>;
inline constexpr AttackKind kMeleeAttackKind{0};
inline constexpr AttackKind kRangedAttackKind{1};

using Damage = StrongType<std::int32_t, struct DamageTag>;

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
