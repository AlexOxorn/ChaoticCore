//
// Created by alexoxorn on 4/29/24.
//

#include <cstdio>
#include <new>
#include "chaotic_api.h"
#include "match.h"
#include "field.h"
#include "protocol_buffers/query.pb.h"

CHAOTICAPI int CHAOTIC_CreateDuel(CHAOTIC_Duel* out_chaotic_duel, CHAOTIC_DuelOptions options) {
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
    printf("\033[31mNEW CARD %d\033[0m\n", info.code);
    auto* pmatch = static_cast<match*>(chaotic_duel);
    auto& game_field = *(pmatch->game_field);

    auto seq = std::bit_cast<sequence_type>(info.sequence);

    if (game_field.is_location_usable(info.supertype, info.controller, info.location, seq)) {
        card* pcard = pmatch->new_card(info.code);
        pcard->owner = info.controller;
        pcard->current.position = info.position;
        game_field.add_card(info.controller, pcard, info.location, seq);
        /*
         * Trigger field effects???
         */
    }
}

CHAOTICAPI void CHAOTIC_StartDuel(CHAOTIC_Duel chaotic_duel) {
    auto* pmatch = static_cast<match*>(chaotic_duel);
    pmatch->game_field->emplace_process<procs::Startup>();
}

CHAOTICAPI void CHAOTIC_PrintBoard(CHAOTIC_Duel chaotic_duel) {
    auto* pmatch = static_cast<match*>(chaotic_duel);
    pmatch->game_field->print_field();
}

CHAOTICAPI int CHAOTIC_DuelProcess(CHAOTIC_Duel chaotic_duel) {
    auto* pmatch = static_cast<match*>(chaotic_duel);
    pmatch->buff.clear();
    auto flag = CHAOTIC_DUEL_STATUS_END;
    do {
        flag = pmatch->game_field->process();
        pmatch->generate_buffer();
    } while (pmatch->buff.empty() && flag == CHAOTIC_DUEL_STATUS_CONTINUE);
    return flag;
}

CHAOTICAPI void* CHAOTIC_DuelGetMessage(CHAOTIC_Duel chaotic_duel, uint32_t* length) {
    auto* pmatch = static_cast<match*>(chaotic_duel);
    pmatch->generate_buffer();
    if (length)
        *length = static_cast<uint32_t>(pmatch->buff.size());
    return pmatch->buff.data();
}

CHAOTICAPI void CHAOTIC_DuelSetResponse(CHAOTIC_Duel chaotic_duel, const void* buffer, uint32_t length) {
    auto* p_match = static_cast<match*>(chaotic_duel);
    p_match->set_response(buffer, length);
}

CHAOTICAPI uint32_t CHAOTIC_DuelQueryCount(CHAOTIC_Duel chaotic_duel, PLAYER playerid, LOCATION loc) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    if (+playerid > 1)
        return 0;
    auto& player = pduel->game_field->player[+playerid];
    switch (loc) {
        case LOCATION::ATTACK_HAND: return player.attack_hand.size();
        case LOCATION::MUGIC_HAND: return player.mugic_hand.size();
        case LOCATION::ATTACK_DISCARD: return player.attack_grave.size();
        case LOCATION::GENERAL_DISCARD: return player.general_grave.size();
        case LOCATION::REMOVED: return player.removed.size();
        case LOCATION::LOCATION_DECK: return player.location_deck.size();
        default: break;
    }

    uint32_t count = 0;
    if (loc == LOCATION::FIELD) {
        for (auto& pcard : pduel->game_field->board) {
            if (!pcard.creatures[+playerid])
                continue;
            ++count;
            if (pcard.creatures[+playerid]->current.battlegear)
                ++count;
        }
    }
    return count;
}

CHAOTICAPI void* CHAOTIC_DuelQuery(CHAOTIC_Duel chaotic_duel, uint32_t* length, CHAOTIC_QueryInfo info) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    pduel->query_buffer.clear();
    card* pcard = nullptr;
    pcard = pduel->game_field->get_field_card(info.con, info.loc, std::bit_cast<sequence_type>(info.seq));
    if (pcard == nullptr) {
        if (length)
            *length = 0;
        return nullptr;
    }
    auto response = pcard->get_infos(info.flags);
    if (response.ByteSizeLong() > pduel->query_buffer.size())
        pduel->query_buffer.resize(response.GetCachedSize());
    if (length)
        *length = response.GetCachedSize();
    response.SerializeWithCachedSizesToArray(pduel->query_buffer.data());
    return pduel->query_buffer.data();
}

