//
// Created by alexoxorn on 5/16/24.
//

#include "group.h"
group::group(match* pd, lua_obj* pobj) :
        lua_obj_helper(pd)
{
    if(pobj->lua_type == LuaParamType::PARAM_TYPE_CARD)
        container.insert(static_cast<card*>(pobj));
    else if(pobj->lua_type == LuaParamType::PARAM_TYPE_GROUP)
        container = static_cast<group*>(pobj)->container;
}
