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
  #include <concepts>
  #include <initializer_list>

#else

  #include "stding.h"
  #include "stdint.h"
  #define ENUM enum

#endif

#if __cplusplus

struct bad_bit : std::exception {};

template <typename T>
concept enum_class = std::is_enum<T>::value;

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

template <enum_class T>
constexpr T operator~(T a) {
    return (T) ~(std::underlying_type_t<T>) a;
}
template <enum_class T>
constexpr T operator|(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a | (std::underlying_type_t<T>) b);
}
template <enum_class T>
constexpr T operator&(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a & (std::underlying_type_t<T>) b);
}
template <enum_class T>
constexpr T operator^(T a, T b) {
    return (T) ((std::underlying_type_t<T>) a ^ (std::underlying_type_t<T>) b);
}
template <enum_class T>
constexpr T& operator|=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a |= (std::underlying_type_t<T>) b);
}
template <enum_class T>
constexpr T& operator&=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a &= (std::underlying_type_t<T>) b);
}
template <enum_class T, std::integral S>
constexpr T& operator&=(T& a, S b) {
    return (T&) ((std::underlying_type_t<T>&) a &= b);
}
template <enum_class T>
constexpr T& operator^=(T& a, T b) {
    return (T&) ((std::underlying_type_t<T>&) a ^= (std::underlying_type_t<T>) b);
}

template <enum_class T>
constexpr bool operator!(const T a) {
    return !static_cast<bool>(a);
}

template <enum_class T>
constexpr std::underlying_type_t<T> operator+(T a) {
    return static_cast<std::underlying_type_t<T>>(a);
}

template <enum_class T>
constexpr std::underlying_type_t<T> operator<<(long a, T b) {
    return a << (std::underlying_type_t<T>) b;
}

template <enum_class T, std::convertible_to<std::underlying_type_t<T>> I>
constexpr inline bool is(I val, T flag) {
    return static_cast<std::underlying_type_t<T>>(val) & static_cast<std::underlying_type_t<T>>(flag);
}

template <enum_class T>
constexpr inline bool is(T val, T flag) {
    return static_cast<std::underlying_type_t<T>>(val) & static_cast<std::underlying_type_t<T>>(flag);
}

#else

long BIT(long a) {
    return 1 << (a - 1);
}

  #define is(X, Y) (X & Y)

#endif

ENUM PLAYER : uint8_t{ONE, TWO, NONE, ALL};

template<enum_class T, typename ... ARGS>
constexpr inline bool any_of(T val, ARGS... flag) requires (std::is_same_v<T, ARGS> && ...) {
    return ((val == flag) || ...);
    return false;
}



inline PLAYER opp(PLAYER playerid) {
    switch (playerid) {
        case PLAYER::ONE: return PLAYER::TWO;
        case PLAYER::TWO: return PLAYER::ONE;
        default: return playerid;
    }
}

ENUM LOCATION : uint32_t{
                        NONE = 0,
                        ATTACK_DECK = BIT(1),
                        ATTACK_DISCARD = BIT(2),
                        FIELD = BIT(3),
                        MUGIC_HAND = BIT(4),
                        ATTACK_HAND = BIT(5),
                        GENERAL_DISCARD = BIT(6),
                        REMOVED = BIT(7),
                        BURST = BIT(8),
                        ACTIVE_LOCATION = BIT(9),
                        LOCATION_DECK = BIT(10),
                        GLOBAL = FIELD | GENERAL_DISCARD | LOCATION_DECK,
                        HAND = ATTACK_HAND | MUGIC_HAND,
                        DECK = ATTACK_DECK | LOCATION_DECK,
                        GRAVE = ATTACK_DISCARD | GENERAL_DISCARD,
                        VECTOR_AREA = DECK | GRAVE | HAND | REMOVED,
                        BOTTOM_ATTACK_DECK = ATTACK_DECK | BIT(17),
                        SHUFFLE_ATTACK_DECK = ATTACK_DECK | BIT(18),
                        BOTTOM_LOCATION_DECK = LOCATION_DECK | BIT(17),
                        SHUFFLE_LOCATION_DECK = LOCATION_DECK | BIT(18),
                };

ENUM STATUS {
        DESTROY_CONFIRMED = BIT(1)
};

ENUM POSITION : uint8_t{
                        NONE = 0,
                        FACE_UP = BIT(1),
                        FACE_DOWN = BIT(2),
                };

ENUM SUPERTYPE : uint8_t{
                         CREATURE,
                         BATTLE_GEAR,
                         MUGIC,
                         LOCATION,
                         ATTACK,
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

ENUM PHASE : uint8_t{
                     NONE = 0,
                     LOCATION_STEP = BIT(1),
                     ACTION_STEP = BIT(2),
                     RECOVERY_STEP = BIT(3),
                     ENGAGEMENT = BIT(4),
                     REVEAL_BATTLE = BIT(5),
                     BEGINNING_OF_COMBAT = BIT(6),
                     INITIATIVE = BIT(7),
                     STRIKE_PHASE = BIT(8),
             };

ENUM ASSUME : uint32_t{NAME, SUPERTYPE, SUBTYPE, TRIBE, FLAGS, FIRE, WIND, EARTH, WATER, COURAGE, POWER, WISDOM, SPEED};

ENUM OPTION : uint8_t{MINION_TRIBAL_MUGIC = BIT(1),
                      SHOWDOWN_4_TURNS = BIT(2),
                      ADVANCED_APPRENTICE = BIT(3),
                      DEFENDERS_WIN_INITIATIVE = BIT(4),
                      MINION_BOTH_ABILITIES = BIT(6),
                      SECOND_PLAYER_ATTACK_MULLIGAN = BIT(7)};

ENUM RESET : uint32_t{
                     EVENT = BIT(13),
                     CARD = BIT(14),
                     COPY = BIT(15),
                     CODE = BIT(16),

                     DISABLE = BIT(17),
                     TURN_SET = BIT(18),
                     TO_GRAVE = BIT(19),
                     REMOVE = BIT(20),
                     TO_HAND = BIT(21),
                     TO_DECK = BIT(22),
                     LEAVE = BIT(23),
                     TO_FIELD = BIT(24),
                     CONTROL = BIT(25),

                     BASIC = TO_GRAVE | REMOVE | TO_HAND | TO_DECK | LEAVE | TO_FIELD,

                     SELF_TURN = BIT(29),
                     OPPO_TURN = BIT(20),
                     PHASE = BIT(31),
                     CHAIN = BIT(32),
             };

#define DECK_TOP     0
#define DECK_BOTTOM  1
#define DECK_SHUFFLE 2


#endif // CHAOTIC_CORE_COMMON_H
