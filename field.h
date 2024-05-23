//
// Created by alexoxorn on 8/21/22.
//

#ifndef CHAOTIC_CORE_FIELD_H
#define CHAOTIC_CORE_FIELD_H

#include "card.h"
#include "chaotic_api.h"
#include "process_units.h"
#include "effect_constants.h"
#include <array>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <optional>

class match;
class group;
class effect;

struct battle_board_location {
    std::array<card*, 2> creatures{};
    card* mirage = nullptr;
    bool blocked = false;

    int occupied() {
        return (int) std::count_if(creatures.begin(), creatures.end(), [](card* a) { return a; });
    }
};

using battle_board = std::vector<battle_board_location>;

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
    uint8_t triggering_controller;
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
    card_vector attack_hand;
    card_vector mugic_hand;
    card_vector location_deck;
    card_vector removed;

    card_vector& get_list_for(LOCATION location) {
        switch (location) {
            case LOCATION::ATTACK_DECK: return attack_deck;
            case LOCATION::ATTACK_DISCARD: return attack_grave;
            case LOCATION::MUGIC_HAND: return mugic_hand;
            case LOCATION::ATTACK_HAND: return attack_hand;
            case LOCATION::GENERAL_DISCARD: return general_grave;
            case LOCATION::REMOVED: return removed;
            case LOCATION::LOCATION_DECK: return location_deck;
            default: unreachable();
        }
    }

    uint32_t attack_hand_count = 2;
};

using processors::processor_unit;

struct processor {
    using processor_list = std::deque<processor_unit>;
    processor_list units;
    processor_list subunits;

    card_set operated_set;

    bool shuffle_check_disabled;
    bool shuffle_attack_hand_check[2];
    bool shuffle_mugic_hand_check[2];
    bool shuffle_attack_deck_check[2];
    bool shuffle_location_deck_check[2];

    bool can_engage;
    burst_array current_burst;
    card_set just_sent_cards;
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

class progressive_buffer {
public:
    std::vector<uint8_t> data;
    void clear() { data.clear(); }
    template <class T>
    T at(const size_t pos) const {
        constexpr static auto valsize = sizeof(T);
        size_t size = (pos + 1) * valsize;
        T ret{};
        if (data.size() < size)
            return ret;
        std::memcpy(&ret, data.data() + pos * valsize, sizeof(T));
        return ret;
    }
    template <class T>
    void set(const size_t pos, T val) {
        constexpr static auto valsize = sizeof(T);
        size_t size = (pos + 1) * valsize;
        if (data.size() < size)
            data.resize(size);
        std::memcpy(data.data() + pos * valsize, &val, sizeof(T));
    }
    [[nodiscard]] bool bitGet(const size_t pos) const {
        size_t real_pos = pos / 8u;
        uint32_t index = pos % 8u;
        size_t size = real_pos + 1;
        if (data.size() < size)
            return false;
        return !!(data[real_pos] & (1 << index));
    }
    void bitToggle(const size_t pos, bool set) {
        size_t real_pos = pos / 8u;
        uint32_t index = pos % 8u;
        size_t size = real_pos + 1;
        if (data.size() < size) {
            data.resize(size);
        }
        if (set)
            data[real_pos] |= (1 << index);
        else {
            data[real_pos] &= ~(1 << index);
        }
    }
};

class field {
public:
    match* pmatch;
    std::array<player_info, 2> player;
    uint32_t board_size{3};
    battle_board board;
    card* temp_card{};
    processor core{};
    field_info infos;

    progressive_buffer returns;

    explicit field(match* pmatch, const CHAOTIC_DuelOptions& options) :
            pmatch(pmatch), board_size(options.columns), board(board_size * board_size * 2 + 1){};
    bool is_location_usable(SUPERTYPE card_type, PLAYER player_id, LOCATION location, sequence_type sequence);
    bool move_card(PLAYER controller, card* pcard, LOCATION location, sequence_type sequence, bool engage = false);
    void add_card(PLAYER controller, card* pcard, LOCATION, sequence_type);
    void remove_card(card* pcard);
    void reset_sequence(PLAYER playerid, LOCATION location);
    void refresh_attack_deck(PLAYER playerid);
    void shuffle(PLAYER playerid, LOCATION location);

    battle_board_location& get_board_from_sequence(sequence_type sequence) {
        auto index = sequence.vertical * 2 * board_size + sequence.horizontal;
        if (index >= board.size())
            return board.back();
        return board[index];
    }

    int32_t is_player_can_draw(uint8_t playerid);

    struct Step {
        uint16_t step;
    };
    template <typename T, typename... Args>
    constexpr inline void emplace_process(Step step, Args&&... args) {
        processors::emplace_variant<T>(core.subunits, step.step, std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
    constexpr inline void emplace_process(Args&&... args) {
        emplace_process<T>(Step{0}, std::forward<Args>(args)...);
    }

    bool raise_event(card* event_card, EVENT event_code, effect* reason_effect, REASON reason, PLAYER event_player,
                     uint32_t event_value);
    bool raise_event(card_set event_cards, EVENT event_code, effect* reason_effect, REASON reason, PLAYER reason_player,
                     PLAYER event_player, uint32_t event_value);
    bool raise_single_event(card* trigger_card, card_set* event_cards, EVENT event_code, effect* reason_effect,
                            REASON reason, PLAYER reason_player, PLAYER event_player, uint32_t event_value);
    int32_t process_instant_event();
    int32_t process_single_event();

    bool process(processors::Startup& arg);
    bool process(processors::Draw& arg);
    bool process(processors::RevealBattlegear&);
    bool process(processors::Debug&);

    template <typename... Args>
    CHAOTIC_DuelStatus operator()(std::variant<Args...>& arg) {
        return std::visit(*this, arg);
    }

    template <typename T>
    CHAOTIC_DuelStatus operator()(T& arg) {
        if (process(arg)) {
            core.units.pop_front();
            return CHAOTIC_DUEL_STATUS_CONTINUE;
        } else {
            ++arg.step;
            return processors::needs_answer<T> ? CHAOTIC_DUEL_STATUS_AWAITING : CHAOTIC_DUEL_STATUS_CONTINUE;
        }
    }
    CHAOTIC_DuelStatus process();
    void draw(effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid, uint16_t count);
    void send_to(card_set targets, effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid,
                 LOCATION destination, uint32_t sequence, POSITION position, bool ignore);
    bool process(processors::SendTo& arg);
    void adjust_instant();
    void adjust_disable_check_list();
    void adjust_self_destroy_set();
    bool process(processors::Turn& arg);
};

#endif // CHAOTIC_CORE_FIELD_H
