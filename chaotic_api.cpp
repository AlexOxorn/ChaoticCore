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
    printf("GET DESTROY DUEL HI\n");
    if (chaotic_duel)
        delete static_cast<match*>(chaotic_duel);
}

CHAOTICAPI void CHAOTIC_DuelNewCard(CHAOTIC_Duel chaotic_duel, CHAOTIC_NewCardInfo info) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    printf("GET NEW CARD %d\n", info.code);
    auto& game_field = *(pduel->game_field);
    if (game_field.is_location_usable(info.supertype, info.controller, info.location, info.sequence)) {
        card* pcard = pduel->new_card(info.code);
        pcard->owner = info.controller;
        pcard->current.position = info.position;
        game_field.add_card(info.controller, pcard, info.location, info.sequence);
        printf("creature %p and %p\n", game_field.board[0].creature1, game_field.board[0].creature2);
        printf("creature %p and %p\n", game_field.board[1].creature1, game_field.board[1].creature2);
        /*
         * Trigger field effects???
         */
    }
}

CHAOTICAPI void CHAOTIC_StartDuel(CHAOTIC_Duel chaotic_duel) {
    printf("GET START DUEL HA\n");
    auto* pduel = static_cast<match*>(chaotic_duel);
    //    pduel->game_field->emplace_process<Processors::Startup>();
}