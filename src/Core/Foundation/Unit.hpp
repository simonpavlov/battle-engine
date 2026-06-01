#pragma once

#include <Core/Foundation/StrongType.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace sw::core {

// TODO: rename Unit to Entity
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

// TODO: rename Unit to Entity
struct UnitType {
    UnitTypeId id = {.value = 0};
    std::string name;
    std::vector<IActionPtr> actions;
    std::unordered_map<std::type_index, IReactionPtr> reactions;

    // TODO: add helper addAction

    template <typename TInterface>
    void setReaction(IReactionPtr reaction) {
        const auto [it, inserted] = reactions.emplace(std::type_index(typeid(TInterface)), std::move(reaction));
        (void)it;
        (void)inserted;
        assert(inserted && "reaction already registered for this interface");
    }

    template <typename T>
    std::optional<std::reference_wrapper<T>> findReaction() const {
        const auto it = reactions.find(std::type_index(typeid(T)));
        if (it == reactions.end()) {
            return std::nullopt;
        }
        return std::ref(static_cast<T&>(*it->second));
    }
};

using UnitTypeRef = std::reference_wrapper<const UnitType>;

}  // namespace sw::core
