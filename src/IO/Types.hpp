#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Combat/CombatReaction.hpp>
#include <Core/Modules/Combat/Range.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <istream>
#include <ostream>

namespace sw::core {

template <class T, class Tag>
std::istream& operator>>(std::istream& is, StrongType<T, Tag>& v) {
    return is >> v.value;
}

template <class T, class Tag>
std::ostream& operator<<(std::ostream& os, const StrongType<T, Tag>& v) {
    return os << v.value;
}

}  // namespace sw::core
