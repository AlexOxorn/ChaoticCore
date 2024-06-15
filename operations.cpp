//
// Created by alexoxorn on 5/14/24.
//

#include "field.h"
#include "effect_constants.h"
#include "internal_common.h"
#include "match.h"

void field::draw(effect* reason_effect, REASON reason, PLAYER reason_player, PLAYER playerid, uint16_t count) {
    emplace_process<procs::Draw>(reason_effect, reason, reason_player, playerid, count);
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
std::vector<int32_t> field::adjacent(int32_t from) {
    auto [x, y] = coord_from(from);

    std::vector<int32_t> ret;
    auto move = [&, this](int newx, int newy) {
        auto newpos = get_board_from_sequence({x + 1, y + 1});
        if (!newpos.blocked)
            ret.push_back(index_from({x + 1, y + 1}));
    };
    // Up Left
    if (y > 0 and x > 0)
        move(x - 1, y - 1);
    // up
    if (y > 0)
        move(x, y - 1);
    // up right
    if (y > 0 and x < 2 * board_size)
        move(x + 1, y - 1);

    // Left
    if (x > 0)
        move(x - 1, y);
    // right
    if (x < 2 * board_size)
        move(x + 1, y);

    // down Left
    if (y < board_size and x > 0)
        move(x - 1, y + 1);
    // down
    if (y < board_size)
        move(x, y + 1);
    // up right
    if (y < board_size and x < 2 * board_size)
        move(x + 1, y + 1);

    return ret;
}

bool field::process(procs::Draw& arg) {
    auto reason_effect = arg.reason_effect;
    auto reason = arg.reason;
    auto reason_player = arg.reason_player;
    auto playerid = arg.playerid;
    auto count = arg.count;
    auto* exargs = arg.extra_args.get();
    auto& deck = player[+playerid].attack_deck;
    switch (arg.step) {
        case 0:
            {
                arg.extra_args = std::make_unique<procs::Draw::exargs>();
                return false;
            }
        case 1:
            {
                if (exargs->drawn >= count) {
                    arg.step = 3;
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
                adjust_all();
                return false;
            }
        case 3:
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
        case 4:
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
        case 5:
            {
                core.operated_set.swap(arg.drawn_set);
                returns.set<int32_t>(count);
                return true;
            }
    }
    return true;
}
bool field::process(procs::SendTo& arg) {
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
                    returns.set<int32_t>(0);
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
                arg.extra_args = std::make_unique<procs::SendTo::exargs>();
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
                returns.set<int32_t>(static_cast<int32_t>(targets->container.size()));
                pmatch->delete_group(targets);
                return true;
            }
    }
    return true;
}
bool field::process(procs::RevealBattlegear& arg) {
    fprintf(stderr, "PROCESS RevealBattlegear Not implemented\n");
    return true;
}
bool field::process(procs::Move& arg) {
    auto moving_player = arg.player;
    auto source_index = arg.source_index;
    auto destination_index = arg.destination_index;

    constexpr static int COMBAT = 1000;

    switch (arg.step) {
        case 0:
            {
                // ============
                // SANITY CHECK
                // ============
                if (!board[source_index].creatures[+moving_player]) {
                    returns.set(false);
                    return true;
                }
                if (board[destination_index].creatures[+moving_player]) {
                    returns.set(false);
                    return true;
                }
                if (!arg.allow_combat && board[destination_index].creatures[+opp(moving_player)]) {
                    returns.set(false);
                    return true;
                }
                if (board[destination_index].blocked) {
                    returns.set(false);
                    return true;
                }

                return false;
            }
        case 1:
            {
                // ============
                // COMBAT CHECK
                // ============
                if (board[destination_index].creatures[+opp(moving_player)]) {
                    arg.start_combat = true;
                    return false;
                }

                return false;
            }
        case 2:
            {
                // ===============
                // NON COMBAT MOVE
                // ===============

                card* pcard = board[source_index].creatures[+moving_player];
                auto& message = pmatch->new_message<MSG_CreatureMove>();

                message.set_code(pcard->data.code);
                write_location_info(message.mutable_previous_location(), pcard->get_location_info());

                std::swap(board[source_index].creatures[+moving_player],
                          board[destination_index].creatures[+moving_player]);
                pcard->previous.sequence = pcard->current.sequence;
                pcard->current.sequence = coord_from(destination_index);

                write_location_info(message.mutable_new_location(), pcard->get_location_info());
                message.set_combat(arg.start_combat);

                raise_single_event(pcard,
                                   nullptr,
                                   EVENT::CREATURE_MOVE,
                                   arg.reason_effect,
                                   arg.reason,
                                   arg.reason_player,
                                   moving_player,
                                   destination_index);

                raise_event(pcard,
                            EVENT::CREATURE_MOVE,
                            arg.reason_effect,
                            arg.reason,
                            arg.reason_player,
                            moving_player,
                            destination_index);

                process_instant_event();
                process_single_event();
                returns.set(arg.start_combat);
                return true;
            }
    }

    return true;
}
bool field::process(procs::Damage& arg) {
    switch (arg.step) {
        case 0:
            {
                // =============
                // EFFECT CHECKS
                // =============
                return false;
            }
        case 1:
            {
                for (auto target : arg.targets) {
                    target->previous.damage = target->current.damage;
                    target->current.damage += arg.amount;

                    if (is(arg.reason, REASON::ATTACK)) {
                        target->attack_damage_received += arg.amount;
                    }
                    if (is(arg.reason, REASON::EFFECT)) {
                        target->effect_damage_received += arg.amount;
                    }
                    auto& message = pmatch->new_message<MSG_Damage>();
                    message.set_amount(arg.amount);
                    message.set_reason(+arg.reason);
                    message.set_code(target->data.code);
                    write_location_info(message.mutable_loc_info(), target->get_location_info());
                }

                if (arg.source and is(arg.reason, REASON::ATTACK)) {
                    arg.source->attack_damage_delt += arg.amount;
                }

                if (arg.source)
                    raise_event(arg.source,
                                EVENT::DAMAGE_DEALT,
                                arg.reason_effect,
                                arg.reason,
                                arg.reason_player,
                                PLAYER::NONE,
                                arg.amount);
                raise_event(arg.targets,
                            EVENT::DAMAGE_RECEIVED,
                            arg.reason_effect,
                            arg.reason,
                            arg.reason_player,
                            PLAYER::NONE,
                            arg.amount);

                for (auto* target : arg.targets) {
                    raise_single_event(target,
                                       nullptr,
                                       EVENT::DAMAGE_RECEIVED,
                                       arg.reason_effect,
                                       arg.reason,
                                       arg.reason_player,
                                       PLAYER::NONE,
                                       arg.amount);
                }

                if (arg.source)
                    raise_single_event(arg.source,
                                       nullptr,
                                       EVENT::DAMAGE_DEALT,
                                       arg.reason_effect,
                                       arg.reason,
                                       arg.reason_player,
                                       PLAYER::NONE,
                                       arg.amount);

                process_single_event();
                process_instant_event();
                return true;
            }
    }
    return true;
}
bool field::process(procs::Recover& arg) {
    switch (arg.step) {
        case 0:
            {
                // =============
                // EFFECT CHECKS
                // =============
                return false;
            }
        case 1:
            {
                for (auto target : arg.targets) {
                    target->previous.damage = target->current.damage;
                    target->current.damage = std::max(target->current.damage - arg.amount, 0);

                    int32_t diff = target->previous.damage - target->current.damage;
                    if (diff == 0) {
                        arg.targets.erase(target);
                        continue;
                    }

                    raise_single_event(target,
                                       nullptr,
                                       EVENT::HEALED,
                                       arg.reason_effect,
                                       arg.reason,
                                       arg.reason_player,
                                       PLAYER::NONE,
                                       diff);

                    auto& message = pmatch->new_message<MSG_Recover>();
                    message.set_amount(diff);
                    message.set_reason(+arg.reason);
                    message.set_code(target->data.code);
                    write_location_info(message.mutable_loc_info(), target->get_location_info());
                }

                raise_event(arg.targets,
                            EVENT::HEALED,
                            arg.reason_effect,
                            arg.reason,
                            arg.reason_player,
                            PLAYER::NONE,
                            arg.amount);


                process_single_event();
                process_instant_event();
                return true;
            }
    }
    return true;
}
bool field::process(procs::Destroy& arg) {
    auto targets = arg.targets;
    auto reason_effect = arg.reason_effect;
    auto reason = arg.reason;
    auto reason_player = arg.reason_player;
    switch (arg.step) {
        case 0:
            {
                card_set extra;
                effect_list eset;
                card_set indestructable_set;
                for (auto cit = targets->container.begin(); cit != targets->container.end();) {
                    auto rm = cit++;
                    card* pcard = *rm;
                    if (!pcard->is_destructible()) {
                        indestructable_set.insert(pcard);
                        continue;
                    }
                    eset.clear();
                    pcard->filter_effect(EFFECT::INDESTRUCTIBLE, eset);
                    if (!eset.empty()) {
                        bool is_destructable = true;
                        for (const auto& peff : eset) {
                            // Check for valid destructible effect
                        }
                        if (!is_destructable) {
                            indestructable_set.insert(pcard);
                            continue;
                        }
                    }
                    eset.clear();
                }
                for (auto& pcard : indestructable_set) {
                    pcard->current.reason = pcard->temp.reason;
                    pcard->current.reason_effect = pcard->temp.reason_effect;
                    pcard->current.reason_player = pcard->temp.reason_player;
                    pcard->set_status(STATUS::DESTROY_CONFIRMED, false);
                    targets->container.erase(pcard);
                }
                arg.step = 2;
                return false;
            }
        case 3:
            {
                if (targets->container.empty()) {
                    returns.set<int32_t>(0);
                    core.operated_set.clear();
                    pmatch->delete_group(targets);
                    return true;
                }
                card_vector cv(targets->container.begin(), targets->container.end());
                if (cv.size() > 1)
                    std::sort(cv.begin(), cv.end(), card::card_operation_sort);
                for (auto& pcard : cv) {
                    if (is(pcard->current.location, (LOCATION::GRAVE | LOCATION::REMOVED))) {
                        pcard->current.reason = pcard->temp.reason;
                        pcard->current.reason_effect = pcard->temp.reason_effect;
                        pcard->current.reason_player = pcard->temp.reason_player;
                        targets->container.erase(pcard);
                        continue;
                    }
                    pcard->current.reason |= REASON::DESTROY;
                    raise_single_event(pcard,
                                       nullptr,
                                       EVENT::DESTROY,
                                       pcard->current.reason_effect,
                                       pcard->current.reason,
                                       pcard->current.reason_player,
                                       PLAYER::NONE,
                                       0);
                }
                adjust_instant();
                process_single_event();
                raise_event(targets->container, EVENT::DESTROY, reason_effect, reason, reason_player, PLAYER::NONE, 0);
                process_instant_event();
                return false;
            }
        case 4:
            {
                group* sendtargets = pmatch->new_group(targets->container);
                sendtargets->is_readonly = true;
                for (auto& pcard : sendtargets->container) {
                    pcard->set_status(STATUS::DESTROY_CONFIRMED, false);
                    LOCATION dest = pcard->sendto_param.location;
                    if (!dest)
                        dest = pcard->grave_for();
                    pcard->sendto_param.location = dest;
                }
                emplace_process<procs::SendTo>(
                        Step{1}, sendtargets, reason_effect, reason | REASON::DESTROY, reason_player);
                return false;
            }
        case 5:
            {
                core.operated_set.clear();
                core.operated_set = targets->container;
                returns.set<int32_t>(static_cast<int32_t>(core.operated_set.size()));
                pmatch->delete_group(targets);
                return true;
            }
    }
    return true;
}
bool field::process(procs::RevealLocation& arg) {
    switch (arg.step) {
        case 0:
            {
                // ========================
                // HANDLE LOCATION OVERRIDE
                // ========================
                return false;
            }
        case 1:
            {
                // ========================
                // GET TOP OF LOCATION DECK
                // AND ACTIVATE
                // ========================
                card* topdeck = player[+arg.playerid].location_deck.back();
                topdeck->current.reason = arg.reason;
                topdeck->current.reason_player = arg.reason_player;
                topdeck->current.reason_effect = arg.reason_effect;
                emplace_process<procs::ActivateLocation>(player[+arg.playerid].location_deck.back(),
                                                         arg.playerid,
                                                         arg.reason_effect,
                                                         arg.reason,
                                                         arg.reason_player);
                return true;
            }
    }
    return true;
}

