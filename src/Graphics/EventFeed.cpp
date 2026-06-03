#include "EventFeed.hpp"

#include <Core/Foundation/UnitEvents.hpp>
#include <Core/Foundation/UnitSystem.hpp>
#include <Core/Modules/Spatial/PositionEvents.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Vitals/HealthEvents.hpp>
#include <Core/Modules/Vitals/HealthSystem.hpp>
#include <IO/System/details/PrintFieldVisitor.hpp>
#include <sstream>

#include "raylib.h"

namespace sw::graphics {

namespace {

template <class TEvent>
std::string format(std::uint64_t tick, const TEvent& event) {
    std::ostringstream out;
    out << "[" << tick << "] " << TEvent::name << " ";
    PrintFieldVisitor visitor(out);
    event.visit(visitor);
    return out.str();
}

}  // namespace

void EventFeed::push(const std::string& line) {
    lines_.push_back(line);
    while (lines_.size() > kMaxLogLines) {
        lines_.pop_front();
    }
}

void EventFeed::recordAttack(const AttackEffect& effect) {
    const double cutoff = effect.start - kAttackEffectDuration;
    std::erase_if(attacks_, [cutoff](const AttackEffect& e) { return e.start < cutoff; });
    attacks_.push_back(effect);
}

void EventFeed::subscribe(sw::core::Engine& engine) {
    engine_ = &engine;

    auto& position_system = engine.systems.getSystem<sw::core::IPositionSystem>();
    auto& health_system = engine.systems.getSystem<sw::core::IHealthSystem>();
    auto& unit_system = engine.systems.getSystem<sw::core::IUnitSystem>();

    position_system.onMapCreated().connect([this](const sw::core::events::MapCreated& e) {
        boardWidth_ = e.width;
        boardHeight_ = e.height;
        push(format(engine_->tick, e));
    });
    position_system.onMoved().connect([this](const sw::core::events::Moved& e) {
        push(format(engine_->tick, e));
    });
    position_system.onMarchStarted().connect([this](const sw::core::events::MarchStarted& e) {
        push(format(engine_->tick, e));
    });
    position_system.onMarchEnded().connect([this](const sw::core::events::MarchEnded& e) {
        push(format(engine_->tick, e));
    });
    health_system.onAttacked().connect([this, &position_system](const sw::core::events::Attacked& e) {
        push(format(engine_->tick, e));
        const auto from = position_system.getPosition(e.source);
        const auto to = position_system.getPosition(e.target);
        recordAttack(AttackEffect{
            .source = e.source,
            .target = e.target,
            .from = from,
            .to = to,
            .ranged = sw::core::chebyshev(from, to).value >= 2,
            .damage = e.damage.value,
            .start = GetTime(),
        });
    });
    unit_system.onSpawned().connect([this](const sw::core::events::Spawned& e) {
        roster_[e.unit_id] = RosterEntry{std::string(e.type)};
        push(format(engine_->tick, e));
    });
    unit_system.onDied().connect([this](const sw::core::events::Died& e) {
        roster_.erase(e.unit_id);
        push(format(engine_->tick, e));
    });
}

void EventFeed::clear() {
    lines_.clear();
    roster_.clear();
    attacks_.clear();
    boardWidth_ = 0;
    boardHeight_ = 0;
}

}  // namespace sw::graphics
