#ifndef LUAFNS_H_
#define LUAFNS_H_

#include <cstdio>
#include <iostream>

#include "Consts.h"
#include "ConsoleColor.h"
#include "ExprtkFns.h"
#include "FileSystem.h"
#include "Lua.h"
#include "Path.h"
#include "Quoted.h"
#include "Variables.h"

int lua_nsm_linenumber(lua_State* L);
void lua_nsm_pusherrmsg(lua_State* L, const std::string& errStr);
void lua_nsm_pusherrmsg(lua_State* L, const std::string& errStr, int lineNoOffset);

int lua_cd(lua_State* L);

int lua_sys(lua_State* L);
int lua_sys_bell(lua_State* L);

int lua_exprtk(lua_State* L);
int lua_exprtk_compile(lua_State* L);
int lua_exprtk_eval(lua_State* L);
int lua_exprtk_load(lua_State* L);
int lua_exprtk_str(lua_State* L);

int lua_nsm_lang(lua_State* L);
int lua_nsm_mode(lua_State* L);

int lua_nsm_write(lua_State* L);

int lua_nsm_tonumber(lua_State* L);
int lua_nsm_tostring(lua_State* L);

int lua_nsm_tolightuserdata(lua_State* L);

int lua_nsm_setnumber(lua_State* L);
int lua_nsm_setstring(lua_State* L);

#endif //LUAFNS_H_
