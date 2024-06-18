//
// Created by alexoxorn on 8/17/22.
//

#ifndef CHAOTIC_CORE_CARD_H
#define CHAOTIC_CORE_CARD_H

#include "common.h"
#include "lua_obj.h"
#include "stdio.h"
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include "chaotic_api_types.h"
#include "effect_constants.h"
#include "effect.h"
#include "internal_common.h"
#include <unordered_map>

/*
 * Database Column Brainstorm
 *
 * ID: 1 Bytes for SetID, 3 Bytes for Number within set
 * NAME: ID of the first card to share the proper name
 * SUBNAME: Increments for each new card of a particular NAME
 * RARITY: One byte, index of (Common, Uncommon, Rare, SuperRare, UltraRare, Promo)
 * TRIBE: bit field, see enum `tribe`
 * COURAGE:
 *      - Creature: has courage of value
 *      - Attack: bit field of (has check, has challenge, has fail)
 * POWER
 * WISDOM
 * SPEED
 * ENERGY:
 *      - Creature: Max Energy
 *      - Attack: Base Damage
 * ELEMENTS: 4 bytes, one for each fire, wind, earth, water
 *      - Creatures: 1 means it has type, 0 means it does not
 *      - Attack: X means it has element type with damage 5 * (X-1)
 * ARCHETYPES: bit field, see enum `archetype`
 * LOYAL:
 * UNIQUE:
 * LEGENDARY:
 * ARTIST
 *
 * IMAGE DATABASE:
 * ID: FOREIGN KEY
 * IC, IF, IA
 *
 * Text
 */

struct disciplines {
    int16_t courage;
    int16_t power;
    int16_t wisdom;
    int16_t speed;
};

union element_values {
    struct {
        int8_t fire;
        int8_t air;
        int8_t earth;
        int8_t water;
    };
    int8_t data[4];
};

struct card_data {
    uint32_t code{};
    SUPERTYPE supertype{};
    uint32_t supercode{};
    uint32_t subcode{};
    TRIBE tribe{};
    SUBTYPE subtype{};
    disciplines stats{};
    element_values elements{};
    int8_t mugic_ability{};
    int16_t energy{};

    card_data(const CHAOTIC_CardData& data);
    card_data() = default;
};

struct sendto_params {
    void set(PLAYER p, POSITION pos, LOCATION loc) {
        playerid = p;
        position = pos;
        location = loc;
        sequence = {};
    }
    void set(PLAYER p, POSITION pos, LOCATION loc, uint32_t ind1, uint32_t ind2) {
        set(p, pos, loc);
        sequence.horizontal = ind1;
        sequence.vertical = ind2;
    }
    void set(PLAYER p, POSITION pos, LOCATION loc, uint32_t seq) {
        set(p, pos, loc);
        sequence.index = seq;
    }
    void clear() {
        playerid = PLAYER::NONE;
        position = POSITION::FACE_DOWN;
        location = LOCATION::NONE;
        sequence = {};
    }
    PLAYER playerid{};
    POSITION position{};
    LOCATION location{};
    sequence_type sequence{};
};

struct card_state {
    SUBTYPE subtypes;
    uint32_t name{};
    uint32_t sub_name{};
    uint32_t horizontal{};
    uint32_t vertical{};
    sequence_type sequence{};
    uint32_t burst_sequence{};
    int32_t energy{};
    int32_t damage{};
    disciplines stats{};
    element_values elements{};
    int8_t mugic_ability{};
    int8_t mugic_counters{};
    TRIBE tribes{};
    TRIBE castable_mugic{};
    SUPERTYPE card_type{};
    FLAGS flags{};
    LOCATION location{};
    POSITION position{};
    PLAYER controller{};
    union {
        card* equipped_creature;
        card* battlegear;
    };
    enum BRAIN_WASH : uint8_t { NORMAL, ASSUME_NO_BRAINWASH, ASSUME_BRAINWASHED } pseudo_brainwash;
    REASON reason = REASON::NONE;
    effect* reason_effect{};
    PLAYER reason_player = PLAYER::NONE;

    void set0xff();
};

/*
 * attack_damage_received // intense polyphony
 * prev_attack_damage_received
 * won_initiative // allegro
 * treat_as_brainwashed // deep dirge, open deepmines
 * treat_as_not_brainwashed // minion's freesong, wlt'dred
 * moved_this_turn // hyper_hover
 * attack_damage_dealt // Improvisational Melody
 * won_stat_check
 * won_challenge // Melody of Miracles
 * won_stat_fail
 * engaged_in_combat // The Barracks
 *
 * first_attack // mondo rondo
 * to_be_banished // Lore High Muge of the Hive
 */

