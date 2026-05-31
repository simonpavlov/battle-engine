#pragma once

#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

// Randomness for action resolution (e.g. picking which adjacent unit to hit). Determinism is not
// required by the brief.
struct IRngSystem : ISystem {
    virtual UnitId pick(const std::vector<UnitId>& candidates) = 0;
    virtual uint32_t randomInt(uint32_t lo, uint32_t hi) = 0;

    virtual ~IRngSystem() = default;
};

using IRngSystemPtr = std::unique_ptr<IRngSystem>;

IRngSystemPtr MakeCoreRngSystem();

}
