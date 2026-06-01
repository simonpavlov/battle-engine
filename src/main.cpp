#include <Core/Destination.hpp>
#include <Core/Engine.hpp>
#include <Core/Health.hpp>
#include <Core/HealthSystem.hpp>
#include <Core/PositionSystem.hpp>
#include <Core/RngSystem.hpp>
#include <Core/Strength.hpp>
#include <Features/Hunter.hpp>
#include <Features/Swordsman.hpp>
#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/March.hpp>
#include <IO/Commands/SpawnHunter.hpp>
#include <IO/Commands/SpawnSwordsman.hpp>
#include <IO/Events/MapCreated.hpp>
#include <IO/Events/MarchEnded.hpp>
#include <IO/Events/MarchStarted.hpp>
#include <IO/Events/UnitAttacked.hpp>
#include <IO/Events/UnitDied.hpp>
#include <IO/Events/UnitMoved.hpp>
#include <IO/Events/UnitSpawned.hpp>
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
        engine.systems.registerSystem<core::IPositionSystem>(core::MakeCorePositionSystem(engine));
        engine.systems.registerSystem<core::IHealthSystem>(core::MakeCoreHealthSystem(engine));
        engine.systems.registerSystem<core::IRngSystem>(core::MakeCoreRngSystem());

        engine.components.registerComponent<core::Position>();
        engine.components.registerComponent<core::Health>();
        engine.components.registerComponent<core::Strength>();
        engine.components.registerComponent<core::Destination>();

        engine.registerUnitType(feature::MakeSwordsmanType(engine));
    }

    EventLog log;
    {
        auto& ps = engine.systems.getSystem<core::IPositionSystem>();
        auto& hs = engine.systems.getSystem<core::IHealthSystem>();
        ps.onMapCreated().connect([&](uint32_t w, uint32_t h) { log.log(engine.tick, io::MapCreated{w, h}); });
        ps.onMoved().connect(
            [&](core::UnitId id, core::Position p) { log.log(engine.tick, io::UnitMoved{id.value, p.x, p.y}); });
        ps.onMarchStarted().connect([&](core::UnitId id, core::Position from, core::Position to) {
            log.log(engine.tick, io::MarchStarted{id.value, from.x, from.y, to.x, to.y});
        });
        ps.onMarchEnded().connect(
            [&](core::UnitId id, core::Position p) { log.log(engine.tick, io::MarchEnded{id.value, p.x, p.y}); });
        hs.onAttacked().connect([&](core::UnitId src, core::UnitId tgt, int dmg, int tgt_hp) {
            log.log(
                engine.tick,
                io::UnitAttacked{src.value, tgt.value, static_cast<uint32_t>(dmg), static_cast<uint32_t>(tgt_hp)});
        });
        engine.onSpawned().connect([&](core::UnitId id, std::string_view type, core::Position p) {
            log.log(engine.tick, io::UnitSpawned{id.value, std::string(type), p.x, p.y});
        });
        engine.onDied().connect([&](core::UnitId id) { log.log(engine.tick, io::UnitDied{id.value}); });
    }

    {
        io::CommandParser parser;
        auto& ps = engine.systems.getSystem<core::IPositionSystem>();
        parser.add<io::CreateMap>([&](io::CreateMap c) { ps.setBounds(c.width, c.height); })
            .add<io::SpawnSwordsman>([&](io::SpawnSwordsman c) {
                feature::spawnSwordsman(
                    engine,
                    core::UnitId{c.unitId},
                    core::Position{c.x, c.y},
                    // TODO: change Core HP to uint32_t
                    static_cast<int>(c.hp),
                    c.strength);
            })
            .add<io::SpawnHunter>([&](io::SpawnHunter) { feature::spawnHunter(engine); })
            .add<io::March>(
                [&](io::March c) { ps.march(core::UnitId{c.unitId}, core::Position{c.targetX, c.targetY}); });
        parser.parse(file);
    }

    engine.run();

    return 0;
}
