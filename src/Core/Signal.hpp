#pragma once

#include <functional>
#include <vector>

namespace sw::core {

// A typed event channel owned by the producer that emits it. The signal's parameter list *is* the
// event contract, so there is no separate event-struct vocabulary in Core. Subscribers (the logger,
// or any system added later) connect through the producer's interface; emission is synchronous and
// runs slots in connect order.
template <class... TArgs>
class Signal {
public:
    // Connect-only handle exposed to consumers. Holds no power to emit, so a subscriber can never
    // fire an event it does not own — only the signal's owner (which holds the Signal itself) emits.
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
