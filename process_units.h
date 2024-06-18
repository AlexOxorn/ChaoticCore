//
// Created by alexoxorn on 5/14/24.
//

#ifndef CHAOTIC_CORE_PROCESS_UNITS_H
#define CHAOTIC_CORE_PROCESS_UNITS_H

#include <cstdint>
#include <deque>
#include <variant>
#include "common.h"
#include "effect.h"
#include "internal_common.h"
#include "card.h"
#include "memory"

class group;

namespace procs {
    constexpr inline auto restart = static_cast<uint16_t>(~(uint16_t()));

    struct reason_data {
        REASON reason;
        PLAYER reason_player;
        effect* reason_effect;
    };

    template <bool NeedResponse>
    struct Process {
        constexpr static auto needs_response = NeedResponse;
        Process(const Process&) = delete;            // non construction-copyable
        Process& operator=(const Process&) = delete; // non copyable
        Process(Process&&) = default;                // construction-movable
        Process& operator=(Process&&) = default;     // movable
        uint16_t step;
    public:
        explicit Process(uint16_t step_) : step(step_) {}
    };

    struct Startup : public Process<false> {
        using Process::Process;
    };

    struct Adjust : public Process<false> {
        using Process::Process;
    };

    struct Turn : public Process<false> {
        PLAYER turn_player;
        using Process::Process;
        Turn(uint16_t p_step, PLAYER p_turn_player) : Process(p_step), turn_player(p_turn_player) {}
    };

    struct RevealBattlegear : public Process<false>,
                              reason_data {
        card* battlegear;
        PLAYER playerid;
        RevealBattlegear(uint16_t step, card* p_battlegear, effect* p_reason_effect, REASON p_reason,
                         PLAYER p_reason_player, PLAYER p_playerid) :
                Process<false>(step),
                reason_data(p_reason, p_reason_player, p_reason_effect),
                battlegear{p_battlegear},
                playerid{p_playerid} {};
    };

    struct Draw : public Process<false>,
                  reason_data {
        uint16_t count;
        PLAYER playerid;
        card_set drawn_set;

        struct exargs {
            uint16_t drawn = 0;
            bool deck_refreshed = false;
            uint32_t public_count = 0;
        };
        std::unique_ptr<exargs> extra_args;
        Draw(uint16_t step_, effect* reason_effect_, REASON reason_, PLAYER reason_player_, PLAYER playerid_,
             uint16_t count_) :
                Process(step_),
                reason_data(reason_, reason_player_, reason_effect_),
                count(count_),
                playerid(playerid_) {}
    };

    struct SendTo : public Process<false> {
        struct exargs {
            card_set leave_field, leave_grave, detach;
            bool show_decktop[2];
            card_vector cv;
            card_vector::iterator cvit;
            effect* predirect;
        };
        PLAYER reason_player;
        REASON reason;
        group* targets;
        effect* reason_effect;
        std::unique_ptr<exargs> extra_args;
        SendTo(uint16_t step_, group* targets_, effect* reason_effect_, REASON reason_, PLAYER reason_player_) :
                Process(step_),
                reason_player(reason_player_),
                reason(reason_),
                targets(targets_),
                reason_effect(reason_effect_),
                extra_args(nullptr) {}
    };

    struct Destroy : public Process<false> {
        PLAYER reason_player;
        REASON reason;
        group* targets;
        effect* reason_effect;
        Destroy(uint16_t step_, group* targets_, effect* reason_effect_, REASON reason_, PLAYER reason_player_) :
                Process(step_),
                reason_player(reason_player_),
                reason(reason_),
                targets(targets_),
                reason_effect(reason_effect_) {}
    };

    struct RevealLocation : public Process<false>,
                            reason_data {
        PLAYER playerid;
        RevealLocation(uint16_t step, PLAYER playerid, effect* reason_effect, REASON reason, PLAYER reason_player) :
                Process(step), reason_data(reason, reason_player, reason_effect), playerid(playerid) {}
    };

    struct ReturnLocation : public Process<false> {
        using Process::Process;
    };

