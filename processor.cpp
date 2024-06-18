//
// Created by alexoxorn on 6/10/24.
//

#include "field.h"
#include "effect_constants.h"
#include "internal_common.h"
#include "match.h"

bool field::raise_event(card* event_card, EVENT event_code, effect* reason_effect, REASON reason, PLAYER reason_player,
                        PLAYER event_player, uint32_t event_value) {
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
    //     fprintf(stderr, "field::process_single_event unimplemented\n");
    return 0;
}
int32_t field::process_instant_event() {
    //     fprintf(stderr, "field::process_single_event unimplemented\n");
    return 0;
}
void field::adjust_instant() {
    ++infos.event_id;
    adjust_disable_check_list();
    adjust_self_destroy_set();
}
void field::adjust_all() {
    ++infos.event_id;
    core.readjust_map.clear();
    emplace_process<procs::Adjust>();
}
void field::adjust_disable_check_list() {
    //     fprintf(stderr, "field::process_single_event unimplemented\n");
}
void field::adjust_self_destroy_set() {
    //     fprintf(stderr, "field::process_single_event unimplemented\n");
}

bool field::process(procs::Adjust& arg) {
    switch (arg.step) {
        case 0:
            {
                core.re_adjust = false;
                return false;
            }
        case 1:
            {
                // =============
                // CHECK FOR WIN
                // =============
                return false;
            }
        case 2:
            {
                // ==============================
                // CHECK FOR CREATURE DESTRUCTION
                // ==============================

                if (infos.turn_phase == PHASE::RECOVERY_STEP)
                    return false;

                card_set to_destroy;

                for (auto& pos : board) {
                    if (pos.blocked)
                        continue;

                    FOR_PLAYER_ID(i)
                    if (card* pcard = pos.creatures[i]; pcard) {
                        if (pcard->damage() >= pcard->get_energy()) {
                            to_destroy.insert(pcard);
                        }
                    }
                }

                if (!to_destroy.empty()) {
                    destroy(to_destroy, nullptr, REASON::RULE, PLAYER::NONE);
                    core.re_adjust = true;
                }

                return false;
            }
        case 3:
            {
                // ====================
                // CHECK FOR COMBAT WIN
                // ====================
                if (infos.combat_phase == PHASE::NONE)
                    return false;

                auto& combat_creatures = core.battle_position->creatures;
                if (combat_creatures[0] && combat_creatures[1]) {
                    return false;
                }
                if (combat_creatures.empty()) {
                    core.end_combat = true;
                    core.combat_winner = PLAYER::NONE;
                }
                card* victor = COALESCE(combat_creatures[0], combat_creatures[1]);
                victor->won_combat = true;
                core.end_combat = true;
                return false;
            }
        case 4:
            {
                // ==============
                // NEGATION CHECK
                // ==============
                return false;
            }
        case 5:
            {
                // ==================
                // BRAIN WASHED CHECK
                // ==================
                return false;
            }
        case 6:
            {
                if (false /*adjust_grant_effect()*/)
                    core.re_adjust = true;
                return false;
            }
        case 7:
            {
                // ============
                // SHUFFLE HAND
                // ============
                for (int p = 0; p < 2; p++) {
                    for (auto* pcard : player[p].attack_hand) {
                        effect* pub = pcard->is_affected_by_effect(EFFECT::PUBLIC);
                        if (!pub && pcard->is_position(POSITION::FACE_UP))
                            core.shuffle_attack_hand_check[p] = true;
                        pcard->current.position = pub ? POSITION::FACE_UP : POSITION::FACE_DOWN;
                    }
                    for (auto* pcard : player[p].mugic_hand) {
                        if (pcard->is_position(POSITION::FACE_UP))
                            core.shuffle_mugic_hand_check[0] = true;
                        pcard->current.position = POSITION::FACE_DOWN;
                    }
                }
                if (core.shuffle_attack_hand_check[infos.turn_player])
                    shuffle(static_cast<PLAYER>(infos.turn_player), LOCATION::ATTACK_HAND);
                if (core.shuffle_attack_hand_check[1 - infos.turn_player])
                    shuffle(static_cast<PLAYER>(1 - infos.turn_player), LOCATION::ATTACK_HAND);

                if (core.shuffle_mugic_hand_check[infos.turn_player])
                    shuffle(static_cast<PLAYER>(infos.turn_player), LOCATION::MUGIC_HAND);
                if (core.shuffle_mugic_hand_check[1 - infos.turn_player])
                    shuffle(static_cast<PLAYER>(1 - infos.turn_player), LOCATION::MUGIC_HAND);

                return false;
            }
        case 8:
            {
                // ===========
                // RAISE EVENT
                // ===========
                raise_event(nullptr, EVENT::ADJUST, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                return false;
            }
        case 9:
            {
                // ===========
                // RAISE EVENT
                // ===========
                if (core.re_adjust) {
                    arg.step = procs::restart;
                    return false;
                }
                for (int p = 0; p < 2; ++p) {
                    if (core.shuffle_attack_deck_check[0])
                        shuffle((PLAYER) p, LOCATION::ATTACK_DECK);
                    if (core.shuffle_location_deck_check[1])
                        shuffle((PLAYER) p, LOCATION::MUGIC_HAND);
                }
                return true;
            }
    }
    return true;
}
bool field::process(procs::Startup& arg) {
    switch (arg.step) {
        case 0:
            {
                core.shuffle_attack_hand_check[0] = false;
                core.shuffle_attack_hand_check[1] = false;
                core.shuffle_mugic_hand_check[0] = false;
                core.shuffle_mugic_hand_check[1] = false;
                core.shuffle_attack_deck_check[0] = true;
                core.shuffle_attack_deck_check[1] = true;
                core.shuffle_location_deck_check[0] = true;
                core.shuffle_location_deck_check[1] = true;
                raise_event(nullptr, EVENT::STARTUP, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                adjust_all();
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
                            emplace_process<procs::RevealBattlegear>(
                                    battle_gear, nullptr, REASON::RULE, PLAYER::NONE, PLAYER(p));
                        }
                    }
                }
                return false;
            }
        case 3:
            {
                emplace_process<procs::Turn>(PLAYER::ONE);
                return false;
            }
        case 4:
            {
                return true;
            }
    }
    return true;
}
bool field::process(procs::Turn& arg) {
    const auto& turn_player = arg.turn_player;
    switch (arg.step) {
        case 0:
            {
                // ==============================
                // Cleanup once per turn tracking
                // ==============================
                for (const auto& ev : core.used_event) {
                    if (ev.event_cards) {
                        pmatch->delete_group(ev.event_cards);
                    }
                }
                core.used_event.clear();
                core.turn_engage_count = 0;
                core.turn_move_count = 0;

                for (auto& peffect : core.reset_effects) {
                    pmatch->delete_effect(peffect);
                }

                core.reset_effects.clear();
                core.effect_count_code.clear();
                for (auto& pos : board) {
                    if (pos.blocked)
                        continue;

                    for (card* creature : pos.creatures) {
                        if (!creature)
                            continue;

                        creature->won_initiative = false;
                        creature->won_stat_check = false;
                        creature->won_challenge = false;
                        creature->won_stat_fail = false;
                        creature->moved_this_turn = 0;
                        creature->engaged_in_combat = 0;
                        creature->attack_damage_received = 0;
                        creature->attack_damage_delt = 0;
                        creature->effect_damage_received = 0;
                        creature->effect_damage_delt = 0;
                    }
                }
                return false;
            }
        case 1:
            {
                // ======================================
                // Update info and emit new turn event
                // ======================================

                ++infos.turn_id;
                ++infos.turn_id_by_player[+turn_player];
                infos.turn_player = +turn_player;
                ++turns_since_battle;

                auto& message = pmatch->new_message<MSG_NewTurn>();
                message.set_playerid(+turn_player);

                infos.turn_phase = PHASE::LOCATION_STEP;
                infos.combat_phase = PHASE::NONE;
                raise_event(nullptr, EVENT::PHASE_START_LOCATION, nullptr, REASON::NONE, PLAYER::NONE, turn_player, 0);

                process_instant_event();
                adjust_all();
                return false;
            }
        case 2:
            {
                return false;
            }
        case 3:
            {
                // ====================
                // LOCATION PHASE START
                // ====================
                core.new_triggers.clear();

                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.turn_phase);
                raise_event(nullptr, EVENT::PRE_LOCATION_REVEAL, nullptr, REASON::NONE, PLAYER::NONE, turn_player, 0);
                process_instant_event();

                if (!core.new_triggers.empty()) {
                    // emplace_process<procs::PointEvent>()
                }
                return false;
            }
        case 4:
            {
                return false;
            }
        case 5:
            {
                // ==================================
                // REVEAL LOCATION
                // AND MANAGE TRIGGERS AND OPEN BUSRT
                // ==================================
                reveal_location(turn_player);
                emplace_process<procs::PointEvent>(false, false, false);
                emplace_process<procs::PhaseEvent>(PHASE::LOCATION_STEP);
                return false;
            }
        case 6:
            {
                // =================
                // ACTION STEP START
                // =================
                infos.turn_phase = PHASE::ACTION_STEP;
                core.phase_action = false;
                raise_event(nullptr, EVENT::PHASE_START_ACTION, nullptr, REASON::RULE, PLAYER::NONE, turn_player, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case 7:
            {
                core.new_triggers.clear();
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.turn_phase);
                emplace_process<procs::MoveCommand>();
                return false;
            }
        case 8:
            {
                // ===================
                // RECOVERY STEP START
                // ===================
                infos.turn_phase = PHASE::RECOVERY_STEP;
                core.phase_action = false;
                raise_event(nullptr, EVENT::PHASE_START_RECOVERY, nullptr, REASON::RULE, PLAYER::NONE, turn_player, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case 9:
            {
                core.new_triggers.clear();
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.turn_phase);

                emplace_process<procs::Recover>(get_all_field_card(),
                                                std::numeric_limits<int32_t>::max(),
                                                procs::reason_data{.reason = REASON::RULE,
                                                                   .reason_player = PLAYER::NONE,
                                                                   .reason_effect = nullptr});

                adjust_all();
                return false;
            }
        case 10:
            {
                emplace_process<procs::ReturnLocation>();
            }
        case 11:
            {
                core.new_triggers.clear();
                arg.step = procs::restart;
                arg.turn_player = opp(arg.turn_player);
                return false;
            }
    }
    return true;
}
bool field::process(procs::Combat& arg) {
    auto moving_player = arg.player;
    auto destination_index = arg.destination_index;
    auto& combat_pos = board[arg.destination_index];

    constexpr int DEFENDER = 0;
    constexpr int ENGAGEMENT = 10;
    constexpr int BATTLEGEAR = 20;
    constexpr int BEGIN_COMBAT = 30;
    constexpr int INITIATIVE = 40;
    constexpr int STRIKE = 50;
    constexpr int NEW_STRIKER = 60;
    constexpr int AFTER_DAMAGE = 70;
    constexpr int COMBAT_END = 80;

#define END_COMBAT_CHECK \
    if (core.end_combat) { \
        arg.step = COMBAT_END - 1; \
        return false; \
    }

    switch (arg.step) {
        case 0:
            {
                // ===================
                // CHECK FOR DEFENDERS
                // ===================

                // Use `effect_filter` to get list of valid defenders
                // emplace <DefenderSelect> to prompt opponent to select defender
                return false;
            }
        case 1:
            {
                // ==================================
                // Swap engaged opponent and defender
                // ==================================

                arg.step = ENGAGEMENT - 1;
                return false;
            }
        case ENGAGEMENT:
            {
                // ===============
                // ENGAGEMENT STEP
                // ===============

                arg.creatures = combat_pos.creatures;

                infos.turns_since_no_damage = 0;

                auto& message = pmatch->new_message<MSG_CombatStart>();
                message.set_code1(combat_pos.creatures[0]->data.code);
                message.set_code2(combat_pos.creatures[1]->data.code);
                write_coord_info(message.mutable_battle_location(), coord_from(destination_index));

                infos.combat_phase = PHASE::ENGAGEMENT;
                core.battle_position = &combat_pos;
                core.end_combat = false;
                for (card* pcard : combat_pos.creatures) {
                    pcard->engaged_in_combat = true;
                }

                raise_event(
                        nullptr, EVENT::PHASE_START_ENGAGEMENT, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case ENGAGEMENT + 1:
            {
                END_COMBAT_CHECK
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.combat_phase);
                arg.step = BATTLEGEAR - 1;
                return false;
            }
        case BATTLEGEAR:
            {
                // ======================
                // REVEAL BATTLEGEAR STEP
                // ======================
                infos.combat_phase = PHASE::REVEAL_BATTLE;

                raise_event(
                        nullptr, EVENT::PHASE_START_BATTLEGEAR, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case BATTLEGEAR + 1:
            {
                END_COMBAT_CHECK
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.combat_phase);

                for (card* pcard : combat_pos.creatures) {
                    emplace_process<procs::RevealBattlegear>(
                            pcard->current.battlegear, nullptr, REASON::RULE, PLAYER::NONE, pcard->current.controller);
                }
                adjust_all();
                arg.step = BEGIN_COMBAT - 1;
                return false;
            }
        case BEGIN_COMBAT:
            {
                // ========================
                // BEGINNING OF COMBAT STEP
                // ========================
                END_COMBAT_CHECK
                infos.combat_phase = PHASE::BEGINNING_OF_COMBAT;

                raise_event(nullptr,
                            EVENT::PHASE_START_BEGINNING_COMBAT,
                            nullptr,
                            REASON::NONE,
                            PLAYER::NONE,
                            PLAYER::NONE,
                            0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case BEGIN_COMBAT + 1:
            {
                END_COMBAT_CHECK
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.combat_phase);

                if (!core.new_triggers.empty()) {
                    emplace_process<procs::PointEvent>();
                }
                adjust_all();
                arg.step = INITIATIVE - 1;
            }
        case INITIATIVE:
            {
                // ===============
                // INITIATIVE STEP
                // ===============
                END_COMBAT_CHECK
                infos.combat_phase = PHASE::INITIATIVE;

                raise_event(
                        nullptr, EVENT::PHASE_START_INITIATIVE, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case INITIATIVE + 1:
            {
                auto& new_phase_message = pmatch->new_message<MSG_NewPhase>();
                new_phase_message.set_phase(+infos.combat_phase);

                /*
                 * (1) Any Creature with the Surprise ability. If both players’ Creatures have this ability
                 * then the Initiative check moves to (2)
                 */

                bool p1_has_surprise = combat_pos.creatures[0]->is_affected_by_effect(EFFECT::SURPRISE);
                bool p2_has_surprise = combat_pos.creatures[1]->is_affected_by_effect(EFFECT::SURPRISE);
                bool p1_lose_init;
                bool p2_lose_init;

                if (p1_has_surprise and not p2_has_surprise) {
                    infos.striking_player = 0;
                    goto initiative_end;
                } else if (p2_has_surprise and not p1_has_surprise) {
                    infos.striking_player = 1;
                    goto initiative_end;
                }

                /*
                 * (2) If a Creature has “loses Initiative checks” then the opponent’s Creature gains
                 * Initiative. If both players’ Creatures have this ability then the Initiative check moves to (3)
                 */

                p1_lose_init = combat_pos.creatures[0]->is_affected_by_effect(EFFECT::LOST_INITIATIVE);
                p2_lose_init = combat_pos.creatures[1]->is_affected_by_effect(EFFECT::LOST_INITIATIVE);

                if (p1_lose_init and not p2_lose_init) {
                    infos.striking_player = 1;
                    goto initiative_end;
                } else if (p2_lose_init and not p1_lose_init) {
                    infos.striking_player = 0;
                    goto initiative_end;
                }

                /*
                 * SKIPPED FOR NOW
                 * (3) The active Location lists an attribute in the form “Initiative: Characteristic.” The
                 * attribute is a value (e.g. Wisdom, Scanned Energy) or other card characteristic (e.g.
                 * Fire, OverWorld, etc.). Whichever engaged creature has a higher statistic or has the
                 * listed characteristic gains Initiative. In the event of a tie move to (4)
                 */

                /*
                 * (4) The Creature controlled by the Active Player gains Initiative.
                 */

                infos.striking_player = infos.turn_player;

            initiative_end:
                auto& message = pmatch->new_message<MSG_WonInitiative>();
                message.set_playerid(infos.striking_player);
                combat_pos.creatures[infos.striking_player]->won_initiative = true;
                arg.step = STRIKE - 1;
                return false;
            }
        case STRIKE:
            {
                // ==============
                // STRIKING PHASE
                // ==============
                infos.combat_phase = PHASE::STRIKE_PHASE;

                raise_event(nullptr, EVENT::PHASE_START_STRIKE, nullptr, REASON::NONE, PLAYER::NONE, PLAYER::NONE, 0);
                process_instant_event();
                adjust_all();
                return false;
            }
        case STRIKE + 1:
            {
                END_COMBAT_CHECK
                auto& message = pmatch->new_message<MSG_NewPhase>();
                message.set_phase(+infos.combat_phase);
                arg.step = NEW_STRIKER - 1;

                return false;
            }
        case NEW_STRIKER:
            {
                auto& message = pmatch->new_message<MSG_StrikerChange>();
                message.set_playerid(+infos.striking_player);
                return false;
            }
        case NEW_STRIKER + 1:
            {
                // ================
                // DRAW ATTACK CARD
                // ================
                draw(nullptr, REASON::RULE, PLAYER::NONE, (PLAYER) infos.striking_player, 1);
                adjust_all();
                return false;
            }
        case NEW_STRIKER + 2:
            {
                // ==================
                // SELECT ATTACK CARD
                // ==================
                END_COMBAT_CHECK
                if (combat_pos.creatures[infos.striking_player]->is_affected_by_effect(EFFECT::RANDOM_ATTACK)) {
                    std::uniform_int_distribution<> distrib(0,
                                                            (int) player[infos.striking_player].attack_hand.size() - 1);
                    returns.put<int32_t>(distrib(pmatch->random));
                    return false;
                }

                core.valid_attack_cards = player[infos.striking_player].attack_hand;
                emplace_process<procs::SelectAttackCard>((PLAYER) infos.striking_player);
                return false;
            }
        case NEW_STRIKER + 3:
            {
                // ================
                // PLAY ATTACK CARD
                // ================
                auto choice = returns.get<int32_t>();

                /*
                 * ADD TO BURST
                 * WILL ADD WHEN IMPLEMENTING EFFECT
                 * IN THE MEANTIME, JUST RESOLVE DAMAGE DIRECTLY
                 */

                card* attack_card = core.valid_attack_cards[choice];
                card* source = combat_pos.creatures[infos.striking_player];
                card* target = combat_pos.creatures[1 - infos.striking_player];

                send_to(attack_card,
                        nullptr,
                        REASON::RULE | REASON::DISCARD,
                        PLAYER::NONE,
                        PLAYER(infos.striking_player),
                        LOCATION::ATTACK_DISCARD,
                        0,
                        POSITION::FACE_UP,
                        false);

                auto [damage, type] = attack_card->calculate_attack_damage(source, target);
                REASON reason = REASON::ATTACK;
                if (type & 0b1)
                    reason |= REASON::FIRE;
                if (type & 0b10)
                    reason |= REASON::AIR;
                if (type & 0b100)
                    reason |= REASON::EARTH;
                if (type & 0b1000)
                    reason |= REASON::WATER;

                infos.turns_since_no_damage = damage ? 0 : infos.turns_since_no_damage + 1;

                emplace_process<procs::Damage>(card_set{target},
                                               source,
                                               damage,
                                               procs::reason_data{.reason = reason,
                                                                  .reason_player = (PLAYER) infos.striking_player,
                                                                  .reason_effect = nullptr});

                adjust_all();
                arg.step = AFTER_DAMAGE - 1;
                return false;
            }
        case AFTER_DAMAGE:
            {
                END_COMBAT_CHECK
                if (infos.turns_since_no_damage >= 20) {
                    destroy(card_set(std::begin(combat_pos.creatures), std::end(combat_pos.creatures)),
                            nullptr,
                            REASON::RULE,
                            PLAYER::NONE);
                    adjust_all();
                    return false;
                }

                if (!(combat_pos.creatures[0] && combat_pos.creatures[1])) {
                    adjust_all();
                    return false;
                }

                emplace_process<procs::PointEvent>();
                return false;
            }
        case AFTER_DAMAGE + 1:
            {
                // ===========
                // SWITCH TURN
                // ===========
                END_COMBAT_CHECK
                infos.striking_player = 1 - infos.striking_player;
                arg.step = NEW_STRIKER - 1;
                return false;
            }
        case COMBAT_END:
            {
                // ==========
                // END COMBAT
                // ==========
                auto& message = pmatch->new_message<MSG_CombatEnd>();
//                asm("int3");
                message.set_code1(arg.creatures[0]->data.code);
                message.set_code2(arg.creatures[1]->data.code);
                message.set_winner(+core.combat_winner);
                if (core.combat_winner != PLAYER::NONE) {
                    arg.creatures[+core.combat_winner]->won_combat = true;
                }

                infos.combat_phase = PHASE::NONE;
                core.end_combat = false;
                core.combat_winner = PLAYER::NONE;
                core.battle_position = nullptr;
                for (card* pcard : arg.creatures) {
                    pcard->engaged_in_combat = false;
                }
                return true;
            }
    }
    return true;
}
bool field::process(procs::MoveCommand& arg) {
    switch (arg.step) {
        case 0:
            {
                // ======================
                // Set up allowed actions
                // ======================

                // Mandatory Move Into
                // EFFECT::MUST_MOVE_INTO

                // Mandatory Attack     (Rothar, Forceful Negotiator)
                // EFFECT::MUST_ATTACK

                core.movable_cards.clear();

                if (core.must_move) {
                    return false;
                }

                for (auto& pos : board) {
                    card* pcard = pos.creatures[+infos.turn_player];
                    if (!pcard or pos.blocked) {
                        continue;
                    }
                    if (pcard->is_affected_by_effect(EFFECT::CANNOT_MOVE)) {
                        continue;
                    }
                    if (pcard->moved_this_turn)
                        continue;

                    uint32_t moves = 1 + pcard->accumulate_effect(EFFECT::SWIFT);
                    bool has_range = pcard->is_affected_by_effect(EFFECT::RANGE);

                    int32_t starting_index = index_from(pcard->current.sequence);
                    std::unordered_set<int32_t> attack_positions;
                    std::unordered_set<int32_t> movable_positions;
                    std::unordered_set<int32_t> intermediate{starting_index};
                    std::unordered_set<int32_t> intermediate_next;

                    if (pcard->is_affected_by_effect(EFFECT::CAN_MOVE_ANY_UNOCCUPIED)) {
                        for (const auto& newpos : board) {
                            if (!newpos.occupied() and !newpos.blocked)
                                movable_positions.insert((int32_t) (&newpos - board.begin().base()));
                        }
                    }

                    for (int step = 0; step < moves; ++step) {
                        for (auto from : intermediate) {
                            for (int32_t newindex : adjacent(from)) {
                                if (newindex == starting_index)
                                    continue;
                                auto& newpos = board[newindex];
                                if (newpos.blocked)
                                    continue;

                                if (!newpos.occupied()) {
                                    intermediate_next.insert(newindex);
                                    movable_positions.insert(newindex);
                                }
                                if (has_range) {
                                    intermediate_next.insert(newindex);
                                }
                                if (newpos.creatures[1 - infos.turn_player] and not core.turn_engage_count) {
                                    attack_positions.insert(newindex);
                                }
                            }
                        }
                        std::swap(intermediate, intermediate_next);
                        intermediate_next.clear();
                    }
                    std::vector<int32_t> option_list;
                    option_list.insert(option_list.end(), movable_positions.begin(), movable_positions.end());
                    option_list.insert(option_list.end(), attack_positions.begin(), attack_positions.end());
                    if (!option_list.empty())
                        core.movable_cards.emplace_back(pcard, option_list);
                }

                if (core.movable_cards.empty())
                    returns.put(-1);
                else
                    emplace_process<procs::SelectMoveCommand>((PLAYER) infos.turn_player);
                return false;
            }
        case 1:
            {
                auto [source_index, destination_index] = returns.get<int32_t, int32_t>();

                if (source_index < 0) {
                    return true;
                }

                const auto& source = core.movable_cards[source_index];
                const auto& destination = source.second[destination_index];

                arg.card_to_move = source.first;

                emplace_process<procs::Move>((PLAYER) infos.turn_player,
                                             index_from(source.first->current.sequence),
                                             destination,
                                             !core.turn_engage_count,
                                             procs::reason_data{.reason = REASON::NONE,
                                                                .reason_player = PLAYER::NONE,
                                                                .reason_effect = nullptr});
                return false;
            }
        case 2:
            {
                arg.card_to_move->moved_this_turn = true;
                auto battle = returns.get<bool>();
                if (battle) {
                    core.turn_engage_count++;
                    emplace_process<procs::Combat>(
                            (PLAYER) infos.turn_player, index_from(arg.card_to_move->current.sequence), true);
                }
                return false;
            }
        case 3:
            {
                emplace_process<procs::PointEvent>(false, false, false);
                return false;
            }
        case 4:
            {
                arg.step = procs::restart;
                return false;
            }
    }
    return true;
}
bool field::process(procs::Debug& arg) {
    auto [x, y, z] = returns.get<int32_t, int32_t, int32_t>();
    auto debugIndex = x;

    if (arg.step == 0)
        return false;

    switch (debugIndex) {
        case 0: return true;
        case 1: // DISCARD CARD
            {
                send_to(player[y].attack_hand[z],
                        nullptr,
                        REASON::RULE | REASON::DISCARD,
                        PLAYER::NONE,
                        PLAYER(x),
                        LOCATION::ATTACK_DISCARD,
                        0,
                        POSITION::FACE_UP,
                        false);
                emplace_process<procs::Debug>();
                return true;
            }
        case 2: // DRAW CARD
            {
                draw(nullptr, REASON::RULE, PLAYER::NONE, PLAYER(y), 1);
                emplace_process<procs::Debug>();
                return true;
            }
        default: return false;
    }
}
bool field::process(procs::PhaseEvent& arg) {
    return true;
}
bool field::process(procs::PointEvent& arg) {
    return true;
}
