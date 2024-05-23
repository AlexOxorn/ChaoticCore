//
// Created by alexoxorn on 5/16/24.
//

#ifndef CHAOTIC_CORE_GROUP_H
#define CHAOTIC_CORE_GROUP_H

#include <set>
#include <vector>
#include "common.h"
#include "card.h"
#include "lua_obj.h"

class match;

class group : public lua_obj_helper<LuaParamType::PARAM_TYPE_GROUP> {
public:
    card_set container;
    card_set::iterator it;
    uint8_t is_readonly{ 0 };
    bool is_iterator_dirty{ true };

    bool has_card(card* c) const {
        return container.find(c) != container.end();
    }

    explicit group(match* pd) : lua_obj_helper(pd) {}
    group(match* pd, card* pcard) : lua_obj_helper(pd), container({ pcard }) {}
    group(match* pd, const card_set& cset) : lua_obj_helper(pd), container(cset) {}
    group(match* pd, card_set&& cset) : lua_obj_helper(pd), container(std::move(cset)) {}
    group(match* pd, group* pgroup) : lua_obj_helper(pd), container(pgroup->container) {}
    template<typename Iter>
    group(match* pd, const Iter begin, const Iter end) : lua_obj_helper(pd), container(begin, end) {}
    group(match* pd, const std::vector<card*>& vcard) : group(pd, vcard.begin(), vcard.end()) {}
    group(match* pd, lua_obj* pobj);
    ~group() = default;
};

#endif // CHAOTIC_CORE_GROUP_H
