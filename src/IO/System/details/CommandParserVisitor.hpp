#pragma once

#include <IO/Types.hpp>
#include <iostream>

namespace sw {
class CommandParserVisitor {
public:
    explicit CommandParserVisitor(std::istream& stream) :
            stream_(stream) {}

    template <class TField>
    void visit([[maybe_unused]] const char* name, TField& field) {
        stream_ >> field;
    }

private:
    std::istream& stream_;
};
}  // namespace sw
