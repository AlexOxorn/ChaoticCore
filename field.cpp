//
// Created by alexoxorn on 8/21/22.
//

#include "field.h"
#include "internal_common.h"

#include "protocol_buffers/messages.pb.h"

#include "match.h"
#include <algorithm>
#include <list>

bool field::is_location_usable(SUPERTYPE card_type, PLAYER player_id, LOCATION location, sequence_type sequence) {
    if (!is(location, LOCATION::FIELD))
        return true;
    auto& pos = get_board_from_sequence(sequence);
    if (pos.blocked)
        return false;
    if (card_type == SUPERTYPE::LOCATION) {
        if (location == LOCATION::ACTIVE_LOCATION) {
            return active_location;
        }
        if (location == LOCATION::FIELD) {
            return std::none_of(board.begin(), board.end(), [](battle_board_location pos) { return pos.mirage; });
        }
        return true;
    }
    if (card_type == SUPERTYPE::CREATURE) {
        if (!pos.occupied())
            return true;
        if (!pos.creatures[+opp(player_id)] && !infos.combat_phase && infos.turn_phase == PHASE::ACTION_STEP
            && infos.turn_player == +player_id && !core.turn_engage_count)
            return true;
        return false;
    }
    if (card_type == SUPERTYPE::BATTLE_GEAR) {
        return pos.creatures[+player_id] && !pos.creatures[+player_id]->current.battlegear;
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
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::ATTACK_DISCARD:
            for (auto& pcard : player[(int) playerid].attack_grave)
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::ATTACK_HAND:
            for (auto& pcard : player[(int) playerid].attack_hand)
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::MUGIC_HAND:
            for (auto& pcard : player[(int) playerid].mugic_hand)
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::GENERAL_DISCARD:
            for (auto& pcard : player[(int) playerid].general_grave)
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::REMOVED:
            for (auto& pcard : player[(int) playerid].removed)
                pcard->current.sequence.index = i++;
            break;
        case LOCATION::BURST:
            for (auto& pcard : player[(int) playerid].general_grave)
                pcard->current.burst_sequence = i++;
            break;
        case LOCATION::LOCATION_DECK:
            for (auto& pcard : player[(int) playerid].location_deck)
                pcard->current.sequence.index = i++;
            break;
        default: break;
    }
}
/*
 * Moving Cards:
 * Draw()
 * Discard
 */
