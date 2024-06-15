//
// Created by alexoxorn on 4/25/24.
//

#ifndef CHAOTIC_CORE_INTERNAL_COMMON_H
#define CHAOTIC_CORE_INTERNAL_COMMON_H

#include <cstdint>
#include <type_traits>
#include <exception>
#include <algorithm>
#include <vector>
#include <tuple>
#include "common.h"
#include <set>

struct unimplemented : std::exception {
    const char* message;
    explicit unimplemented(const char* reason) : message(reason) {}

    [[nodiscard]] const char* what() const noexcept override { return message; }
};

struct sequence_type {
    union {
        struct {
            int32_t horizontal;
            int32_t vertical;
        };
        int32_t index;
    };
    sequence_type() = default;
    sequence_type(int32_t x) : index{x} {}
    sequence_type(int32_t x, int32_t y) : horizontal{x}, vertical{y} {}
    sequence_type(std::initializer_list<int32_t> x) : horizontal{*x.begin()}, vertical{*(x.begin() + 1)} {}
    bool operator==(const sequence_type& other) const {
        return (horizontal == other.horizontal && vertical == other.vertical) or (index == other.index);
    };

    std::pair<uint32_t, uint32_t> as_pair() { return {horizontal, vertical}; }
    template <size_t I>
    int32_t& get() {
        if constexpr (I == 0)
            return horizontal;
        else
            return vertical;
    }
};

template <>
struct std::tuple_size<sequence_type> : std::integral_constant<std::size_t, 2> {};

template <size_t I>
struct std::tuple_element<I, sequence_type> {
    using type = int32_t;
};

struct loc_info {
    PLAYER controller;
    LOCATION location;
    sequence_type sequence;
    POSITION position;
};

class MSG_locinfo;
class MSG_sequence;
class MSG_coordinate;

void write_location_info(MSG_locinfo* msg, loc_info info);
void write_sequence_info(MSG_sequence* msg, sequence_type seq, LOCATION loc);
void write_coord_info(MSG_coordinate* msg, sequence_type seq);

struct burst;
class card;
struct tevent;
struct effect;

using burst_array = std::vector<burst>;
using card_vector = std::vector<card*>;
using event_list = std::vector<tevent>;
using effect_list = std::vector<effect*>;

#define FOR_PLAYER_ID(X) for (int X = 0; X < 2; ++X)

#ifdef _MSC_VER
  #define COALESCE_SAFE(T, F) \
      (([&]() { \
        auto&& t = T; \
        if (t) \
            return std::forward<decltype(t)>(t); \
        return F; \
      })())
  #define COALESCE(T, F) (T ? T : F)
#else
  #define COALESCE(T, F)      (T ?: F)
  #define COALESCE_SAFE(T, F) (T ?: F)
#endif

struct card_sort {
    bool operator()(const card* c1, const card* c2) const;
};
using card_set = std::set<card*, card_sort>;

#endif // CHAOTIC_CORE_INTERNAL_COMMON_H
