#pragma once

#include <Core/Unit.hpp>

namespace sw::core {

enum class CollideReaction {
    Ignore,
    RestrictMove,
};

struct ICollisionReaction : IReaction {
    virtual CollideReaction OnCollide() = 0;
};

}  // namespace sw::core
