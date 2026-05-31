#pragma once

#include <Core/Unit.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace sw::core {

struct IComponent {
    virtual void del(UnitId) = 0;

    virtual ~IComponent() = default;
};

template <class TData>
struct IComponentStore : IComponent {
    virtual void add(UnitId id, TData&& data) = 0;
    virtual bool has(UnitId id) = 0;
    virtual TData& get(UnitId id) = 0;
    virtual void forEach(const std::function<void(UnitId, TData&)>& fn) = 0;

    virtual ~IComponentStore() = default;
};

using IComponentPtr = std::unique_ptr<IComponent>;

namespace detail {

// Default component storage. Kept in `detail` so users never name a concrete store type; they only
// ever go through ComponentsLocator::registerComponent<T>() / getComponent<T>().
template <class TData>
struct DefaultComponentStore : IComponentStore<TData> {
    std::unordered_map<UnitId, TData> data;

    void add(UnitId id, TData&& value) override {
        data.insert_or_assign(id, std::move(value));
    }

    void del(UnitId id) override {
        data.erase(id);
    }

    bool has(UnitId id) override {
        return data.find(id) != data.end();
    }

    TData& get(UnitId id) override {
        return data.at(id);
    }

    void forEach(const std::function<void(UnitId, TData&)>& fn) override {
        for (auto& [id, value] : data) {
            fn(id, value);
        }
    }
};

}  // namespace detail

struct ComponentsLocator {
    std::unordered_map<std::type_index, IComponentPtr> components;

    template <class TData>
    void registerComponent(IComponentPtr&& component) {
        const auto [it, inserted] = components.emplace(std::type_index(typeid(TData)), std::move(component));
        assert(inserted && "component already registered for this data type");
    }

    // Register a component using the default storage. The concrete store type lives in `detail`, so
    // swapping it for another implementation is a one-line change for every component that uses this.
    template <class TData>
    void registerComponent() {
        registerComponent<TData>(std::make_unique<detail::DefaultComponentStore<TData>>());
    }

    template <class TData>
    IComponentStore<TData>& getComponent() {
        const auto it = components.find(std::type_index(typeid(TData)));
        assert(it != components.end() && "component not registered for this data type");
        return static_cast<IComponentStore<TData>&>(*it->second);
    }

    // Drop a unit from every registered store (used by end-of-tick death sweep).
    void removeUnitEverywhere(UnitId id) {
        for (auto& [type, component] : components) {
            component->del(id);
        }
    }
};

}
