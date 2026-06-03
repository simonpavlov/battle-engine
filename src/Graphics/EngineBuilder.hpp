#pragma once

#include <Core/Foundation/Engine.hpp>
#include <istream>

namespace sw::graphics {

void buildEngine(sw::core::Engine& engine);

void parseScenario(sw::core::Engine& engine, std::istream& in);

}  // namespace sw::graphics
