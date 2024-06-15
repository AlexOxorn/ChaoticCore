//
// Created by alexoxorn on 4/26/24.
//

#ifndef CHAOTIC_CORE_MATCH_H
#define CHAOTIC_CORE_MATCH_H

#include "common.h"
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <random>
#include <variant>
#include <deque>
#include "protocol_buffers/messages.pb.h"
#include "card.h"
#include "group.h"
#include "chaotic_api.h"

class field;
class interpreter;
class card;
class group;
class effect;

template <typename T>
using pointer_set = std::unordered_set<T*>;
// using pointer_set = std::vector<std::unique_ptr<T>>;

struct message
        : public std::variant<MSG_ShuffleAttackDeck, MSG_ShuffleLocationDeck, MSG_ShuffleAttackHand,
                              MSG_ShuffleMugicHand, MSG_Move, MSG_Draw, MSG_NewTurn, MSG_NewPhase, MSG_ActivateLocation,
                              MSG_SelectMove, MSG_Retry, MSG_CreatureMove, MSG_CombatStart, MSG_SelectAttackCard,
                              MSG_Damage, MSG_WonInitiative, MSG_StrikerChange, MSG_CombatEnd, MSG_Recover> {
    using variant::variant;

    uint32_t size() {
        return std::visit([](const auto& mes) { return mes.ByteSizeLong(); }, *this);
    }

    std::string date() {
        return std::visit([](const auto& mes) { return mes.SerializeAsString(); }, *this);
    };
};

class match {
public:
    std::mt19937 random;
    std::vector<uint8_t> query_buffer;
    std::deque<message> messages;

    std::vector<uint8_t> buff;
    std::unique_ptr<field> game_field;
    pointer_set<card> cards;
    pointer_set<effect> effects;
    pointer_set<group> groups;

    template <typename ProtoBuf>
    ProtoBuf& new_message() {
        return std::get<ProtoBuf>(messages.emplace_back(std::in_place_type<ProtoBuf>));
    }

    std::unordered_map<uint32_t, card_data> data_cache;

    CHAOTIC_DataReader read_card_callback;
    CHAOTIC_ScriptReader read_script_callback;
    CHAOTIC_LogHandler handle_message_callback;
    CHAOTIC_DataReaderDone read_card_done_callback;
    void* read_card_payload;
    void* read_script_payload;
    void* handle_message_payload;
    void* read_card_done_payload;

    void temp_register_card(card* pcard);

    card* new_card(uint32_t code);
    match(const CHAOTIC_DuelOptions& options);
    ~match();
    const card_data& read_card(uint32_t code);

    template <typename... Args>
    [[nodiscard]] group* new_group(Args&&... args) {
        auto* pgroup = new group(this, std::forward<Args>(args)...);
        groups.insert(pgroup);
        return pgroup;
    }
    void delete_group(group* targets) {
        groups.erase(targets);
        delete targets;
    }
    void delete_effect(effect* peffect) {
        effects.erase(peffect);
        delete peffect;
    };
    void generate_buffer();
    void write_buffer(const void* data, size_t size);
    void set_response(const void* p_void, uint32_t i);
};

#endif // CHAOTIC_CORE_MATCH_H
