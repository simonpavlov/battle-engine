#pragma once

#include <Core/Unit.hpp>

namespace sw::core {

struct ICollisionReaction : IReaction {
    virtual bool blocksMovement() = 0;
    virtual bool ignoresOccupants() = 0;
};

struct DefaultCollisionReaction : ICollisionReaction {
    bool blocksMovement() override {
        return true;
    }

    bool ignoresOccupants() override {
        return false;
    }
};

}  // namespace sw::core
