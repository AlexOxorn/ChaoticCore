//
// Created by alexoxorn on 5/14/24.
//

#include "field.h"
#include "effect_constants.h"
#include "internal_common.h"
#include "match.h"

bool field::raise_event(card* event_card, EVENT event_code, effect* reason_effect, REASON reason, PLAYER event_player,
                        uint32_t event_value) {
    return false;
}
bool field::raise_event(card_set event_cards, EVENT event_code, effect* reason_effect, REASON reason,
                        PLAYER reason_player, PLAYER event_player, uint32_t event_value) {
    return false;
}
bool field::raise_single_event(card* trigger_card, card_set* event_cards, EVENT event_code, effect* reason_effect,
                               REASON reason, PLAYER reason_player, PLAYER event_player, uint32_t event_value) {
    return false;
}
int32_t field::process_single_event() {
    fprintf(stderr, "field::process_single_event unimplemented\n");
    return 0;
}
int32_t field::process_instant_event() {
    fprintf(stderr, "field::process_single_event unimplemented\n");
    return 0;
}
void field::adjust_instant() {
    ++infos.event_id;
    adjust_disable_check_list();
    adjust_self_destroy_set();
}

void field::adjust_disable_check_list() {
    fprintf(stderr, "field::process_single_event unimplemented\n");
}
void field::adjust_self_destroy_set() {
    fprintf(stderr, "field::process_single_event unimplemented\n");
}

void field::draw(effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid, uint16_t count) {
    emplace_process<processors::Draw>(reason_effect, reason, reason_player, playerid, count);
}

void field::refresh_attack_deck(PLAYER playerid) {
    auto& grave = player[+playerid].attack_grave;
    card_set grave_set(grave.begin(), grave.end());
    if (grave_set.empty()) {
        return;
    }
    send_to(grave_set,
            nullptr,
            REASON::RULE,
            PLAYER::NONE,
            playerid,
            LOCATION::ATTACK_DECK,
            DECK_SHUFFLE,
            POSITION::FACE_DOWN,
            false);
}

