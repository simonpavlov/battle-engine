#pragma once

#include <Core/Components.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <algorithm>
#include <vector>

namespace sw::core {

using UnitTypes = std::unordered_map<UnitTypeId, UnitType>;
using UnitToType = std::unordered_map<UnitId, UnitTypeRef>;

struct Engine {
    SystemsLocator systems;
    ComponentsLocator components;

    UnitToType unit_to_type;
    UnitTypes unit_types;
    std::vector<UnitId> creation_order;

    // Units reported dead by systems this tick; Engine performs structural removal at end of tick.
    std::vector<UnitId> pending_deaths;

    void scheduleDeath(UnitId id) {
        pending_deaths.push_back(id);
    }

    void registerUnitType(UnitType&& unit_type) {
        unit_types.emplace(unit_type.id, std::move(unit_type));
    }

    // TODO: extract with 3 unit collections to separate class
    void addUnit(UnitTypeId unit_type_id, UnitId unit_id) {
        auto& type = unit_types.at(unit_type_id);
        unit_to_type.emplace(unit_id, std::ref(type));
        creation_order.push_back(unit_id);
    }

    void run() {
        while (creation_order.size() > 1) {
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
                unit_to_type.erase(dead_id);
                creation_order.erase(
                    std::remove(creation_order.begin(), creation_order.end(), dead_id), creation_order.end());
                components.removeUnitEverywhere(dead_id);
            }
            pending_deaths.clear();

            if (!has_action) {
                break;
            }
        }
    }
};

}
