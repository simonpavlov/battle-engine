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
    {
        engine.systems.registerSystem<core::IPositionSystem>(core::makeCorePositionSystem(engine));
        engine.systems.registerSystem<core::IHealthSystem>(core::makeCoreHealthSystem(engine));
        engine.systems.registerSystem<core::ICombatSystem>(core::makeCoreCombatSystem(engine));
        engine.systems.registerSystem<core::IRngSystem>(core::makeCoreRngSystem());
        engine.systems.registerSystem<core::IUnitSystem>(core::makeCoreUnitSystem(engine));

        auto& combat_system = engine.systems.getSystem<core::ICombatSystem>();
        combat_system.registerAttackKind(core::kMeleeAttackKind);
        combat_system.registerAttackKind(core::kRangedAttackKind);

        engine.components.registerComponent<core::components::Health>();
        engine.components.registerComponent<core::components::Position>();
        engine.components.registerComponent<core::components::Speed>();
        engine.components.registerComponent<core::components::Destination>();
        engine.components.registerComponent<core::components::Strength>();
        engine.components.registerComponent<core::components::Agility>();
        engine.components.registerComponent<core::components::Range>();

        auto& unit_system = engine.systems.getSystem<core::IUnitSystem>();
        unit_system.registerUnitType(feature::makeSwordsmanType(engine));
        unit_system.registerUnitType(feature::makeHunterType(engine));
        unit_system.registerUnitType(feature::makeRavenType(engine));
    }

    EventLog log;
    {
        auto& ps = engine.systems.getSystem<core::IPositionSystem>();
        auto& hs = engine.systems.getSystem<core::IHealthSystem>();
        auto& us = engine.systems.getSystem<core::IUnitSystem>();
        ps.onMapCreated().connect([&](const auto& e) { log.log(engine.tick, e); });
        ps.onMoved().connect([&](const auto& e) { log.log(engine.tick, e); });
        ps.onMarchStarted().connect([&](const auto& e) { log.log(engine.tick, e); });
        ps.onMarchEnded().connect([&](const auto& e) { log.log(engine.tick, e); });
        hs.onAttacked().connect([&](const auto& e) { log.log(engine.tick, e); });
        us.onSpawned().connect([&](const auto& e) { log.log(engine.tick, e); });
        us.onDied().connect([&](const auto& e) { log.log(engine.tick, e); });
    }

    {
        io::CommandParser parser;
        auto& ps = engine.systems.getSystem<core::IPositionSystem>();
        parser.add<core::commands::CreateMap>([&](core::commands::CreateMap c) { ps.setBounds(c.width, c.height); })
            .add<feature::commands::SpawnSwordsman>([&](feature::commands::SpawnSwordsman c) {
                feature::spawnSwordsman(engine, std::move(c));
            })
            .add<feature::commands::SpawnHunter>([&](feature::commands::SpawnHunter c) {
                feature::spawnHunter(engine, std::move(c));
            })
            .add<feature::commands::SpawnRaven>([&](feature::commands::SpawnRaven c) {
                feature::spawnRaven(engine, std::move(c));
            })
            .add<core::commands::March>([&](core::commands::March c) { ps.march(c.unitId, c.target); });
        parser.parse(file);
    }

    engine.run();

    return 0;
}
