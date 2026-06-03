#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace sw::graphics {

class EventFeed {
public:
    struct RosterEntry {
        std::string type;
    };

    struct AttackEffect {
        sw::core::UnitId source;
        sw::core::UnitId target;
        sw::core::components::Position from;
        sw::core::components::Position to;
        bool ranged;
        int damage;
        double start;
    };

    static constexpr std::size_t kMaxLogLines = 1000;
    static constexpr double kAttackEffectDuration = 0.3;

    void subscribe(sw::core::Engine& engine);
    void clear();

    const std::deque<std::string>& lines() const {
        return lines_;
    }

    const std::unordered_map<sw::core::UnitId, RosterEntry>& roster() const {
        return roster_;
    }

    const std::vector<AttackEffect>& attacks() const {
        return attacks_;
    }

    std::uint32_t boardWidth() const {
        return boardWidth_;
    }

    std::uint32_t boardHeight() const {
        return boardHeight_;
    }

private:
    void push(const std::string& line);
    void recordAttack(const AttackEffect& effect);

    sw::core::Engine* engine_ = nullptr;
    std::deque<std::string> lines_;
    std::unordered_map<sw::core::UnitId, RosterEntry> roster_;
    std::vector<AttackEffect> attacks_;
    std::uint32_t boardWidth_ = 0;
    std::uint32_t boardHeight_ = 0;
};

}  // namespace sw::graphics
