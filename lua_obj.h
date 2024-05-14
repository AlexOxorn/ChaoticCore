//
// Created by alexoxorn on 8/18/22.
//

#ifndef CHAOTIC_CORE_LUA_OBJ_H
#define CHAOTIC_CORE_LUA_OBJ_H


#include <cstdint>

enum LuaParamType {
    PARAM_TYPE_INT          = 1 << 0,
    PARAM_TYPE_STRING       = 1 << 1,
    PARAM_TYPE_CARD         = 1 << 2,
    PARAM_TYPE_CREATURE     = 1 << 3,
    PARAM_TYPE_BATTLE_GEAR  = 1 << 4,
    PARAM_TYPE_MUGIC        = 1 << 5,
    PARAM_TYPE_LOCATION     = 1 << 6,
    PARAM_TYPE_GROUP        = 1 << 7,
    PARAM_TYPE_EFFECT       = 1 << 8,
    PARAM_TYPE_FUNCTION     = 1 << 9,
    PARAM_TYPE_BOOLEAN      = 1 << 10,
    PARAM_TYPE_INDEX        = 1 << 11,
    PARAM_TYPE_DELETED      = 1 << 12,
};

class match;

class lua_obj {
public:
    LuaParamType lua_type;
    int32_t ref_handle;
    match* pduel{ nullptr };
protected:
    lua_obj(LuaParamType _lua_type, match* _pduel) :lua_type(_lua_type), pduel(_pduel) {};
};

template<LuaParamType _Type>
class lua_obj_helper : public lua_obj {
    static_assert(
            _Type == PARAM_TYPE_CARD ||
                    _Type == PARAM_TYPE_GROUP ||
                    _Type == PARAM_TYPE_EFFECT ||
                    _Type == PARAM_TYPE_CREATURE ||
                    _Type == PARAM_TYPE_BATTLE_GEAR ||
                    _Type == PARAM_TYPE_MUGIC ||
                    _Type == PARAM_TYPE_DELETED,
                  "Invalid parameter type");
public:
    lua_obj_helper(match* pduel) : lua_obj(_Type, pduel) {};
};


#endif//CHAOTIC_CORE_LUA_OBJ_H
