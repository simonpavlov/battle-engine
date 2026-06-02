#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <cstdint>

namespace sw::core::components {

using Strength = StrongType<std::int32_t, struct StrengthTag>;

}  // namespace sw::core::components