bool field::process(processors::Startup& arg) {
    switch (arg.step) {
        case 0:
            {
                core.shuffle_attack_hand_check[0] = false;
                core.shuffle_attack_hand_check[1] = false;
                core.shuffle_mugic_hand_check[0] = false;
                core.shuffle_mugic_hand_check[1] = false;
                core.shuffle_attack_deck_check[0] = false;
                core.shuffle_attack_deck_check[1] = false;
                core.shuffle_location_deck_check[0] = false;
                core.shuffle_location_deck_check[1] = false;
                raise_event(nullptr, EVENT::STARTUP, nullptr, REASON::NONE, PLAYER::NONE, 0);
                process_instant_event();
                return false;
            }
        case 1:
            {
                for (int p = 0; p < 2; p++) {
                    core.shuffle_attack_hand_check[p] = false;
                    core.shuffle_mugic_hand_check[p] = false;
                    core.shuffle_attack_deck_check[p] = false;
                    core.shuffle_location_deck_check[p] = false;
                    if (player[p].attack_hand_count > 0)
                        draw(nullptr, REASON::RULE, PLAYER::NONE, static_cast<PLAYER>(p), player[p].attack_hand_count);
                }
                //                emplace_process<Processors::Turn>(0);
                return false;
            }
        case 2:
            {
                for (auto& pos : board) {
                    if (pos.blocked)
                        continue;
                    for (int p = 0; p < 2; p++) {
                        card* creature = pos.creatures[p];
                        card* battle_gear = creature ? creature->current.battlegear : nullptr;
                        if (!battle_gear)
                            continue;
                        if (battle_gear->is_affected_by_effect(EFFECT::START_GAME_FACEUP)) {
                            emplace_process<processors::RevealBattlegear>(
                                    battle_gear, nullptr, REASON::RULE, PLAYER::NONE, PLAYER(p));
                        }
                    }
                }
                return false;
            }
        case 3:
            {
                emplace_process<processors::Debug>();
                return false;
            }
        case 4:{
                return true;
            }
    }
    return true;
}
bool field::process(processors::Draw& arg) {
    auto reason_effect = arg.reason_effect;
    auto reason = arg.reason;
    auto reason_player = arg.reason_player;
    auto playerid = arg.playerid;
    auto count = arg.count;
    auto* exargs = arg.extra_args.get();
    auto& deck = player[+playerid].attack_deck;
    switch (arg.step) {
        case 0: {
                arg.extra_args = std::make_unique<processors::Draw::exargs>();
                return false;
            }
        case 1:
            {
                if (exargs->drawn >= count) {
                    arg.step = 2;
                    return false;
                }
                if (player[+playerid].attack_deck.empty()) {
                    if (exargs->deck_refreshed)
                        return true;
                    refresh_attack_deck(playerid);
                    exargs->deck_refreshed = true;
                    arg.step = 0;
                    return false;
                }
                return false;
            }
        case 2:
            {
                card* pcard = deck.back();
                pcard->enable_field_effect(false);
                pcard->cancel_field_effect();
                deck.pop_back();
                exargs->drawn++;
                if (!core.current_burst.empty()) {
                    core.just_sent_cards.insert(pcard);
                }

                pcard->previous.controller = pcard->current.controller;
                pcard->previous.location = pcard->current.location;
                pcard->previous.sequence = pcard->current.sequence;
                pcard->previous.position = pcard->current.position;

                pcard->current.controller = PLAYER::NONE;
                pcard->current.reason_effect = reason_effect;
                pcard->current.reason_player = reason_player;
                pcard->current.reason = reason | REASON::DRAW;
                pcard->current.location = LOCATION::NONE;

                add_card(playerid, pcard, LOCATION::ATTACK_HAND, 0);
                pcard->enable_field_effect(true);

                effect* pub = pcard->is_affected_by_effect(EFFECT::PUBLIC);
                if (pub) {
                    exargs->public_count++;
                }
                pcard->current.position = pub ? POSITION::FACE_UP : POSITION::FACE_DOWN;
                arg.drawn_set.insert(pcard);
                pcard->reset(RESET::TO_HAND, RESET::EVENT);

                auto& message = pmatch->new_message<MSG_Draw>();
                message.set_playerid(+playerid);
                message.set_code(pcard->data.code);
                message.set_position(+pcard->current.position);

                raise_single_event(pcard, nullptr, EVENT::DRAW, reason_effect, reason, reason_player, playerid, 0);
                raise_single_event(pcard, nullptr, EVENT::TO_HAND, reason_effect, reason, reason_player, playerid, 0);
                raise_single_event(pcard, nullptr, EVENT::MOVE, reason_effect, reason, reason_player, playerid, 0);

                arg.step = 0;
                process_single_event();
                return false;
            }
        case 3:
            {
                auto raise = [&](EVENT x) {
                    raise_event(arg.drawn_set, x, reason_effect, reason, reason_player, playerid, exargs->drawn);
                };
                raise(EVENT::DRAW);
                raise(EVENT::TO_HAND);
                raise(EVENT::MOVE);
                process_instant_event();
                return false;
            }
        case 4:
            {
                core.operated_set.swap(arg.drawn_set);
                returns.set<uint32_t>(0, count);
                return true;
            }
    }
    return true;
}
bool field::process(processors::SendTo& arg) {
    auto targets = arg.targets;
    auto reason_effect = arg.reason_effect;
    auto reason = arg.reason;
    auto reason_player = arg.reason_player;
    switch (arg.step) {
        case 0:
            {
                // -----------------------------------------
                // Remove cards from set that can't be moved
                // -----------------------------------------
                return false;
            }
        case 1:
            {
                // -------------------------------
                // Consider Replacement effects
                // -------------------------------
                return false;
            }
        case 2:
            {
                // -------------------------------
                // Set Previous State
                // -------------------------------
                if (targets->container.empty()) {
                    returns.set<int32_t>(0, 0);
                    core.operated_set.clear();
                    pmatch->delete_group(targets);
                }
                card_set leave_p, destroying;

                for (auto& pcard : targets->container) {
                    if (is(pcard->current.location, LOCATION::FIELD)) {
                        raise_single_event(pcard,
                                           nullptr,
                                           EVENT::LEAVE_FIELD_P,
                                           pcard->current.reason_effect,
                                           pcard->current.reason,
                                           pcard->current.reason_player,
                                           PLAYER::NONE,
                                           0);
                        leave_p.insert(pcard);

                        pcard->previous.horizontal = pcard->current.horizontal;
                        pcard->previous.vertical = pcard->previous.vertical;
                        if (is(pcard->current.position, POSITION::FACE_UP)) {
                            pcard->previous.subtypes = pcard->current.subtypes;
                            if (is(pcard->data.supertype, SUPERTYPE::CREATURE)) {
                                pcard->previous.energy = pcard->get_energy();
                                pcard->previous.damage = pcard->get_energy();
                                pcard->previous.stats = pcard->get_stats();
                                pcard->previous.elements = pcard->get_elements();
                                pcard->previous.mugic_counters = pcard->get_mugic_counters();
                                pcard->previous.tribes = pcard->get_tribes();
                            }
                        }
                    }
                }
                if (leave_p.size())
                    raise_event(std::move(leave_p),
                                EVENT::LEAVE_FIELD_P,
                                reason_effect,
                                reason,
                                reason_player,
                                PLAYER::NONE,
                                0);
                process_single_event();
                process_instant_event();
                return false;
            }
        case 3:
            {
                // -------------------------------
                // Calculate Redirections
                // -------------------------------
                LOCATION dest = LOCATION::NONE, redirect = LOCATION::NONE;
                uint32_t redirect_seq = 0;
                for (auto& pcard : targets->container) {
                    pcard->enable_field_effect(false);
                }
                for (auto& pcard : targets->container) {
                    if (is(pcard->current.location, LOCATION::FIELD)) {
                        redirect = pcard->leave_field_redirect(pcard->current.reason);
                        redirect_seq = +redirect >> 16;
                        redirect &= 0xffff;
                    }
                    if (+redirect) {
                        pcard->current.reason |= REASON::REDIRECT;
                        pcard->sendto_param.location = redirect;
                        pcard->sendto_param.sequence.index = redirect_seq;
                        dest = redirect;
                        if (dest == LOCATION::REMOVED && pcard->sendto_param.position != POSITION::FACE_UP)
                            pcard->sendto_param.position = POSITION::FACE_UP;
                    }
                    redirect = pcard->destination_redirect(dest, pcard->current.reason);
                    if (+redirect) {
                        redirect_seq = +redirect >> 16;
                        redirect &= 0xffff;
                    }
                    if (+redirect && (pcard->current.location != redirect)) {
                        pcard->current.reason |= REASON::REDIRECT;
                        pcard->sendto_param.location = redirect;
                        pcard->sendto_param.sequence.index = redirect_seq;
                    }
                }
                return false;
            }
        case 4:
            {
                // -------------------------------
                // INITIALIZE LOOP
                // -------------------------------
                arg.extra_args = std::make_unique<processors::SendTo::exargs>();
                auto* param = arg.extra_args.get();
                param->show_decktop[0] = false;
                param->show_decktop[1] = false;
                param->cv.assign(targets->container.begin(), targets->container.end());
                if (param->cv.size() > 1)
                    std::sort(param->cv.begin(), param->cv.end());
                param->cvit = param->cv.begin();
                return false;
            }
        case 5:
            {
                // -------------------------------
                // Loop Body
                // -------------------------------
                auto* param = arg.extra_args.get();
                if (param->cvit == param->cv.end()) {
                    return false;
                }
                card* pcard = *param->cvit;
                pcard->enable_field_effect(false);
                auto oloc = pcard->current.location;
                auto playerid = pcard->sendto_param.playerid;
                auto dest = pcard->sendto_param.location;
                auto seq = pcard->sendto_param.sequence;
                auto control_player = pcard->current.controller;
                if (pcard->current.controller != playerid or pcard->current.location != dest) {
                    auto& message = pmatch->new_message<MSG_Move>();
                    message.set_code(pcard->data.code);
                    write_location_info(message.mutable_previous_location(), pcard->get_location_info());
                    move_card(playerid, pcard, dest, seq);
                    write_location_info(message.mutable_new_location(), pcard->get_location_info());
                    message.set_reason(+pcard->current.reason);
                }
                if (is(oloc, LOCATION::FIELD)) {
                    param->leave_field.insert(pcard);
                } else if (is(oloc, LOCATION::GRAVE)) {
                    param->leave_grave.insert(pcard);
                }
                ++param->cvit;
                arg.step = 4;
                return false;
            }
        case 6:
            {
                // -------------------------------
                // Reset Flags Update
                // Raises Leave field/grave events
                // -------------------------------
                auto* param = arg.extra_args.get();
                for (auto& pcard : targets->container) {
                    pcard->enable_field_effect(true);
                    auto nloc = pcard->current.location;
                    switch (nloc) {
                        case LOCATION::ATTACK_HAND:
                        case LOCATION::MUGIC_HAND: pcard->reset(RESET::TO_HAND, RESET::EVENT); break;
                        case LOCATION::ATTACK_DECK:
                        case LOCATION::LOCATION_DECK: pcard->reset(RESET::TO_DECK, RESET::EVENT); break;
                        case LOCATION::ATTACK_DISCARD:
                        case LOCATION::GENERAL_DISCARD: pcard->reset(RESET::TO_GRAVE, RESET::EVENT); break;
                        case LOCATION::REMOVED: pcard->reset(RESET::REMOVE, RESET::EVENT); break;
                    }
                    pcard->refresh_negated_status();
                }

                for (auto& pcard : param->leave_field) {
                    raise_single_event(pcard,
                                       nullptr,
                                       EVENT::LEAVE_FIELD,
                                       pcard->current.reason_effect,
                                       pcard->current.reason,
                                       pcard->current.reason_player,
                                       PLAYER::NONE,
                                       0);
                }
                for (auto& pcard : param->leave_field) {
                    raise_single_event(pcard,
                                       nullptr,
                                       EVENT::LEAVE_GRAVE,
                                       pcard->current.reason_effect,
                                       pcard->current.reason,
                                       pcard->current.reason_player,
                                       PLAYER::NONE,
                                       0);
                }
                process_single_event();
                if (!param->leave_field.empty())
                    raise_event(std::move(param->leave_field),
                                EVENT::LEAVE_FIELD,
                                reason_effect,
                                reason,
                                reason_player,
                                PLAYER::NONE,
                                0);
                if (!param->leave_grave.empty())
                    raise_event(std::move(param->leave_grave),
                                EVENT::LEAVE_FIELD,
                                reason_effect,
                                reason,
                                reason_player,
                                PLAYER::NONE,
                                0);
                process_instant_event();
                adjust_instant();
                return false;
            }
        case 7:
            {
                // -------------------------------
                // Destroy lingering battlegear
                // Raises To destination events
                // -------------------------------
                card_set to_hand, to_deck, to_grave, remove, discard, destroyed, battle_gears, released;
                auto raise_card_event = [&, this](card* pcard, EVENT ev) {
                    raise_single_event(pcard,
                                       nullptr,
                                       EVENT::TO_HAND,
                                       pcard->current.reason_effect,
                                       pcard->current.reason,
                                       pcard->current.reason_player,
                                       PLAYER::NONE,
                                       0);
                };
                auto raise_set_event = [&, this](card_set& pcard, EVENT ev) {
                    raise_event(
                            std::move(pcard), EVENT::TO_HAND, reason_effect, reason, reason_player, PLAYER::NONE, 0);
                };
                for (auto& pcard : targets->container) {
                    auto nloc = pcard->current.location;
                    if (pcard->data.supertype == SUPERTYPE::CREATURE and pcard->current.battlegear) {
                        battle_gears.insert(pcard->current.battlegear);
                        pcard->current.battlegear = nullptr;
                    }
                    if (pcard->data.supertype == SUPERTYPE::BATTLE_GEAR and pcard->current.equipped_creature) {
                        pcard->unequip();
                    }
                    pcard->clear_card_target();
                    if (is(nloc, LOCATION::HAND)) {
                        if (pcard->owner != pcard->current.controller) {
                            /*
                             * Create Card Control Return?
                             */
                        }
                        to_hand.insert(pcard);
                        raise_card_event(pcard, EVENT::TO_HAND);
                    }
                    if (is(nloc, LOCATION::DECK)) {
                        to_deck.insert(pcard);
                        raise_card_event(pcard, EVENT::TO_DECK);
                    }
                    if (nloc == LOCATION::REMOVED) {
                        remove.insert(pcard);
                        raise_card_event(pcard, EVENT::REMOVE);
                    }
                    if (is(pcard->current.reason, REASON::DISCARD)) {
                        discard.insert(pcard);
                        raise_card_event(pcard, EVENT::DISCARD);
                    }
                    if (is(pcard->current.reason, REASON::SACRIFICE)) {
                        released.insert(pcard);
                        raise_card_event(pcard, EVENT::SACRIFICE);
                    }
                    if (is(pcard->current.reason, REASON::DESTROY)) {
                        destroyed.insert(pcard);
                        raise_card_event(pcard, EVENT::DESTROYED);
                    }
                    raise_card_event(pcard, EVENT::MOVE);
                }
                if (!to_hand.empty())
                    raise_set_event(to_hand, EVENT::TO_HAND);
                if (!to_deck.empty())
                    raise_set_event(to_deck, EVENT::TO_DECK);
                if (!to_grave.empty())
                    raise_set_event(to_grave, EVENT::TO_GRAVE);
                if (!remove.empty())
                    raise_set_event(remove, EVENT::REMOVE);
                if (!discard.empty())
                    raise_set_event(discard, EVENT::DISCARD);
                if (!released.empty())
                    raise_set_event(released, EVENT::SACRIFICE);
                if (!destroyed.empty())
                    raise_set_event(to_hand, EVENT::DESTROYED);
                raise_set_event(targets->container, EVENT::MOVE);
                process_single_event();
                process_instant_event();
                if (!battle_gears.empty()) {
                    send_to(std::move(battle_gears),
                            nullptr,
                            REASON::RULE | REASON::LOST_TARGET,
                            PLAYER::NONE,
                            PLAYER::NONE,
                            LOCATION::GENERAL_DISCARD,
                            0,
                            POSITION::FACE_UP,
                            false);
                }
                adjust_instant();
                return false;
            }
        case 8:
            {
                // -------------------------------
                // Finalize
                // -------------------------------
                core.operated_set.clear();
                core.operated_set = targets->container;
                returns.set<int32_t>(0, static_cast<int32_t>(targets->container.size()));
                pmatch->delete_group(targets);
                return true;
            }
    }
    return true;
}
bool field::process(processors::RevealBattlegear& arg) {
    fprintf(stderr, "PROCESS RevealBattlegear Not implemented\n");
    return true;
}
bool field::process(processors::Turn& arg) {
    fprintf(stderr, "PROCESS Turn Not implemented\n");
    return true;
}

bool field::process(processors::Debug& arg) {
    auto debugIndex = returns.at<uint32_t>(0);

    if (arg.step == 0)
        return false;

    switch (debugIndex) {
        case 0:
            return true;
        case 1: // DISCARD CARD
            {
                send_to({player[returns.at<uint32_t>(1)].attack_hand[returns.at<uint32_t>(2)]},
                        nullptr,
                        REASON::RULE | REASON::DISCARD,
                        PLAYER::NONE,
                        PLAYER(returns.at<uint32_t>(1)),
                        LOCATION::ATTACK_DISCARD,
                        0,
                        POSITION::FACE_UP,
                        false);
                emplace_process<processors::Debug>();
                return true;
            }
        case 2: // DRAW CARD
            {
                draw(nullptr, REASON::RULE, PLAYER::NONE, PLAYER(returns.at<uint32_t>(1)), 1);
                emplace_process<processors::Debug>();
                return true;
            }
        default: return false;
    }
}