bool field::process(procs::ReturnLocation& arg) {
    // Handle Currently Active Location
    if (active_location) {
        if (active_location->is_mirage())
            set_mirage(active_location, nullptr, REASON::RULE, PLAYER::NONE);
        else {
            send_to(active_location,
                    nullptr,
                    REASON::RULE,
                    PLAYER::NONE,
                    active_location->owner,
                    LOCATION::LOCATION_DECK,
                    DECK_BOTTOM,
                    POSITION::FACE_DOWN,
                    false);
        }
    }
    return true;
}

bool field::process(procs::ActivateLocation& arg) {
    switch (arg.step) {
        case 0:
            {
                // ========================
                // HANDLE LOCATION OVERRIDE
                // ========================
                return false;
            }
        case 1:
            {
                // Handle Currently Active Location
                emplace_process<procs::ReturnLocation>();
                return false;
            }
        case 2:
            {
                auto& message = pmatch->new_message<MSG_ActivateLocation>();
                message.set_player(+arg.playerid);
                message.set_code(arg.pcard->data.code);
                write_location_info(message.mutable_previous_location(), arg.pcard->get_location_info());

                remove_card(arg.pcard);
                add_card(arg.playerid, arg.pcard, LOCATION::ACTIVE_LOCATION, 0);

                arg.pcard->enable_field_effect(true);

                raise_single_event(arg.pcard,
                                   nullptr,
                                   EVENT::ACTIVATED_LOCATION,
                                   arg.pcard->current.reason_effect,
                                   arg.pcard->current.reason,
                                   arg.pcard->current.reason_player,
                                   PLAYER::NONE,
                                   0);

                raise_event(arg.pcard,
                            EVENT::ACTIVATED_LOCATION,
                            arg.reason_effect,
                            arg.reason,
                            arg.reason_player,
                            PLAYER::NONE,
                            0);

                process_single_event();
                process_instant_event();

                arg.pcard->refresh_negated_status();
                return true;
            }
    }
    return true;
}
bool field::process(procs::SetMirage& arg) {
    return true;
}
