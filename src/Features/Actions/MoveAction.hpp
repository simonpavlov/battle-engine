#pragma once

#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>

namespace sw::feature {

class MoveAction : public core::IAction {
public:
    explicit MoveAction(core::SystemsLocator& systems) :
            systems_(systems) {}

    bool tryExecute(core::UnitId self_id) override {
        return systems_.getSystem<core::IPositionSystem>().advanceMarch(self_id);
    }

private:
    core::SystemsLocator& systems_;
};

}  // namespace sw::feature
