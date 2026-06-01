#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Foundation/Unit.hpp>

namespace sw::feature {

// TODO: move to core
struct MoveAction : core::IAction {
    // TODO: Action shouldn't reference to all core::Engine&, only systems
    explicit MoveAction(core::Engine& engine) :
            engine(engine) {}

    core::Engine& engine;

    bool tryExecute(core::UnitId self_id) override {
        return engine.systems.getSystem<core::IPositionSystem>().advanceMarch(self_id);
    }
};

}  // namespace sw::feature