void field::remove_card(card* pcard) {
    if (pcard->current.controller == PLAYER::NONE || !pcard->current.location)
        return;

    auto playerid = +pcard->current.controller;
    LOCATION loc = pcard->current.location;
    if (is(loc, LOCATION::VECTOR_AREA)) {
        auto& list = player[playerid].get_list_for(pcard->current.location);
        list.erase(list.begin() + pcard->current.sequence.index);
        reset_sequence(pcard->current.controller, loc);
        if (is(loc, LOCATION::DECK) and !core.shuffle_check_disabled) {
            if (loc == LOCATION::ATTACK_DECK)
                core.shuffle_attack_deck_check[playerid] = true;
            else
                core.shuffle_location_deck_check[playerid] = true;
        }
    } else if (is(loc, LOCATION::FIELD)) {
        auto& board_pos = get_board_from_sequence(pcard->current.sequence);
        switch (pcard->data.supertype) {
            case SUPERTYPE::CREATURE: board_pos.creatures[playerid] = nullptr; break;
            case SUPERTYPE::BATTLE_GEAR:
                if (card* creature = board_pos.creatures[playerid]; creature) {
                    creature->previous.battlegear = creature->current.battlegear;
                    creature->current.battlegear = nullptr;
                }
                break;
            case SUPERTYPE::LOCATION: board_pos.mirage = nullptr; break;
            case SUPERTYPE::MUGIC:
            case SUPERTYPE::ATTACK: unreachable();
        }
    }
    pcard->cancel_field_effect();
    pcard->previous.controller = pcard->current.controller;
    pcard->previous.location = pcard->current.location;
    pcard->previous.sequence = pcard->current.sequence;
    pcard->previous.position = pcard->current.position;
    pcard->current.controller = PLAYER::NONE;
    pcard->current.location = LOCATION::NONE;
    pcard->current.sequence = sequence_type();
}
bool field::move_card(PLAYER controller, card* pcard, LOCATION location, sequence_type sequence, bool engage) {
    if (!is_location_usable(pcard->data.supertype, controller, location, sequence))
        return false;
    auto pre_player = pcard->current.controller;
    auto pre_sequence = pcard->current.sequence;
    if (+pcard->current.location) {
        if (pcard->current.location == location) {
            if (is(pcard->current.location, LOCATION::DECK)) {
                if (pre_player == controller) {
                    auto& list = pcard->current.location == LOCATION::ATTACK_DECK ? player[+pre_player].attack_deck
                                                                                  : player[+pre_player].location_deck;

                    auto& message = pmatch->new_message<MSG_Move>();
                    message.set_code(pcard->data.code);
                    write_location_info(message.mutable_previous_location(), pcard->get_location_info());

                    list.erase(list.begin() + pcard->current.sequence.index);
                    if (sequence.index == DECK_TOP) {
                        list.push_back(pcard);
                    } else if (sequence.index == DECK_BOTTOM) {
                        list.insert(list.begin(), pcard);
                    } else {
                        list.push_back(pcard);
                        if (!core.shuffle_check_disabled) {
                            auto& shuffle_check = pcard->current.location == LOCATION::ATTACK_DECK
                                                        ? core.shuffle_attack_deck_check[+controller]
                                                        : core.shuffle_location_deck_check[+controller];
                            shuffle_check = true;
                        }
                    }
                    reset_sequence(controller, location);
                    pcard->previous.controller = pre_player;
                    pcard->current.controller = controller;
                    write_location_info(message.mutable_new_location(), pcard->get_location_info());
                    message.set_reason(+pcard->current.reason);
                    return true;
                } else {
                    remove_card(pcard);
                }

            } else if (is(pcard->current.location, LOCATION::FIELD)) {
                if (controller == pre_player and sequence == pre_sequence)
                    return false;
                auto& old_battle_pos = get_board_from_sequence(pre_sequence);
                auto& board_pos = get_board_from_sequence(sequence);
                if (board_pos.blocked)
                    return false;

                MSG_Move* message = nullptr;
                if (pre_player == controller) {
                    message = &pmatch->new_message<MSG_Move>();
                    message->set_code(pcard->data.code);
                    write_location_info(message->mutable_previous_location(), pcard->get_location_info());
                }

                if (!core.current_burst.empty()) {
                    core.just_sent_cards.insert(pcard);
                }

                pcard->previous.controller = pcard->current.controller;
                pcard->previous.location = pcard->current.location;
                pcard->previous.sequence = pcard->current.sequence;
                pcard->previous.position = pcard->current.position;

                switch (pcard->data.supertype) {
                    case SUPERTYPE::CREATURE:
                        {
                            if (board_pos.creatures[+controller])
                                return false;
                            if (board_pos.creatures[1 - +controller] && not engage)
                                return false;

                            old_battle_pos.creatures[+pre_player] = nullptr;
                            board_pos.creatures[+controller] = pcard;
                            break;
                        };
                    case SUPERTYPE::BATTLE_GEAR:
                        {
                            if (!board_pos.creatures[+controller])
                                return false;
                            if (board_pos.creatures[+controller]->current.battlegear)
                                return false;

                            auto creature = board_pos.creatures[+controller];
                            creature->previous.battlegear = creature->current.battlegear;
                            creature->current.battlegear = pcard;
                            pcard->previous.equipped_creature = pcard->current.equipped_creature;
                            pcard->current.equipped_creature = creature;
                            break;
                        };
                    case SUPERTYPE::LOCATION:
                        {
                            if (board_pos.mirage)
                                return false;

                            old_battle_pos.mirage = nullptr;
                            board_pos.mirage = pcard;
                            break;
                        };
                    default: return false;
                }

                pcard->current.controller = controller;
                pcard->current.sequence = sequence;

                if (message) {
                    write_location_info(message->mutable_new_location(), pcard->get_location_info());
                    message->set_reason(+pcard->current.reason);
                } else {
                    pcard->field_id = infos.field_id++;
                }
                return true;
            } else if (is(location, LOCATION::HAND)) {
                if (pre_player == controller) {
                    return false;
                }
                remove_card(pcard);
            } else if (is(location, LOCATION::GRAVE | LOCATION::REMOVED)) {
                auto& message = pmatch->new_message<MSG_Move>();
                message.set_code(pcard->data.code);
                write_location_info(message.mutable_previous_location(), pcard->get_location_info());

                auto& cur_player = player[+controller];
                auto& list = is(location, LOCATION::REMOVED)        ? cur_player.removed
                           : is(location, LOCATION::ATTACK_DISCARD) ? cur_player.attack_grave
                                                                    : cur_player.general_grave;
                list.erase(list.begin() + pcard->current.sequence.index);
                list.push_back(pcard);
                reset_sequence(pcard->current.controller, location);

                write_location_info(message.mutable_previous_location(), pcard->get_location_info());
                message.set_reason(+pcard->current.reason);
            } else {
                // BURST T-T
                return false;
            }
        } else {
            remove_card(pcard);
        }
    }
    add_card(controller, pcard, location, sequence);
    return true;
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
        pcard->current.sequence.index = vec.size() - 1;
    };
    switch (location) {
        case LOCATION::ATTACK_DECK:
            {
                auto& attack_deck = player[(int) controller].attack_deck;
                if (sequence.index == DECK_TOP) {
                    push_back(attack_deck);
                } else if (sequence.index == DECK_BOTTOM) {
                    attack_deck.insert(attack_deck.begin(), pcard);
                    reset_sequence(controller, LOCATION::ATTACK_DECK);
                } else {
                    attack_deck.push_back(pcard);
                    pcard->current.sequence.index = attack_deck.size() - 1;
                    if (!core.shuffle_check_disabled)
                        core.shuffle_attack_deck_check[(int) controller] = true;
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
                            new_pos.creatures[+controller] = pcard;
                            pcard->current.sequence = sequence;
                            if (pcard->current.battlegear) {
                                pcard->current.battlegear->current.sequence = sequence;
                            }
                            break;
                        }
                    case SUPERTYPE::LOCATION:
                        {
                            new_pos.mirage = pcard;
                            pcard->current.sequence = sequence;
                            break;
                        }
                    case SUPERTYPE::BATTLE_GEAR:
                        {
                            if (card* creature = new_pos.creatures[+controller]; creature) {
                                pcard->equip(creature, false);
                                pcard->current.sequence = sequence;
                            }
                        }
                    default: break;
                }
                break;
            }
        case LOCATION::MUGIC_HAND:
            {
                push_back(player[(int) controller].mugic_hand);
                break;
            }
        case LOCATION::ATTACK_HAND:
            {
                push_back(player[(int) controller].attack_hand);
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
                if (sequence.index == DECK_TOP) {
                    push_back(location_deck);
                } else if (sequence.index == DECK_BOTTOM) {
                    location_deck.insert(location_deck.begin(), pcard);
                    reset_sequence(controller, LOCATION::ATTACK_DECK);
                } else {
                    location_deck.push_back(pcard);
                    pcard->current.sequence.index = location_deck.size() - 1;
                    if (!core.shuffle_check_disabled)
                        core.shuffle_location_deck_check[(int) controller] = true;
                }
                pcard->sendto_param.position = POSITION::FACE_DOWN;
                break;
            }
        case LOCATION::ACTIVE_LOCATION:
            {
                active_location = pcard;
                pcard->sendto_param.position = POSITION::FACE_UP;
                pcard->current.sequence = 0;
            }
        default: break;
    }
    pcard->apply_field_effect();
    pcard->field_id = infos.field_id++;
    pcard->field_id_r = pcard->field_id;
    pcard->turn_id = infos.turn_id;
}
void field::reveal_location(PLAYER playerid, effect* reason_effect, REASON reason, PLAYER reason_player) {
    emplace_process<procs::RevealLocation>(playerid, reason_effect, reason, reason_player);
}
void field::set_mirage(card* pcard, effect* reason_effect, REASON reason, PLAYER reason_player) {}

