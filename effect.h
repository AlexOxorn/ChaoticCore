//
// Created by alexoxorn on 8/20/22.
//

#ifndef CHAOTIC_CORE_EFFECT_H
#define CHAOTIC_CORE_EFFECT_H

#include "lua_obj.h"

class card;

class effect : public lua_obj_helper<PARAM_TYPE_EFFECT> {
    card* owner{ nullptr };
    card* handler{ nullptr };
    int8_t effect_owner{ -1 };
    uint32_t id{};
    uint16_t type{};
    int32_t condition{};
    int32_t cost{};
    int32_t target{};
    int32_t value{};
    int32_t operation{};
};


#endif//CHAOTIC_CORE_EFFECT_H
