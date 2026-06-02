#pragma once

#include "details/PrintFieldVisitor.hpp"

#include <cstdint>
#include <iostream>

namespace sw {

class EventLog {
public:
    template <class TEvent>
    void log(uint64_t tick, const TEvent& event) {
        std::cout << "[" << tick << "] " << TEvent::name << " ";
        PrintFieldVisitor visitor(std::cout);
        event.visit(visitor);
        std::cout << std::endl;
    }
};

}  // namespace sw
