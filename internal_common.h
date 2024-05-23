//
// Created by alexoxorn on 4/25/24.
//

#ifndef CHAOTIC_CORE_INTERNAL_COMMON_H
#define CHAOTIC_CORE_INTERNAL_COMMON_H

#include <cstdint>
#include <type_traits>
#include <exception>
#include <vector>
#include "common.h"

struct unimplemented : std::exception {
    const char* message;
    explicit unimplemented(const char* reason) : message(reason) {}

    [[nodiscard]] const char* what() const noexcept override { return message; }
};


struct loc_info {
    PLAYER controller;
    LOCATION location;
    sequence_type sequence;
    POSITION position;
};

class MSG_locinfo;

void write_location_info(MSG_locinfo* msg, loc_info info);

struct burst;
using burst_array = std::vector<burst>;

class card;
using card_vector = std::vector<card*>;

#endif // CHAOTIC_CORE_INTERNAL_COMMON_H
