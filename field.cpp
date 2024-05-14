//
// Created by alexoxorn on 8/21/22.
//

#include "field.h"
#include "internal_common.h"
#include <algorithm>

bool field::is_location_usable(SUPERTYPE card_type, PLAYER player_id, LOCATION location, sequence_type sequence) {
    if (!is(location, LOCATION::FIELD))
        return true;
    auto index = sequence.vertical * 2 * board_size + sequence.horizontal;
    auto& pos = board[index];
    if (pos.blocked)
        return false;
    if (card_type == SUPERTYPE::LOCATION) {
        return true;
    }
    if (card_type == SUPERTYPE::CREATURE) {
        if (pos.creature1 == nullptr)
            return true;
        return !pos.creature2 && core.can_engage && !infos.combat_phase;
    }
    if (card_type == SUPERTYPE::BATTLE_GEAR) {
        if (sequence.sequence == 0) {
            return pos.creature1 && !pos.creature1->battlegear;
        } else {
            return pos.creature2 && !pos.creature2->battlegear;
        }
    }
    return true;
}
void field::reset_sequence(PLAYER playerid, LOCATION location) {
    if (is(location, LOCATION::FIELD))
        return;
    uint32_t i = 0;
    switch (location) {
        case LOCATION::ATTACK_DECK:
            for (auto& pcard : player[(int) playerid].attack_deck)
                pcard->current.sequence.sequence = i++;
            break;
        case LOCATION::ATTACK_DISCARD:
            for (auto& pcard : player[(int) playerid].attack_grave)
                pcard->current.sequence.sequence = i++;
            break;
        case LOCATION::HAND:
            for (auto& pcard : player[(int) playerid].hand)
                pcard->current.sequence.sequence = i++;
            break;
        case LOCATION::GENERAL_DISCARD:
            for (auto& pcard : player[(int) playerid].general_grave)
                pcard->current.sequence.sequence = i++;
            break;
        case LOCATION::REMOVED:
            for (auto& pcard : player[(int) playerid].removed)
                pcard->current.sequence.sequence = i++;
            break;
        case LOCATION::BURST:
            for (auto& pcard : player[(int) playerid].general_grave)
                pcard->current.burst_sequence = i++;
            break;
        case LOCATION::LOCATION_DECK:
            for (auto& pcard : player[(int) playerid].location_deck)
                pcard->current.sequence.sequence = i++;
            break;
    }
}
void field::add_card(PLAYER controller, card* pcard, LOCATION location, sequence_type sequence) {
    if (+pcard->current.location)
        return;
    if (!is_location_usable(pcard->data.supertype, controller, location, sequence)) {
        return;
    }
    pcard->current.controller = controller;
    pcard->current.location = location;
    auto push_back = [pcard](card_vector& vec) {
        vec.push_back(pcard);
        pcard->current.sequence.sequence = vec.size() - 1;
    };
    switch (location) {
        case LOCATION::ATTACK_DECK:
            {
                auto& attack_deck = player[(int) controller].attack_deck;
                if (sequence.sequence == DECK_TOP) {
                    push_back(attack_deck);
                } else if (sequence.sequence == DECK_BOTTOM) {
                    attack_deck.insert(attack_deck.begin(), pcard);
                    reset_sequence(controller, LOCATION::ATTACK_DECK);
                } else {
                    attack_deck.push_back(pcard);
                    pcard->current.sequence.sequence = attack_deck.size() - 1;
                    if (!core.shuffle_check_disabled)
                        core.shuffle_deck_check[(int) controller] = true;
                }
                pcard->sendto_param.position = POSITION::FACE_DOWN;
                break;
            }
        case LOCATION::ATTACK_DISCARD:
            {
                push_back(player[(int) controller].attack_grave);
                break;
            }
        case LOCATION::FIELD:
            {
                uint32_t index = sequence.vertical * board_size * 2 + sequence.horizontal;
                battle_board_location& new_pos = board[index];
                switch (pcard->data.supertype) {
                    case SUPERTYPE::CREATURE:
                        {
                            if (!new_pos.creature1) {
                                new_pos.creature1 = pcard;
                                pcard->current.sequence = sequence;
                                pcard->current.sequence.sequence = 0;
                                if (pcard->battlegear) {
                                    pcard->battlegear->current.sequence = sequence;
                                    pcard->battlegear->current.sequence.sequence = 2;
                                }
                            } else {
                                new_pos.creature2 = pcard;
                                pcard->current.sequence = sequence;
                                pcard->current.sequence.sequence = 1;
                                if (pcard->battlegear) {
                                    pcard->battlegear->current.sequence = sequence;
                                    pcard->battlegear->current.sequence.sequence = 3;
                                }
                            }
                            break;
                        }
                    case SUPERTYPE::LOCATION:
                        {
                            auto prev_mirage_itr = std::find_if(
                                    board.begin(), board.end(), [](battle_board_location pos) { return pos.mirage; });
                            if (prev_mirage_itr != board.end()) {
                                throw unimplemented("Move old mirage to location deck");
                            }
                            new_pos.mirage = pcard;
                            pcard->current.sequence = sequence;
                            pcard->current.sequence.sequence = 4;
                            break;
                        }
                    case SUPERTYPE::BATTLE_GEAR: {
                            if (sequence.sequence == 0) {
                                new_pos.creature1->battlegear = pcard;
                                pcard->current.sequence = sequence;
                                pcard->current.sequence.sequence = 2;
                            } else {
                                new_pos.creature2->battlegear = pcard;
                                pcard->current.sequence = sequence;
                                pcard->current.sequence.sequence = 3;
                            }
                        }
                }
                if (pcard->data.supertype == SUPERTYPE::CREATURE) {}
            }
        case LOCATION::HAND:
            {
                auto& hand = player[(int) controller].hand;
                push_back(player[(int) controller].hand);
                std::stable_sort(hand.begin(), hand.end(), [](card* l, card* r) {
                    return l->data.supertype < r->data.supertype;
                });
                reset_sequence(controller, LOCATION::HAND);
                break;
            }
        case LOCATION::GENERAL_DISCARD:
            {
                push_back(player[(int) controller].general_grave);
                break;
            }
        case LOCATION::REMOVED:
            {
                push_back(player[(int) controller].removed);
                break;
            }
        case LOCATION::LOCATION_DECK:
            {
                auto& location_deck = player[(int) controller].location_deck;
                if (sequence.sequence == DECK_TOP) {
                    push_back(location_deck);
                } else if (sequence.sequence == DECK_BOTTOM) {
                    location_deck.insert(location_deck.begin(), pcard);
                    reset_sequence(controller, LOCATION::ATTACK_DECK);
                } else {
                    location_deck.push_back(pcard);
                    pcard->current.sequence.sequence = location_deck.size() - 1;
                    if (!core.shuffle_check_disabled)
                        core.shuffle_deck_check[(int) controller] = true;
                }
                pcard->sendto_param.position = POSITION::FACE_DOWN;
                break;
            }
    }
    pcard->apply_field_effect();
    pcard->field_id = infos.field_id++;
    pcard->field_id_r = pcard->field_id;
    pcard->turn_id = infos.turn_id;
}
