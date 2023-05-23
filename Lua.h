#ifndef LUA_H_
#define LUA_H_

#include <cstdio>
#include <iostream>

#include "StrFns.h"

#if defined __BUNDLED__
	#if defined __LUA_VERSION_5_3__
		#include "Lua-5.3/src/lua.hpp"
		#include "Lua-5.3/src/lualib.h"
		#include "Lua-5.3/src/lauxlib.h"
	#else // __LUAJIT_VERSION_2_1__
		#include "LuaJIT/src/lua.hpp"
	#endif
#else
	#if defined __LUA_VERSION_x__
		#include "/usr/local/include/lua.hpp"
		#include "/usr/local/include/lualib.h"
		#include "/usr/local/include/lauxlib.h"
	#elif __LUA_VERSION_5_4__
		#include "/usr/local/include/lua54/lua.hpp"
		#include "/usr/local/include/lua54/lualib.h"
		#include "/usr/local/include/lua54/lauxlib.h"
	#elif __LUA_VERSION_5_3__
		#include "/usr/local/include/lua53/lua.hpp"
		#include "/usr/local/include/lua53/lualib.h"
		#include "/usr/local/include/lua53/lauxlib.h"
	#elif __LUA_VERSION_5_2__
		#include "/usr/local/include/lua52/lua.hpp"
		#include "/usr/local/include/lua52/lualib.h"
		#include "/usr/local/include/lua52/lauxlib.h"
	#elif __LUA_VERSION_5_1__
		#include "/usr/local/include/lua51/lua.hpp"
		#include "/usr/local/include/lua51/lualib.h"
		#include "/usr/local/include/lua51/lauxlib.h"
	#elif __LUAJIT_VERSION_2_0__
		#include "/usr/local/include/luajit-2.0/lua.hpp"
	#else //__LUAJIT_VERSION_2_1__
		#include "/usr/local/include/luajit-2.1/lua.hpp"
	#endif
#endif

void process_lua_error(std::string& errStr, int& errLineNo);

struct Lua
{
	bool initialised;
	lua_State* L;

	Lua();
	~Lua();

	void init();
	void reset();
};


#endif //LUA_H_
