//
// Created by alexoxorn on 5/31/24.
//

#include "field.h"
#include "effect_constants.h"
#include "internal_common.h"
#include "match.h"

#define FAILED \
    { \
        pmatch->new_message<MSG_Retry>(); \
        return false; \
    }

#define ARG_CHECK(expr) \
    if (!(expr)) \
    FAILED


bool field::process(procs::SelectMoveCommand& arg) {
    switch (arg.step) {
        case 0:
            {
                auto& message = pmatch->new_message<MSG_SelectMove>();
                message.set_player(+arg.playerid);
                message.set_can_end_turn((bool) core.turn_move_count);
                for (const auto& [pcard, dest_list] : core.movable_cards) {
                    auto set = MSG_move_options();
                    set.set_code(pcard->data.code);
                    write_coord_info(set.mutable_source(), pcard->current.sequence);
                    for (auto newpos : dest_list) {
                        write_coord_info(set.add_destinations(), coord_from(newpos));
                    }
                }
                return false;
            }
        default:
            {
                auto [source, destination] = returns.get<int32_t, int32_t>();
                ARG_CHECK(source >= 0 || core.turn_move_count)
                ARG_CHECK(source < core.movable_cards.size())
                ARG_CHECK(destination < core.movable_cards[source].second.size())
                return true;
            }
    }
}

bool field::process(procs::SelectAttackCard& arg) {
    switch (arg.step) {
        case 0:
            {
                auto& message = pmatch->new_message<MSG_SelectAttackCard>();
                message.set_player(+arg.playerid);
                for (card* pcard : core.valid_attack_cards) {
                    auto* cardmsg = message.add_cards();
                    cardmsg->set_code(pcard->data.code);
                    cardmsg->set_index(pcard->current.sequence.index);
                }
                return false;
            }
        default:
            {
                auto choice = returns.get<int32_t>();
                ARG_CHECK(choice >= 0)
                ARG_CHECK(choice < core.valid_attack_cards.size());
                return true;
            }
    }
}