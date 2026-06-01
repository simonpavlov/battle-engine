#pragma once

#include <Core/Engine.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/Unit.hpp>

namespace sw::feature {

struct MoveAction : core::IAction {
    explicit MoveAction(core::Engine& engine) :
            engine(engine) {}

    core::Engine& engine;

    bool tryExecute(core::UnitId self_id) override {
        return engine.systems.getSystem<core::IPositionSystem>().advanceMarch(self_id);
    }
};

}  // namespace sw::feature
