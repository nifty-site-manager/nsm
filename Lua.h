#ifndef LUAJIT_H_
#define LUAJIT_H_

#include <cstdio>
#include <iostream>

#include "StrFns.h"

#if defined __BUNDLED__
	#if defined __LUA_VERSION_5_3__
	    #include "Lua-5.3/src/lua.hpp"
		#include "Lua-5.3/src/lualib.h"
	    #include "Lua-5.3/src/lauxlib.h"
	#else 
	    #include "LuaJIT/src/lua.hpp"
	#endif
#else
	#if defined __LUA_VERSION_5_3__
		#if defined __FreeBSD__
			#include "/usr/local/include/lua53/lua.hpp"
			#include "/usr/local/include/lua53/lualib.h"
			#include "/usr/local/include/lua53/lauxlib.h"
		#else
			#include "/usr/local/include/lua.hpp"
			#include "/usr/local/include/lualib.h"
			#include "/usr/local/include/lauxlib.h"
		#endif
	#else
		#if defined __FreeBSD__
			#include "/usr/local/include/luajit-2.0/lua.hpp"
		#else
			#include "/usr/local/include/luajit-2.1/lua.hpp"
		#endif
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


#endif //LUAJIT_H_
