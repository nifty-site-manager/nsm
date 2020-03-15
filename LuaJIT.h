#ifndef LUAJIT_H_
#define LUAJIT_H_

#include <cstdio>
#include <iostream>

#include "LuaJIT/src/lua.hpp"
#include "StrFns.h"

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
