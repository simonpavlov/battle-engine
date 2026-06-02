#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <cstdint>

namespace sw::core {

struct Engine {
    ComponentsLocator components;
    SystemsLocator systems;

    std::uint64_t tick = 1;

    bool step() {
        auto& units = systems.getSystem<IUnitSystem>();
        if (units.aliveCount() <= 1) {
            return false;
        }
        ++tick;
        const bool acted = units.stepActions();
        units.sweep();
        return acted;
    }

    void run() {
        while (step()) {
        }
    }
};

}  // namespace sw::core
