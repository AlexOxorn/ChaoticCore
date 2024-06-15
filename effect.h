//
// Created by alexoxorn on 8/20/22.
//

#ifndef CHAOTIC_CORE_EFFECT_H
#define CHAOTIC_CORE_EFFECT_H

#include "lua_obj.h"
#include "common.h"
#include "internal_common.h"
#include "effect_constants.h"

class card;

/*
 *
pmatch->lua->add_param<LuaParam::INT>(playerid);
pmatch->lua->add_param<LuaParam::GROUP>(e.event_cards );
pmatch->lua->add_param<LuaParam::INT>(e.event_player);
pmatch->lua->add_param<LuaParam::INT>(e.event_value);
pmatch->lua->add_param<LuaParam::EFFECT>(e.reason_effect );
pmatch->lua->add_param<LuaParam::INT>(e.reason);
pmatch->lua->add_param<LuaParam::INT>(e.reason_player);
 */

class effect;

using effect_function = void(effect*, PLAYER player_id, card_set* event_group, PLAYER event_player, int32_t event_value,
                             effect* reason_effect, REASON reason, PLAYER reason_player);

class effect : public lua_obj_helper<PARAM_TYPE_EFFECT> {
    card* owner{nullptr};
    card* handler{nullptr};
    int8_t effect_owner{-1};
    uint32_t id{};
    uint16_t type{};
    int32_t condition{};
    int32_t cost{};
    int32_t target{};
    int32_t value{};
    int32_t operation{};
};

#endif // CHAOTIC_CORE_EFFECT_H
