#pragma once

#include <Core/Foundation/Unit.hpp>

namespace sw::core {

struct ICollisionReaction : IReaction {
    virtual bool blocksMovement() = 0;
    virtual bool ignoresOccupants() = 0;
};

class DefaultCollisionReaction : public ICollisionReaction {
public:
    bool blocksMovement() override {
        return true;
    }

    bool ignoresOccupants() override {
        return false;
    }
};

}  // namespace sw::core
