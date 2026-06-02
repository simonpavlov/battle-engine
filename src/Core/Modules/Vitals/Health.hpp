#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <cstdint>

namespace sw::core::components {

using HealthPoints = StrongType<std::int32_t, struct HealthPointsTag>;

struct Health {
    HealthPoints hp;
    HealthPoints max_hp;
};

}  // namespace sw::core::components