int32_t field::is_player_can_draw(uint8_t playerid) {
//     fprintf(stderr, "is_player_can_draw unimplemented");
    return true;
}

CHAOTIC_DuelStatus field::process() {
    std::move(core.subunits.rbegin(), core.subunits.rend(), std::front_inserter(core.units));
    core.subunits.clear();
    /*printf("\033[32mProcess: [\033[0m");
    for (const auto& x : core.units) {
        printf("\033[32m%s-%d, \033[0m",
               std::visit([](auto&&x)->decltype(auto){ return typeid(x); }, x).name(),
               std::visit([](auto&&x)->decltype(auto){ return x.step; }, x));
    }
    printf("\033[32m]\033[0m\n");*/
    if (core.units.empty())
        return CHAOTIC_DUEL_STATUS_END;
    return std::visit(*this, core.units.front());
}

void field::send_to(card_set targets, effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid,
                    LOCATION destination, uint32_t sequence, POSITION position, bool ignore) {
    if (is(destination, LOCATION::FIELD))
        return;
    for (auto& pcard : targets) {
        pcard->temp.reason = pcard->current.reason;
        pcard->temp.reason_effect = pcard->current.reason_effect;
        pcard->temp.reason_player = pcard->current.reason_player;
        pcard->current.reason = reason;
        pcard->current.reason_effect = reason_effect;
        pcard->current.reason_player = reason_player;
        PLAYER p = playerid;
        // send to hand from deck and playerid not given => send to the hand of controller
        if (p == PLAYER::NONE && (is(destination, LOCATION::HAND)) && (is(pcard->current.location, LOCATION::DECK))
            && pcard->current.controller == reason_player)
            p = reason_player;
        if (p == PLAYER::NONE)
            p = pcard->owner;
        if (destination == LOCATION::GENERAL_DISCARD && pcard->current.location == LOCATION::REMOVED)
            pcard->current.reason |= REASON::RETURN;
        auto pos = position;
        if (destination != LOCATION::REMOVED && !ignore)
            pos = POSITION::FACE_UP;
        else if (!position)
            pos = pcard->current.position;
        pcard->sendto_param.set(p, pos, destination, sequence);
    }
    group* ng = pmatch->new_group(std::move(targets));
    ng->is_readonly = true;
    emplace_process<procs::SendTo>(ng, reason_effect, reason, reason_player);
}

