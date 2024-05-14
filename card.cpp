//
// Created by alexoxorn on 8/17/22.
//

#include "card.h"
#include <cstdio>
card::card(match* pm) : lua_obj_helper(pm) {
    temp.set0xff();
    current.controller = PLAYER::NONE;
}
void card::apply_field_effect() {}

template<typename T>
static constexpr void set_max_property_val(T& val) {
    val = static_cast<T>(~T());
}
void card_state::set0xff() {
    set_max_property_val(subtypes);
    set_max_property_val(name);
    set_max_property_val(sub_name);
    set_max_property_val(sequence.sequence);
    set_max_property_val(sequence.horizontal);
    set_max_property_val(sequence.vertical);
    set_max_property_val(moved_this_turn);
    set_max_property_val(engaged_in_combat);
    set_max_property_val(attack_damage_received);
    set_max_property_val(energy);
    set_max_property_val(damage);
    set_max_property_val(controller);
    set_max_property_val(stats.courage);
    set_max_property_val(stats.power);
    set_max_property_val(stats.speed);
    set_max_property_val(stats.wisdom);
    set_max_property_val(elements.fire);
    set_max_property_val(elements.earth);
    set_max_property_val(elements.water);
    set_max_property_val(elements.earth);
    set_max_property_val(mugic_ability);
    set_max_property_val(tribes);
    set_max_property_val(castable_mugic);
    set_max_property_val(card_type);
    set_max_property_val(flags);
    set_max_property_val(location);
    set_max_property_val(position);
    set_max_property_val(pseudo_brainwash);
    set_max_property_val(won_initiative);
    set_max_property_val(won_stat_check);
    set_max_property_val(won_challenge);
    set_max_property_val(won_stat_fail);
}
card_data::card_data(const CHAOTIC_CardData& data) {
#define COPY(val) val = data.val;
#define COPY_ENUM(val, enum) val = static_cast<enum>(data.val);
    COPY(code)
    COPY(supercode)
    COPY(subcode)
    COPY_ENUM(supertype, SUPERTYPE)
    COPY_ENUM(subtype, SUBTYPE)
    COPY_ENUM(tribe, TRIBE)
    if (supertype == SUPERTYPE::CREATURE) {
        stats.courage = static_cast<int16_t>(data.courage);
        stats.power = static_cast<int16_t>(data.power);
        stats.wisdom = static_cast<int16_t>(data.wisdom);
        stats.speed = static_cast<int16_t>(data.speed);
    }
    if (is(supertype, SUPERTYPE::CREATURE | SUPERTYPE::ATTACK)) {
        elements.fire = static_cast<int8_t>(data.fire);
        elements.air = static_cast<int8_t>(data.air);
        elements.earth = static_cast<int8_t>(data.earth);
        elements.water = static_cast<int8_t>(data.water);
        COPY(energy)
    }
    if (is(supertype, SUPERTYPE::CREATURE | SUPERTYPE::MUGIC)) {
        COPY(mugic_ability)
    }
#undef COPY
}
