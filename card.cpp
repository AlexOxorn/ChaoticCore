//
// Created by alexoxorn on 8/17/22.
//

#include "card.h"
#include "match.h"
#include "field.h"
#include <cstdio>

bool card_sort::operator()(const card* c1, const card* c2) const {
    return c1->cardid < c2->cardid;
}

card::card(match* pm) : lua_obj_helper(pm) {
    temp.set0xff();
    current.controller = PLAYER::NONE;
}
void card::cancel_field_effect() {}
void card::reset(RESET id, RESET type) {
    fprintf(stderr, "card::reset unimplemented\n");
    using enum RESET;
    if (!is(type, EVENT | PHASE | CODE | COPY | CARD))
        return;
    for (auto rit = relations.begin(); rit != relations.end();) {
        auto rrm = rit++;
        if (is(rrm->second & 0xffffff00, id)) {
            relations.erase(rrm);
        }
        if (is(id, TO_DECK | TO_HAND | TO_GRAVE | REMOVE)) {
            clear_related_effect();
        }
    }
}
void card::clear_related_effect() {
    fprintf(stderr, "card::clear_related_effect unimplemented\n");
}
void card::refresh_negated_status() {
    fprintf(stderr, "card::refresh_negated_status unimplemented\n");
}
void card::equip(card* target, bool send_msg) {
    if (current.equipped_creature)
        return;
    target->previous.battlegear = nullptr;
    target->current.battlegear = this;
    previous.equipped_creature = current.equipped_creature;
    current.equipped_creature = target;
    // NEGATED CHECK LIST
}
void card::unequip() {
    if (!current.equipped_creature)
        return;
    // NEGATED CHECK LIST
    current.equipped_creature->previous.battlegear = current.equipped_creature->current.battlegear;
    current.equipped_creature->current.battlegear = nullptr;
    previous.equipped_creature = current.equipped_creature;
    current.equipped_creature = nullptr;
}
void card::clear_card_target() {
    fprintf(stderr, "card::clear_card_target unimplemented\n");
}
effect* card::is_affected_by_effect(EFFECT) {
    fprintf(stderr, "card::is_affected_by_effect unimplemented\n");
    return nullptr;
}
std::pair<int32_t, uint8_t> card::calculate_attack_damage(card* source, card* target) {
    int32_t total = data.energy;
    int32_t element = 0;
    for (int i = 0; i < 4; ++i) {
        if (data.elements.data[i] < 0)
            continue;
        if (!source->data.elements.data[i])
            continue;
        total += data.elements.data[i];
        element |= (1 << i);
    }
    return {total, element};
}
bool card::card_operation_sort(card* c1, card* c2) {
    match* pmatch = c1->pmatch;
    PLAYER cp1 = c1->current.controller;
    PLAYER cp2 = c2->current.controller;
    if (cp1 != cp2) {
        if (cp1 == PLAYER::NONE || cp2 == PLAYER::NONE)
            return cp1 < cp2;
        if (pmatch->game_field->infos.turn_player == 0)
            return cp1 < cp2;
        else
            return cp1 > cp2;
    }
    if (c1->current.location != c2->current.location)
        return c1->current.location < c2->current.location;
    if (is(c1->current.location, (LOCATION::DECK | LOCATION::GRAVE | LOCATION::REMOVED)))
        return c1->current.sequence.index > c2->current.sequence.index;
    else if (is(c1->current.location, LOCATION::FIELD)) {
        return pmatch->game_field->index_from(c1->current.sequence)
             < pmatch->game_field->index_from(c2->current.sequence);
    }
    return c1->current.sequence.index < c2->current.sequence.index;
}

template <typename T>
constexpr static void set_max_property_val(T& val) {
    val = static_cast<T>(~T());
}
void card_state::set0xff() {
    set_max_property_val(subtypes);
    set_max_property_val(name);
    set_max_property_val(sub_name);
    set_max_property_val(sequence.index);
    set_max_property_val(sequence.horizontal);
    set_max_property_val(sequence.vertical);
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
}
card_data::card_data(const CHAOTIC_CardData& data) {
#define COPY(val)            val = data.val;
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
    if (any_of(supertype, SUPERTYPE::CREATURE, SUPERTYPE::ATTACK)) {
        elements.fire = static_cast<int8_t>(data.fire);
        elements.air = static_cast<int8_t>(data.air);
        elements.earth = static_cast<int8_t>(data.earth);
        elements.water = static_cast<int8_t>(data.water);
        COPY(energy)
    }
    if (any_of(supertype, SUPERTYPE::CREATURE, SUPERTYPE::MUGIC)) {
        COPY(mugic_ability)
    }
#undef COPY
}
