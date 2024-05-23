//
// Created by alexoxorn on 5/14/24.
//

#ifndef CHAOTIC_CORE_EFFECT_CONSTANTS_H
#define CHAOTIC_CORE_EFFECT_CONSTANTS_H

#include "common.h"

enum class EVENT {
    STARTUP = 1000,
    LEAVE_FIELD_P,
    LEAVE_FIELD,
    LEAVE_GRAVE,
    TO_HAND,
    TO_DECK,
    TO_GRAVE,
    REMOVE,
    DISCARD,
    SACRIFICE,
    DESTROYED,
    MOVE,
    DRAW
};

enum class REASON {
    NONE = 0,
    DESTROY = BIT(1),
    REMOVED = BIT(2),
    EFFECT = BIT(3),
    COST = BIT(4),
    ADJUST = BIT(5),
    LOST_TARGET = BIT(6),
    RULE = BIT(7),
    BATTLE_GEAR_FLIP = BIT(8),
    DISCARD = BIT(9),
    DAMAGE = BIT(10),
    RETURN = BIT(11),
    REDIRECT = BIT(12),
    SACRIFICE = BIT(13),
    DRAW = BIT(13)
};

enum class EFFECT {
    PUBLIC,
    START_GAME_FACEUP,
};

#endif // CHAOTIC_CORE_EFFECT_CONSTANTS_H
