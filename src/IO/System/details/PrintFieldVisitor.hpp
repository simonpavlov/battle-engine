#pragma once

#include <IO/Types.hpp>
#include <iostream>

namespace sw {
class PrintFieldVisitor {
public:
    explicit PrintFieldVisitor(std::ostream& stream) :
            stream_(stream) {}

    template <typename T>
    void visit(const char* name, const T& value) {
        stream_ << name << "=" << value << ' ';
    }

private:
    std::ostream& stream_;
};

}  // namespace sw
