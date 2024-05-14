//
// Created by alexoxorn on 8/21/22.
//

#ifndef CHAOTIC_CORE_FIELD_H
#define CHAOTIC_CORE_FIELD_H

#include "card.h"
#include "chaotic_api.h"
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

class match;
class group;
class effect;

struct battle_board_location {
    card* creature1 = nullptr;
    card* creature2 = nullptr;
    card* mirage = nullptr;
    bool blocked = false;
};

using battle_board = std::vector<battle_board_location>;
using card_vector = std::vector<card*>;
using coordinates = std::pair<uint32_t, uint32_t>;

struct tevent {
    card* trigger_card;
    group* event_cards;
    effect* reason_effect;
    uint32_t event_code;
    uint32_t event_value;
    uint32_t reason;
    uint8_t event_player;
    uint8_t reason_player;
    uint32_t global_id;
    bool operator<(const tevent& v) const { return this->global_id < v.global_id; };
};

struct optarget {
    group* op_cards;
    uint8_t op_count;
    uint8_t op_player;
    int32_t op_param;
};

struct burst {
    using opmap = std::unordered_map<uint32_t, optarget>;
    uint16_t burst_id;
    uint8_t burst_count;
    uint8_t triggering_player;
    uint8_t triggering_controler;
    uint16_t triggering_location;
    uint32_t triggering_sequence;
    uint8_t triggering_position;
    card_state triggering_state;
    effect* triggering_effect;
    group* target_cards;
    int32_t replace_op;
    uint8_t target_player;
    int32_t target_param;
    effect* disable_reason;
    uint8_t disable_player;
    tevent evt;
    opmap opinfos;
    opmap possibleopinfos;
    uint32_t flag;
    uint32_t event_id;
    bool was_just_sent{false};
    using applied_burst_counter_t = std::vector<uint32_t>;
    applied_burst_counter_t* applied_burst_counters;
    static bool burst_operation_sort(const burst& c1, const burst& c2);
    void set_triggering_state(card* pcard);
};

struct player_info {
    card_vector attack_deck;
    card_vector attack_grave;
    card_vector general_grave;
    card_vector hand;
    card_vector location_deck;
    card_vector removed;
};


struct processor {

    bool shuffle_check_disabled;
    bool shuffle_hand_check[2];
    bool shuffle_deck_check[2];

    bool can_engage;
};

struct field_info {
    uint32_t event_id{1};
    uint32_t field_id{1};
    uint16_t copy_id{1};
    int16_t turn_id{0};
    int16_t turn_id_by_player[2]{0, 0};
    uint32_t card_id{1};
    TURN_PHASE turn_phase{0};
    COMBAT_PHASE combat_phase{0};
    uint8_t turn_player{0};
    uint8_t priorities[2]{0, 0};
    bool can_shuffle{true};
};

class field {
    match* pmatch;
public:
    std::array<player_info, 2> player;
    uint32_t board_size{3};
    battle_board board;
    card* temp_card{};
    processor core{};
    field_info infos;
    explicit field(match* pmatch, const CHAOTIC_DuelOptions& options) :
            pmatch(pmatch), board_size(options.columns), board(board_size * board_size * 2){};
    bool is_location_usable(SUPERTYPE card_type, PLAYER player_id, LOCATION location, sequence_type sequence);
    void add_card(PLAYER controller, card* pcard, LOCATION, sequence_type);
    void reset_sequence(PLAYER playerid, LOCATION location);
};

#endif // CHAOTIC_CORE_FIELD_H
