#pragma once

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace sw::core {

struct ISystem {
    virtual ~ISystem() = default;
};

using ISystemPtr = std::unique_ptr<ISystem>;

struct SystemsLocator {
    std::unordered_map<std::type_index, ISystemPtr> systems;

    template <class TSystemIFace>
    void registerSystem(ISystemPtr&& system) {
        const auto [it, inserted] = systems.emplace(std::type_index(typeid(TSystemIFace)), std::move(system));
        assert(inserted && "system already registered for this interface");
    }

    template <class TSystemIFace>
    TSystemIFace& getSystem() const {
        const auto it = systems.find(std::type_index(typeid(TSystemIFace)));
        assert(it != systems.end() && "system not registered for this interface");
        return static_cast<TSystemIFace&>(*it->second);
    }
};

}
