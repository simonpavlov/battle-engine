#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitEvents.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Combat/CombatSystem.hpp>
#include <Core/Modules/Rng/RngSystem.hpp>
#include <Core/Modules/Spatial/Destination.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionCommands.hpp>
#include <Core/Modules/Spatial/PositionEvents.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Speed.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <Core/Modules/Vitals/HealthEvents.hpp>
#include <Core/Modules/Vitals/HealthSystem.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Raven.hpp>
#include <Features/Units/Swordsman.hpp>
#include <IO/System/CommandParser.hpp>
#include <IO/System/EventLog.hpp>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    using namespace sw;

    if (argc != 2) {
        throw std::runtime_error("Error: No file specified in command line argument");
    }

    std::ifstream file(argv[1]);
    if (!file) {
        throw std::runtime_error("Error: File not found - " + std::string(argv[1]));
    }

    core::Engine engine;
    auto& systems = engine.systems;
    auto& components = engine.components;
    {
        systems.registerSystem<core::IPositionSystem>(core::makeCorePositionSystem(components, systems));
        systems.registerSystem<core::IHealthSystem>(core::makeCoreHealthSystem(components, systems));
        systems.registerSystem<core::ICombatSystem>(core::makeCoreCombatSystem(systems));
        systems.registerSystem<core::IRngSystem>(core::makeCoreRngSystem());
        systems.registerSystem<core::IUnitSystem>(core::makeCoreUnitSystem(components));

        auto& combat_system = systems.getSystem<core::ICombatSystem>();
        combat_system.registerAttackKind(core::kMeleeAttackKind);
        combat_system.registerAttackKind(core::kRangedAttackKind);

        components.registerComponent<core::components::Health>();
        components.registerComponent<core::components::Position>();
        components.registerComponent<core::components::Speed>();
        components.registerComponent<core::components::Destination>();
        components.registerComponent<core::components::Strength>();
        components.registerComponent<core::components::Agility>();
        components.registerComponent<core::components::Range>();

        auto& unit_system = systems.getSystem<core::IUnitSystem>();
        unit_system.registerUnitType(feature::makeSwordsmanType(components, systems));
        unit_system.registerUnitType(feature::makeHunterType(components, systems));
        unit_system.registerUnitType(feature::makeRavenType(components, systems));
    }

    EventLog log;
    {
        auto& position_system = systems.getSystem<core::IPositionSystem>();
        auto& health_system = systems.getSystem<core::IHealthSystem>();
        auto& unit_system = systems.getSystem<core::IUnitSystem>();
        position_system.onMapCreated().connect([&](const auto& e) { log.log(engine.tick, e); });
        position_system.onMoved().connect([&](const auto& e) { log.log(engine.tick, e); });
        position_system.onMarchStarted().connect([&](const auto& e) { log.log(engine.tick, e); });
        position_system.onMarchEnded().connect([&](const auto& e) { log.log(engine.tick, e); });
        health_system.onAttacked().connect([&](const auto& e) { log.log(engine.tick, e); });
        unit_system.onSpawned().connect([&](const auto& e) { log.log(engine.tick, e); });
        unit_system.onDied().connect([&](const auto& e) { log.log(engine.tick, e); });
    }

    {
        io::CommandParser parser;
        auto& position_system = systems.getSystem<core::IPositionSystem>();
        parser.add<core::commands::CreateMap>([&](auto c) { position_system.setBounds(c.width, c.height); })
            .add<feature::commands::SpawnSwordsman>([&](auto c) {
                feature::spawnSwordsman(components, systems, std::move(c));
            })
            .add<feature::commands::SpawnHunter>([&](auto c) {
                feature::spawnHunter(components, systems, std::move(c));
            })
            .add<feature::commands::SpawnRaven>([&](auto c) { feature::spawnRaven(components, systems, std::move(c)); })
            .add<core::commands::March>([&](auto c) { position_system.march(c.unitId, c.target); });
        parser.parse(file);
    }

    engine.run();

    return 0;
}
