#include "Lua.h"

void process_lua_error(std::string& errStr, int& errLineNo)
{
	size_t pos = errStr.find_first_of(':');
	if(pos != std::string::npos)
	{
		errStr = errStr.substr(pos+1, errStr.size()-pos-1);
		strip_surrounding_whitespace(errStr);
		pos = errStr.find_first_of(':');

		if(pos != std::string::npos)
		{
			errLineNo += (int)std::strtod(errStr.substr(0, pos).c_str(), NULL) - 1;
			errStr = errStr.substr(pos+1, errStr.size()-pos-1);
			strip_surrounding_whitespace(errStr);
		}
	}
}

Lua::Lua()
{
	initialised = 0;
}

Lua::~Lua()
{
	if(initialised)
	{
		initialised = 0;
		lua_close(L);
	}
}

void Lua::init()
{
	if(!initialised)
	{
		initialised = 1;
		L = luaL_newstate();
		luaL_openlibs(L);
	}
}

void Lua::reset()
{
	if(initialised)
	{
		lua_close(L);
		L = luaL_newstate();
		luaL_openlibs(L);
	}
}
