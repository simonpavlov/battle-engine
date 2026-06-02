#include "RngSystem.hpp"

#include <cassert>
#include <random>

namespace sw::core {

namespace {

class CoreRngSystem : public IRngSystem {
public:
    UnitId pick(const std::vector<UnitId>& candidates) override {
        assert(!candidates.empty() && "pick from empty candidate set");
        return candidates[randomInt(0, static_cast<uint32_t>(candidates.size()) - 1)];
    }

    uint32_t randomInt(uint32_t lo, uint32_t hi) override {
        std::uniform_int_distribution<uint32_t> dist(lo, hi);
        return dist(engine_);
    }

    ~CoreRngSystem() override = default;

private:
    std::mt19937 engine_{0xC0FFEEU};
};

}  // namespace

IRngSystemPtr makeCoreRngSystem() {
    return std::make_unique<CoreRngSystem>();
}

}  // namespace sw::core