    struct ActivateLocation : public Process<false>,
                              reason_data {
        card* pcard;
        PLAYER playerid;
        ActivateLocation(uint16_t step, card* pcard, PLAYER playerid, effect* reason_effect, REASON reason,
                         PLAYER reason_player) :
                Process(step), reason_data(reason, reason_player, reason_effect), pcard(pcard), playerid(playerid) {}
    };

    struct SetMirage : public Process<false>,
                       reason_data {
        PLAYER playerid;
        SetMirage(uint16_t step, PLAYER playerid, effect* reason_effect, REASON reason, PLAYER reason_player) :
                Process(step), playerid(playerid), reason_data(reason, reason_player, reason_effect) {}
    };

    struct Damage : public Process<false>,
                    reason_data {
        card_set targets;
        card* source;
        int32_t amount;
        Damage(uint16_t step, card_set targets, card* source, int32_t amount, reason_data reason) :
                Process(step), reason_data(reason), amount(amount), targets(std::move(targets)), source(source) {}
    };

    struct Recover : public Process<false>,
                     reason_data {
        card_set targets;
        int32_t amount;
        int32_t actual_amount;
        Recover(uint16_t step, card_set targets, int32_t amount, reason_data reason) :
                Process(step), reason_data(reason), amount(amount), targets(std::move(targets)) {}
    };

    struct PhaseEvent : public Process<false> {
        PHASE phase;
        bool is_opponent;
        bool priority_passed;
        PhaseEvent(uint16_t step_, PHASE phase_) :
                Process(step_), phase(phase_), is_opponent(false), priority_passed(false) {}
    };

    struct PointEvent : public Process<false> {
        bool skip_trigger;
        bool skip_freechain;
        bool skip_new;
        PointEvent(uint16_t step_, bool skip_trigger_, bool skip_freechain_, bool skip_new_) :
                Process(step_), skip_trigger(skip_trigger_), skip_freechain(skip_freechain_), skip_new(skip_new_) {}
        PointEvent(uint16_t step) : Process(step), skip_freechain(false), skip_trigger(false), skip_new(false) {}
    };

    struct MoveCommand : public Process<false> {
        card* card_to_move;
        int32_t destination_index = 0;
        explicit MoveCommand(uint16_t step_) : Process(step_), card_to_move(nullptr) {}
    };

    struct Move : public Process<false>,
                  reason_data {
        PLAYER player;
        int32_t source_index, destination_index;
        bool allow_combat;
        bool start_combat = false;
        Move(uint16_t step_, PLAYER player, int32_t source, int32_t destination, bool allow_combat = false,
             reason_data data = reason_data()) :
                Process(step_),
                reason_data(data),
                player(player),
                source_index(source),
                destination_index(destination),
                allow_combat(allow_combat) {}
    };

    struct Combat : public Process<false> {
        PLAYER player;
        int32_t destination_index;
        bool allow_defender;
        std::array<card*, 2> creatures;
        Combat(uint16_t step_, PLAYER player, int32_t destination, bool allow_defender = false) :
                Process(step_), player(player), destination_index(destination), allow_defender(allow_defender) {}
    };

    struct SelectMoveCommand : public Process<true> {
        PLAYER playerid;
        SelectMoveCommand(uint16_t step_, PLAYER playerid) : Process(step_), playerid(playerid) {}
    };
    struct SelectAttackCard : public Process<true> {
        PLAYER playerid;
        SelectAttackCard(uint16_t step_, PLAYER playerid) : Process(step_), playerid(playerid) {}
    };

    struct Debug : public Process<true> {
        using Process::Process;
    };

    using processor_unit = std::variant<Startup, Draw, SendTo, RevealBattlegear, Turn, RevealLocation, ActivateLocation,
                                        SetMirage, Adjust, PhaseEvent, PointEvent, MoveCommand, SelectMoveCommand, Move,
                                        Combat, SelectAttackCard, Damage, Debug, Destroy, Recover, ReturnLocation>;

    template <typename T>
    constexpr inline bool needs_answer = T::needs_response;

    template <typename T, typename... Args>
    constexpr inline void emplace_variant(std::deque<processor_unit>& list, Args&&... args) {
        list.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
    }
} // namespace procs

#endif // CHAOTIC_CORE_PROCESS_UNITS_H