CHAOTICAPI void* CHAOTIC_DuelQueryLocation(CHAOTIC_Duel chaotic_duel, uint32_t* length, CHAOTIC_QueryInfo info) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    QUERY_CardList cards;
    auto& buffer = pduel->query_buffer;
    auto populate = [&flags = info.flags, &cards](const card_vector& list) {
        for (auto& pcard : list) {
            auto x = cards.mutable_cards()->add_cards();
            *x = pcard->get_infos(flags);
        }
    };
    auto populate_field = [&flags = info.flags, &cards](int x, int y, const battle_board_location& pos) {
        auto pos_q = cards.mutable_spaces()->add_spaces();
        pos_q->mutable_coordinate()->set_horizontal(x);
        pos_q->mutable_coordinate()->set_vertical(y);
        if (card* curr = pos.creatures[0]; curr) {
            *pos_q->mutable_player1_creature() = curr->get_infos(flags);
        }
        if (card* curr = pos.creatures[1]; curr) {
            *pos_q->mutable_player2_creature() = curr->get_infos(flags);
        }
        if (pos.mirage) {
            *pos_q->mutable_mirage() = pos.mirage->get_infos(flags);
        }
    };
    buffer.clear();
    if (+info.con <= 1u) {
        auto& player = pduel->game_field->player[+info.con];
        switch (info.loc) {
            case LOCATION::ATTACK_DECK: populate(player.attack_deck); break;
            case LOCATION::ATTACK_DISCARD: populate(player.attack_grave); break;
            case LOCATION::MUGIC_HAND: populate(player.mugic_hand); break;
            case LOCATION::ATTACK_HAND: populate(player.attack_hand); break;
            case LOCATION::GENERAL_DISCARD: populate(player.general_grave); break;
            case LOCATION::REMOVED: populate(player.removed); break;
            case LOCATION::LOCATION_DECK: populate(player.location_deck); break;
            case LOCATION::ACTIVE_LOCATION: populate({pduel->game_field->active_location}); break;
            case LOCATION::FIELD:
                {
                    for (int y = 0; y < pduel->game_field->board_size; ++y) {
                        for (int x = 0; x < 2 * pduel->game_field->board_size; ++x) {
                            auto pos = pduel->game_field->get_board_from_sequence({x, y});
                            if (pos.blocked)
                                continue;
                            if (pos.creatures[0] || pos.creatures[1] || pos.mirage)
                                populate_field(x, y, pos);
                        }
                    }
                }
            default: break;
        }
    }
    if (cards.ByteSizeLong() > pduel->query_buffer.size())
        pduel->query_buffer.resize(cards.GetCachedSize());
    cards.SerializeWithCachedSizesToArray(pduel->query_buffer.data());
    if (length)
        *length = cards.GetCachedSize();
    return buffer.data();
}

CHAOTICAPI void* CHAOTIC_DuelQueryField(CHAOTIC_Duel chaotic_duel, uint32_t* length) {
    auto* pduel = static_cast<match*>(chaotic_duel);
    QUERY_FieldData query_data;
    pduel->query_buffer.clear();
    query_data.set_options(0);

    for (int y = 0; y < pduel->game_field->board_size; ++y) {
        for (int x = 0; x < 2 * pduel->game_field->board_size; ++x) {
            auto pos = pduel->game_field->get_board_from_sequence({x, y});
            if (pos.blocked)
                continue;
            if (!(pos.creatures[0] || pos.creatures[1] || pos.mirage))
                continue;
            auto pos_query = query_data.add_field();
            pos_query->mutable_coordinate()->set_horizontal(x);
            pos_query->mutable_coordinate()->set_vertical(y);
            pos_query->set_player1_creature(pos.creatures[0]);
            if (pos.creatures[0] && pos.creatures[0]->current.battlegear) {
                pos_query->set_player1_battlegear(+pos.creatures[0]->current.battlegear->current.position);
            }
            pos_query->set_player2_creature(pos.creatures[1]);
            if (pos.creatures[1] && pos.creatures[1]->current.battlegear) {
                pos_query->set_player2_battlegear(+pos.creatures[1]->current.battlegear->current.position);
            }
            pos_query->set_mirage(pos.mirage);
        }
    }
    using mutable_count_methods = QUERY_FieldData_PlayerData* (QUERY_FieldData::*) ();

    mutable_count_methods counts[] = {&QUERY_FieldData::mutable_player1_counts,
                                      &QUERY_FieldData::mutable_player2_counts};

    for (int playerid = 0; playerid < 2; ++playerid) {
        auto& player = pduel->game_field->player[playerid];
        auto x = (query_data.*counts[playerid])();
        x->set_attack_hand_size((int)player.attack_hand.size());
        x->set_mugic_hand_size((int)player.mugic_hand.size());
        x->set_attack_deck_size((int)player.attack_deck.size());
        x->set_location_deck_size((int)player.location_deck.size());
        x->set_attack_grave_size((int)player.attack_grave.size());
        x->set_general_grave_size((int)player.general_grave.size());
        x->set_removed_size((int)player.removed.size());
    }

    for (const auto& ch : pduel->game_field->core.current_burst) {
        //        effect* peffect = ch.triggering_effect;
        //        insert_value<uint32_t>(query, peffect->get_handler()->data.code);
        //        loc_info info = peffect->get_handler()->get_info_location();
        //        insert_value<uint8_t>(query, info.controler);
        //        insert_value<uint8_t>(query, info.location);
        //        insert_value<uint32_t>(query, info.sequence);
        //        insert_value<uint32_t>(query, info.position);
        //        insert_value<uint8_t>(query, ch.triggering_controler);
        //        insert_value<uint8_t>(query, static_cast<uint8_t>(ch.triggering_location));
        //        insert_value<uint32_t>(query, ch.triggering_sequence);
        //        insert_value<uint64_t>(query, peffect->description);
    }

    if (query_data.ByteSizeLong() > pduel->query_buffer.size())
        pduel->query_buffer.resize(query_data.GetCachedSize());
    query_data.SerializeWithCachedSizesToArray(pduel->query_buffer.data());
    if (length)
        *length = query_data.GetCachedSize();
    return pduel->query_buffer.data();
}
