//
// Created by alexoxorn on 4/25/24.
//

#ifndef CHAOTIC_CORE_INTERNAL_COMMON_H
#define CHAOTIC_CORE_INTERNAL_COMMON_H

#include <cstdint>
#include <type_traits>
#include <exception>

struct unimplemented : std::exception {
    const char* message;
    explicit unimplemented(const char* reason) : message(reason) {}

    [[nodiscard]] const char* what() const noexcept override { return message; }
};

#endif // CHAOTIC_CORE_INTERNAL_COMMON_H