void field::send_to(card* target, effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid,
                    LOCATION destination, uint32_t sequence, POSITION position, bool ignore) {
    send_to(card_set{target}, reason_effect, reason, reason_player, playerid, destination, sequence, position, ignore);
}

void field::shuffle(PLAYER playerid, LOCATION location) {
    if (!is(location, LOCATION::DECK | LOCATION::HAND))
        return;

    card_vector* to_shuffle;

    switch (location) {
        case LOCATION::LOCATION_DECK: to_shuffle = &player[(int) playerid].location_deck; break;
        case LOCATION::ATTACK_DECK: to_shuffle = &player[(int) playerid].attack_deck; break;
        case LOCATION::ATTACK_HAND: to_shuffle = &player[(int) playerid].attack_hand; break;
        case LOCATION::MUGIC_HAND: to_shuffle = &player[(int) playerid].mugic_hand; break;
    }

    if (to_shuffle->empty())
        return;

    auto shuffle_impl1 = [&, this](auto& message) {
        std::shuffle(to_shuffle->begin(), to_shuffle->end(), pmatch->random);
        reset_sequence(playerid, location);
        message.set_playerid((int) playerid);
    };

    auto shuffle_impl2 = [&](auto& message) {
        shuffle_impl1(message);
        for (card* pcard : *to_shuffle) {
            message.add_codes(pcard->data.code);
        }
    };

    switch (location) {
        case LOCATION::ATTACK_DECK: shuffle_impl1(pmatch->new_message<MSG_ShuffleAttackDeck>()); break;
        case LOCATION::LOCATION_DECK: shuffle_impl1(pmatch->new_message<MSG_ShuffleLocationDeck>()); break;
        case LOCATION::ATTACK_HAND: shuffle_impl2(pmatch->new_message<MSG_ShuffleAttackHand>()); break;
        case LOCATION::MUGIC_HAND: shuffle_impl2(pmatch->new_message<MSG_ShuffleMugicHand>()); break;
    }
}

void field::destroy(card* target, effect* reason_effect, REASON reason, PLAYER reason_player) {
    destroy(card_set{target}, reason_effect, reason, reason_player);
}

void field::destroy(card_set targets, effect* reason_effect, REASON reason, PLAYER reason_player) {
    for (auto cit = targets.begin(); cit != targets.end();) {
        LOCATION destination;
        card* pcard = *cit;
        PLAYER p = pcard->owner;

        pcard->temp.reason = pcard->current.reason;
        pcard->current.reason = reason;
        pcard->temp.reason_effect = pcard->current.reason_effect;
        pcard->temp.reason_player = pcard->current.reason_player;
        if (reason_effect)
            pcard->current.reason_effect = reason_effect;
        pcard->current.reason_player = reason_player;
        destination = pcard->grave_for();
        if (!destination) {
            targets.erase(*cit++);
            continue;
        }
        pcard->set_status(STATUS::DESTROY_CONFIRMED, true);
        pcard->sendto_param.set(p, POSITION::FACE_UP, destination, 0);
        ++cit;
    }
    group* ng = pmatch->new_group(std::move(targets));
    ng->is_readonly = true;
    emplace_process<procs::Destroy>(ng, reason_effect, reason, reason_player);
}

card_set field::get_all_field_card() const {
    card_set to_ret;
    for (card* pcard : pmatch->cards) {
        if (pcard->current.location == LOCATION::FIELD)
            to_ret.insert(pcard);
    }
    return to_ret;
}