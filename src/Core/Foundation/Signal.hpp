#pragma once

#include <functional>
#include <vector>

namespace sw::core {

template <class... TArgs>
class Signal {
public:
    class Sink {
    public:
        explicit Sink(Signal& signal) :
                signal_(&signal) {}

        void connect(std::function<void(TArgs...)> slot) {
            signal_->connect(std::move(slot));
        }

    private:
        Signal* signal_;
    };

    void connect(std::function<void(TArgs...)> slot) {
        slots_.push_back(std::move(slot));
    }

    void emit(TArgs... args) const {
        for (const auto& slot : slots_) {
            slot(args...);
        }
    }

    Sink sink() {
        return Sink{*this};
    }

private:
    std::vector<std::function<void(TArgs...)>> slots_;
};

}  // namespace sw::core
