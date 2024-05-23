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

namespace processors {
    constexpr inline auto restart = static_cast<uint16_t>(~(uint16_t()));

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

    struct Turn : public Process<false> {
        PLAYER turn_player;
        using Process::Process;
        Turn(uint16_t p_step, PLAYER p_turn_player) : Process(p_step) {}
    };

    struct RevealBattlegear : public Process<false> {
        card* battlegear;
        PLAYER reason_player;
        PLAYER playerid;
        REASON reason;
        effect* reason_effect;
        RevealBattlegear(uint16_t step, card* p_battlegear, effect* p_reason_effect, REASON p_reason,
                         PLAYER p_reason_player, PLAYER p_playerid) :
                Process<false>(step),
                battlegear{p_battlegear},
                reason_effect{p_reason_effect},
                reason{p_reason},
                reason_player{p_reason_player},
                playerid{p_playerid} {};
    };

    struct Draw : public Process<false> {
        uint16_t count;
        PLAYER reason_player;
        PLAYER playerid;
        REASON reason;
        effect* reason_effect;
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
                count(count_),
                reason_player(reason_player_),
                playerid(playerid_),
                reason(reason_),
                reason_effect(reason_effect_) {}
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

    struct Debug : public Process<true> {
        using Process::Process;
    };

    using processor_unit = std::variant<Startup, Draw, SendTo, RevealBattlegear, Turn, Debug>;

    template <typename T>
    constexpr inline bool needs_answer = T::needs_response;

    template <typename T, typename... Args>
    constexpr inline void emplace_variant(std::deque<processor_unit>& list, Args&&... args) {
        list.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
    }
} // namespace processors

#endif // CHAOTIC_CORE_PROCESS_UNITS_H
