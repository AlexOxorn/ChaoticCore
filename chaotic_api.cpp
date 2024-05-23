//
// Created by alexoxorn on 4/29/24.
//

#include <cstdio>
#include <new>
#include "chaotic_api.h"
#include "match.h"
#include "field.h"

CHAOTICAPI int CHAOTIC_CreateDuel(CHAOTIC_Duel* out_chaotic_duel, CHAOTIC_DuelOptions options) {
    printf("GET CREATE DUEL\n");
    //    if(out_chaotic_duel == nullptr)
    //        return CHAOTIC_DUEL_CREATION_NO_OUTPUT;
    //    if(options.cardReader == nullptr) {
    //        *out_chaotic_duel = nullptr;
    //        return CHAOTIC_DUEL_CREATION_NULL_DATA_READER;
    //    }
    //    if(options.scriptReader == nullptr) {
    //        *out_chaotic_duel = nullptr;
    //        return CHAOTIC_DUEL_CREATION_NULL_SCRIPT_READER;
    //    }
    if (options.logHandler == nullptr) {
        options.logHandler = [](void* /*payload*/, const char* /*string*/, int /*type*/) {
        };
        options.logHandlerPayload = nullptr;
    }
    if (options.cardReaderDone == nullptr) {
        options.cardReaderDone = [](void* /*payload*/, CHAOTIC_CardData* /*data*/) {
        };
        options.cardReaderDonePayload = nullptr;
    }
    auto* duelPtr = new (std::nothrow) match(options);
    if (duelPtr == nullptr)
        return -1;
    *out_chaotic_duel = static_cast<CHAOTIC_Duel>(duelPtr);
    return 0;
}

CHAOTICAPI void CHAOTIC_DestroyDuel(CHAOTIC_Duel chaotic_duel) {
    if (chaotic_duel)
        delete static_cast<match*>(chaotic_duel);
}

CHAOTICAPI void CHAOTIC_DuelNewCard(CHAOTIC_Duel chaotic_duel, CHAOTIC_NewCardInfo info) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    auto& game_field = *(pduel->game_field);
    if (game_field.is_location_usable(info.supertype, info.controller, info.location, info.sequence)) {
        card* pcard = pduel->new_card(info.code);
        pcard->owner = info.controller;
        pcard->current.position = info.position;
        game_field.add_card(info.controller, pcard, info.location, info.sequence);
        /*
         * Trigger field effects???
         */
    }
}

CHAOTICAPI void CHAOTIC_StartDuel(CHAOTIC_Duel chaotic_duel) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    pduel->game_field->emplace_process<processors::Startup>();
}

CHAOTICAPI int CHAOTIC_DuelProcess(CHAOTIC_Duel chaotic_duel) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    pduel->buff.clear();
    auto flag = CHAOTIC_DUEL_STATUS_END;
    do {
        flag = pduel->game_field->process();
        pduel->generate_buffer();
    } while(pduel->buff.empty() && flag == CHAOTIC_DUEL_STATUS_CONTINUE);
    return flag;
}

CHAOTICAPI void* CHAOTIC_DuelGetMessage(CHAOTIC_Duel chaotic_duel, uint32_t* length) {
    auto* pmatch = static_cast<match*>(chaotic_duel);
    pmatch->generate_buffer();
    if(length)
        *length = static_cast<uint32_t>(pmatch->buff.size());
    return pmatch->buff.data();
}

CHAOTICAPI void CHAOTIC_DuelSetResponse(CHAOTIC_Duel chaotic_duel, const void* buffer, uint32_t length) {
    auto* p_match = static_cast<match*>(chaotic_duel);
    p_match->set_response(buffer, length);
}
