#include "RngSystem.hpp"

#include <cassert>
#include <random>

namespace sw::core {

namespace {

struct CoreRngSystem : IRngSystem {
    std::mt19937 engine{0xC0FFEEU};

    UnitId pick(const std::vector<UnitId>& candidates) override {
        assert(!candidates.empty() && "pick from empty candidate set");
        return candidates[randomInt(0, static_cast<uint32_t>(candidates.size()) - 1)];
    }

    uint32_t randomInt(uint32_t lo, uint32_t hi) override {
        std::uniform_int_distribution<uint32_t> dist(lo, hi);
        return dist(engine);
    }

    ~CoreRngSystem() override = default;
};

}  // namespace

IRngSystemPtr makeCoreRngSystem() {
    return std::make_unique<CoreRngSystem>();
}

}  // namespace sw::core
