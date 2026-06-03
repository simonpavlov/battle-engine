#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Systems.hpp>
#include <memory>

#include "EventFeed.hpp"
#include "IconSet.hpp"
#include "VisualizerState.hpp"

namespace sw::graphics {

struct IGraphicsSystem : sw::core::ISystem {
    virtual void frame() = 0;
    virtual bool shouldClose() = 0;

    ~IGraphicsSystem() override = default;
};

std::unique_ptr<IGraphicsSystem>
makeGraphicsSystem(sw::core::Engine& engine, VisualizerState& state, EventFeed& feed, const IconSet& icons);

}  // namespace sw::graphics
