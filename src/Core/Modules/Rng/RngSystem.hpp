#pragma once

#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct IRngSystem : ISystem {
    virtual UnitId pick(const std::vector<UnitId>& candidates) = 0;
    virtual uint32_t randomInt(uint32_t lo, uint32_t hi) = 0;

    ~IRngSystem() override = default;
};

using IRngSystemPtr = std::unique_ptr<IRngSystem>;

IRngSystemPtr makeCoreRngSystem();

}  // namespace sw::core
