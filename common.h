//
// Created by alexoxorn on 4/25/24.
//

#ifndef CHAOTIC_CORE_COMMON_H
#define CHAOTIC_CORE_COMMON_H

#if __cplusplus

  #define ENUM enum class

  #if defined(_MSC_VER) && !defined(__clang_analyzer__)
    #pragma warning(disable : 4244)
    #define unreachable() __assume(0)
    #define NoInline      __declspec(noinline)
    #define ForceInline   __forceinline
  #else
    #if !defined(__forceinline)
      #define ForceInline __attribute__((always_inline)) inline
    #else
      #define ForceInline __forceinline
    #endif
    #define unreachable() __builtin_unreachable()
    #define NoInline      __attribute__((noinline))
  #endif

  #if defined(__clang_analyzer__)
    #undef NDEBUG
  #endif

  #include <cstdint>
  #include <type_traits>
  #include <exception>

#else

  #include "stding.h"
  #include "stdint.h"
  #define ENUM enum

#endif

#if __cplusplus

struct bad_bit : std::exception {};

template <typename T>
constexpr auto BIT(T a) {
    if constexpr (std::is_integral_v<T> && !(std::is_enum_v<T>) ) {
        return 1ll << (a - 1);
    } else if constexpr (std::is_enum_v<T>) {
        return static_cast<std::underlying_type_t<T>>(a);
    } else {
        throw bad_bit{};
    }
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T operator~(T a) {
    return (T) ~(std::underlying_type_t<T>) a;
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T operator|(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a | (std::underlying_type_t<T>) b);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T operator&(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a & (std::underlying_type_t<T>) b);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T operator^(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a ^ (std::underlying_type_t<T>) b);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T& operator|=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a |= (std::underlying_type_t<T>) b);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T& operator&=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a &= (std::underlying_type_t<T>) b);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr T& operator^=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a ^= (std::underlying_type_t<T>) b);
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr bool operator!(T& a) {
    return static_cast<bool>(a);
}
template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr std::underlying_type_t<T> operator+(T& a) {
    return static_cast<std::underlying_type_t<T>>(a);
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr std::underlying_type_t<T> operator<<(long a, T b) {
    return a << (std::underlying_type_t<T>) b;
}

template <typename I, typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true,
          std::enable_if_t<std::is_convertible_v<I, std::underlying_type_t<T>>, bool> = true>
constexpr inline bool is(I val, T flag) {
    return static_cast<std::underlying_type_t<T>>(val) & static_cast<std::underlying_type_t<T>>(flag);
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
constexpr inline bool is(T val, T flag) {
    return static_cast<std::underlying_type_t<T>>(val) & static_cast<std::underlying_type_t<T>>(flag);
}

#else

long BIT(long a) {
    return 1 << (a - 1);
}

#endif

ENUM PLAYER : uint8_t{ONE, TWO, NONE, ALL};

ENUM LOCATION : uint16_t{NONE = 0,
                         ATTACK_DECK = BIT(1),
                         ATTACK_DISCARD = BIT(2),
                         FIELD = BIT(3),
                         HAND = BIT(4),
                         GENERAL_DISCARD = BIT(5),
                         REMOVED = BIT(6),
                         BURST = BIT(7),
                         ACTIVE_LOCATION = BIT(8),
                         LOCATION_DECK = BIT(9),
                         GLOBAL = FIELD | GENERAL_DISCARD | LOCATION_DECK};

ENUM POSITION : uint8_t{
                        FACE_UP = BIT(1),
                        FACE_DOWN = BIT(2),
                };

ENUM SUPERTYPE : uint8_t{
                         CREATURE = BIT(1),
                         BATTLE_GEAR = BIT(2),
                         MUGIC = BIT(3),
                         LOCATION = BIT(4),
                         ATTACK = BIT(5),
                 };

ENUM SUBTYPE : uint64_t{
                       WARRIOR = BIT(1),       ROYAL = BIT(2),         GUARDIAN = BIT(3),     ETHEREAL = BIT(4),
                       ELITE = BIT(5),         SCOUT = BIT(6),         ELEMENTALIST = BIT(7), CARETAKER = BIT(8),
                       COMMANDER = BIT(9),     CONQUEROR = BIT(10),    TASKMASTER = BIT(11),  NOBLE = BIT(12),
                       SQUADLEADER = BIT(13),  MANDIBLOR = BIT(14),    MUGE = BIT(15),        HERO = BIT(16),
                       BATTLEMASTER = BIT(17), STRATEGIST = BIT(18),   STALKER = BIT(19),     CONTROLLER = BIT(20),
                       AMBASSADOR = BIT(21),   WARBEAST = BIT(22),     CONJUROR = BIT(23),    MINION = BIT(24),
                       CHIEFTAIN = BIT(25),    FLUIDMORPHER = BIT(26), KHARALL = BIT(27),     PAST = BIT(28),
                       BEAST = BIT(29),        SHARD = BIT(30),        MIRAGE = BIT(31),
               };

ENUM TRIBE : uint8_t{
                     NONE = 0,
                     OVER_WORLD = BIT(1),
                     UNDER_WORLD = BIT(2),
                     MIPEDIAN = BIT(3),
                     DANIAN = BIT(4),
                     MARRILLIAN = BIT(5),
             };

ENUM FLAGS : uint8_t{INFECTED = BIT(1)};

ENUM ELEMENTS : uint8_t{
                        FIRE = BIT(1),
                        WIND = BIT(2),
                        EARTH = BIT(3),
                        WATER = BIT(4),
                };

ENUM STATS : uint8_t{
                     COURAGE = BIT(1),
                     POWER = BIT(2),
                     WISDOM = BIT(3),
                     SPEED = BIT(4),
             };

ENUM TURN_PHASE : uint8_t{
                          LOCATION_STEP = BIT(1),
                          ACTION_STEP = BIT(2),
                          RECOVERY_STEP = BIT(3),
                  };

ENUM COMBAT_PHASE : uint8_t{
                            NONE = 0,
                            ENGAGEMENT = BIT(1),
                            REVEAL_BATTLE = BIT(2),
                            BEGINNING_OF_COMBAT = BIT(3),
                            INITIATIVE = BIT(4),
                            STRIKE_PHASE = BIT(5),
                    };

ENUM ASSUME : uint32_t{NAME, SUPERTYPE, SUBTYPE, TRIBE, FLAGS, FIRE, WIND, EARTH, WATER, COURAGE, POWER, WISDOM, SPEED};

ENUM OPTION : uint8_t{MINION_TRIBAL_MUGIC = BIT(1),
                      SHOWDOWN_4_TURNS = BIT(2),
                      ADVANCED_APPRENTICE = BIT(3),
                      DEFENDERS_WIN_INITIATIVE = BIT(4),
                      MINION_BOTH_ABILITIES = BIT(6),
                      SECOND_PLAYER_ATTACK_MULLIGAN = BIT(7)};

#define DECK_TOP     0
#define DECK_BOTTOM  1
#define DECK_SHUFFLE 2

typedef struct {
    uint32_t horizontal;
    uint32_t vertical;
    uint32_t sequence;
} sequence_type;

#endif // CHAOTIC_CORE_COMMON_H
