#include "UnitSystem.hpp"

#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace sw::core {

namespace {

struct CoreUnitSystem : IUnitSystem {
    explicit CoreUnitSystem(Engine& engine) :
            engine(engine) {}

    Engine& engine;

    void registerUnitType(UnitType&& unit_type) override {
        const auto [it, inserted] = unit_types.emplace(unit_type.id, std::move(unit_type));
        (void)it;
        (void)inserted;
        assert(inserted && "unit type already registered for this id");
    }

    void addUnit(UnitTypeId unit_type_id, UnitId unit_id) override {
        auto& type = unit_types.at(unit_type_id);
        const auto [it, inserted] = unit_to_type.emplace(unit_id, std::ref(type));
        (void)it;
        (void)inserted;
        assert(inserted && "unit already exists");
        creation_order.push_back(unit_id);
        spawned.emit({unit_id, type.id.value, engine.components.getComponent<components::Position>().get(unit_id)});
    }

    void scheduleDeath(UnitId unit_id) override {
        pending_deaths.push_back(unit_id);
    }

    void sweep() override {
        for (const UnitId unit_id : pending_deaths) {
            delUnit(unit_id);
        }
        pending_deaths.clear();
    }

    bool stepActions() override {
        bool acted = false;
        for (const UnitId unit_id : creation_order) {
            const auto it = unit_to_type.find(unit_id);
            if (it == unit_to_type.end()) {
                continue;
            }
            for (const auto& action : it->second.get().actions) {
                if (action->tryExecute(unit_id)) {
                    acted = true;
                    break;
                }
            }
        }
        return acted;
    }

    std::size_t aliveCount() const override {
        return creation_order.size();
    }

    const UnitType& getUnitType(UnitId unit_id) const override {
        const auto it = unit_to_type.find(unit_id);
        assert(it != unit_to_type.end() && "not found unit type");
        return it->second.get();
    }

    ~CoreUnitSystem() override = default;

private:
    void delUnit(UnitId unit_id) {
        if (!unit_to_type.contains(unit_id)) {
            return;
        }
        died.emit({unit_id});
        unit_to_type.erase(unit_id);
        std::erase(creation_order, unit_id);
        engine.components.removeUnitEverywhere(unit_id);
    }

    UnitTypes unit_types;
    UnitToType unit_to_type;
    std::vector<UnitId> creation_order;
    std::vector<UnitId> pending_deaths;
};

}  // namespace

IUnitSystemPtr makeCoreUnitSystem(Engine& engine) {
    return std::make_unique<CoreUnitSystem>(engine);
}

}  // namespace sw::core
