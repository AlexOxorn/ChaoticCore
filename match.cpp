//
// Created by alexoxorn on 4/26/24.
//

#include "match.h"

#include <memory>
#include "field.h"
#include "effect.h"

match::match(const CHAOTIC_DuelOptions& options) :
        random(options.seed),
        read_card_callback(options.cardReader),
        read_script_callback(options.scriptReader),
        handle_message_callback(options.logHandler),
        read_card_done_callback(options.cardReaderDone),
        read_card_payload(options.cardReaderPayload),
        read_script_payload(options.scriptReaderPayload),
        handle_message_payload(options.logHandlerPayload),
        read_card_done_payload(options.cardReaderDonePayload)
{
    game_field = std::make_unique<field>(this, options);
}
match::~match() {
    for(auto& pcard : cards)
        delete pcard;
    for(auto& peffect : effects)
        delete peffect;
}

const card_data& match::read_card(uint32_t code) {
    if(auto search = data_cache.find(code); search != data_cache.end())
        return search->second;
    CHAOTIC_CardData data{};
    read_card_callback(read_card_payload, code, &data);
    auto ret = &(data_cache.emplace(code, data).first->second);
    read_card_done_callback(read_card_done_payload, &data);
    return *ret;
}

card* match::new_card(uint32_t code) {
    card* pcard = new card(this);
    cards.insert(pcard);
    if(code)
        pcard->data = read_card(code);
    pcard->data.code = code;
//    lua->register_card(pcard);
    return pcard;
}