struct card_sort;
struct effect;

struct card : public lua_obj_helper<PARAM_TYPE_CARD> {
    using counter_map = std::map<uint16_t, std::array<uint16_t, 2>>;
    using effect_container = std::multimap<uint32_t, effect*>;
    using relation_map = std::unordered_map<card*, uint32_t>;

    card(match* pm);
    uint32_t cardid{};
    card_data data{};
    card_state previous{};
    card_state temp{};
    card_state current{};
    PLAYER owner{PLAYER::NONE};
    std::map<uint32_t, uint32_t> assume{};
    counter_map counters;
    effect_container single_effect;
    effect_container field_effect;
    effect_container battlegear_effect;
    effect_container target_effect;
    sendto_params sendto_param{};
    relation_map relations;

    uint32_t field_id{};
    uint32_t field_id_r{};
    uint16_t turn_id{};
    bool won_initiative{};
    bool won_stat_check{};
    bool won_challenge{};
    bool won_stat_fail{};
    bool won_combat{};
    uint32_t moved_this_turn{};
    uint32_t engaged_in_combat{};
    uint32_t attack_damage_received{};
    uint32_t attack_damage_delt{};
    uint32_t effect_damage_received{};
    uint32_t effect_damage_delt{};
    STATUS status;

    /*
3 1 0 0
3 1 0 0
2 2 0
2 2 0
2 2 0
*/

    loc_info get_location_info() { return {current.controller, current.location, current.sequence, current.position}; };

    [[nodiscard]] SUBTYPE get_subtypes() const { return current.subtypes; };
    [[nodiscard]] int32_t get_energy() const { return data.energy; };
    [[nodiscard]] int32_t damage() const { return current.damage; };
    [[nodiscard]] int16_t get_courage() const { return current.stats.courage; };
    [[nodiscard]] int16_t get_power() const { return current.stats.power; };
    [[nodiscard]] int16_t get_wisdom() const { return current.stats.wisdom; };
    [[nodiscard]] int16_t get_speed() const { return current.stats.speed; };
    [[nodiscard]] disciplines get_stats() const { return {get_courage(), get_power(), get_wisdom(), get_speed()}; }
    [[nodiscard]] int8_t get_fire() const { return data.elements.fire; };
    [[nodiscard]] int8_t get_air() const { return data.elements.air; };
    [[nodiscard]] int8_t get_earth() const { return data.elements.earth; };
    [[nodiscard]] int8_t get_water() const { return data.elements.water; };
    [[nodiscard]] element_values get_elements() const { return {get_fire(), get_air(), get_earth(), get_water()}; }
    [[nodiscard]] int8_t get_mugic_counters() const { return current.mugic_counters; };
    [[nodiscard]] TRIBE get_tribes() const { return current.tribes; };
    [[nodiscard]] TRIBE get_castable_mugic() const { return current.castable_mugic; };

    bool is_mirage() { return false; }
    bool is_position(POSITION pos) const { return is(current.position, pos); }
    bool is_destructible() { return true; }

    int32_t accumulate_effect(EFFECT effect_type) { return 0; }

    void set_status(STATUS status_to_toggle, int32_t enabled) {
        if (enabled)
            status |= status_to_toggle;
        else
            status &= ~status_to_toggle;
    }

    void apply_field_effect() {};
    void enable_field_effect(bool enabled = true) {};
    LOCATION leave_field_redirect(REASON reason) {
        //         fprintf(stderr, "enable_field_effect unimplemented\n");
        return {};
    };
    LOCATION destination_redirect(LOCATION dest, REASON reason) {
        //         fprintf(stderr, "destination_redirect unimplemented\n");
        return {};
    };
    void cancel_field_effect();

    void reset(RESET id, RESET type);
    void clear_related_effect();
    void refresh_negated_status();
    void equip(card* target, bool send_msg);
    void unequip();
    void clear_card_target();
    effect* is_affected_by_effect(EFFECT);
    bool is_affected_by_effect(effect*) { return true; };
    void filter_effect(EFFECT, effect_list&) {}

    std::pair<int32_t, uint8_t> calculate_attack_damage(card* source, card* target);
    static bool card_operation_sort(card* c1, card* c2);

    LOCATION grave_for() const {
        switch (data.supertype) {
            case SUPERTYPE::CREATURE:
            case SUPERTYPE::BATTLE_GEAR:
            case SUPERTYPE::MUGIC: return LOCATION::GENERAL_DISCARD;
            case SUPERTYPE::ATTACK: return LOCATION::ATTACK_DISCARD;
            default: return LOCATION::NONE;
        }
    }
};

#endif // CHAOTIC_CORE_CARD_H
