#pragma once

#include <Core/StrongType.hpp>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace sw::core {

using UnitId = StrongType<std::uint32_t>;

struct IAction {
    virtual bool tryExecute(UnitId self_id) = 0;
    virtual ~IAction() = default;
};

using IActionPtr = std::unique_ptr<IAction>;

struct IReaction {
    virtual ~IReaction() = default;
};

using IReactionPtr = std::unique_ptr<IReaction>;

using UnitTypeId = StrongType<std::uint32_t>;

struct UnitType {
    UnitTypeId id = {.value = 0};
    std::vector<IActionPtr> actions;
    std::unordered_map<std::type_index, IReactionPtr> reactions;
};

using UnitTypeRef = std::reference_wrapper<const UnitType>;

}
