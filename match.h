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
#include "card.h"
#include "chaotic_api.h"

class field;
class interpreter;
class card;
class group;
class effect;

template <typename T>
using pointer_set = std::unordered_set<T*>;
// using pointer_set = std::vector<std::unique_ptr<T>>;

class match {
    std::mt19937 random;
    std::vector<uint8_t> buff;
    std::vector<uint8_t> query_buffer;
public:
    std::unique_ptr<field> game_field;
    pointer_set<card> cards;
    pointer_set<effect> effects;

    std::unordered_map<uint32_t, card_data> data_cache;

    CHAOTIC_DataReader read_card_callback;
    CHAOTIC_ScriptReader read_script_callback;
    CHAOTIC_LogHandler handle_message_callback;
    CHAOTIC_DataReaderDone read_card_done_callback;
    void* read_card_payload;
    void* read_script_payload;
    void* handle_message_payload;
    void* read_card_done_payload;

    card* new_card(uint32_t code);
public:
    match(const CHAOTIC_DuelOptions& options);
    ~match();
    const card_data& read_card(uint32_t code);
};

#endif // CHAOTIC_CORE_MATCH_H
