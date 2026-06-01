#pragma once

#include <Core/Components.hpp>
#include <Core/Position.hpp>
#include <Core/Signal.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sw::core {

using UnitTypes = std::unordered_map<UnitTypeId, UnitType>;
using UnitToType = std::unordered_map<UnitId, UnitTypeRef>;

struct Engine {
    Signal<UnitId, std::string_view, Position>::Sink onSpawned() {
        return spawned.sink();
    }

    Signal<UnitId>::Sink onDied() {
        return died.sink();
    }

    SystemsLocator systems;
    ComponentsLocator components;

    UnitToType unit_to_type;
    UnitTypes unit_types;
    std::vector<UnitId> creation_order;

    std::uint64_t tick = 1;

    std::vector<UnitId> pending_deaths;

    void scheduleDeath(UnitId id) {
        pending_deaths.push_back(id);
    }

    void registerUnitType(UnitType&& unit_type) {
        const auto [it, inserted] = unit_types.emplace(unit_type.id, std::move(unit_type));
        assert(inserted && "unit type already registered for this id");
    }

    // TODO: extract with 3 unit collections to separate class UnitStorage (challendge class name)
    void addUnit(UnitTypeId unit_type_id, UnitId unit_id) {
        auto& type = unit_types.at(unit_type_id);
        const auto [it, inserted] = unit_to_type.emplace(unit_id, std::ref(type));
        assert(inserted && "unit already exists");
        creation_order.push_back(unit_id);
        // Position is added before addUnit at every spawn site, so it is present here.
        spawned.emit(unit_id, type.name, components.getComponent<Position>().get(unit_id));
    }

    void delUnit(UnitId unit_id) {
        if (unit_to_type.find(unit_id) == unit_to_type.end()) {
            return;
        }
        died.emit(unit_id);
        unit_to_type.erase(unit_id);
        creation_order.erase(std::remove(creation_order.begin(), creation_order.end(), unit_id), creation_order.end());
        components.removeUnitEverywhere(unit_id);
    }

    void run() {
        while (creation_order.size() > 1) {
            ++tick;
            bool has_action = false;

            for (const UnitId unit_id : creation_order) {
                const auto it = unit_to_type.find(unit_id);
                if (it == unit_to_type.end()) {
                    continue;
                }
                for (const auto& action : it->second.get().actions) {
                    if (action->tryExecute(unit_id)) {
                        has_action = true;
                        break;
                    }
                }
            }

            for (const UnitId dead_id : pending_deaths) {
                delUnit(dead_id);
            }
            pending_deaths.clear();

            if (!has_action) {
                break;
            }
        }
    }

private:
    Signal<UnitId, std::string_view, Position> spawned;  // id, type name, position
    Signal<UnitId> died;                                 // id
};

}  // namespace sw::core
