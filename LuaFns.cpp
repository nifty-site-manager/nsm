#include "LuaFns.h"

int lua_nsm_linenumber(lua_State* L)
{
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);
	return ar.currentline;
}

void lua_nsm_pusherrmsg(lua_State* L, const std::string& errStr)
{
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);
	lua_pushstring(L, ("error: " + std::to_string(ar.currentline) + ": " + errStr).c_str());
}

void lua_nsm_pusherrmsg(lua_State* L, const std::string& errStr, int lineNoOffset)
{
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);
	lua_pushstring(L, ("error: " + std::to_string(ar.currentline + lineNoOffset) + ": " + errStr).c_str());
}

int lua_cd(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "cd: expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "cd: parameter is not a string");
		lua_error(L);
		return 0;
	}

	std::string inStr = lua_tostring(L, 1);
	std::string target = replace_home_dir(inStr);
	lua_remove(L, 1);

	if(!dir_exists(target))
	{
		lua_nsm_pusherrmsg(L, "cd: cannot change directory to " + quote(inStr) + " as it is not a directory");
		lua_error(L);
		return 0;
	}

	if(chdir(target.c_str()))
	{
		lua_nsm_pusherrmsg(L, "cd: failed to change directory to " + quote(inStr));
		lua_error(L);
		return 0;
	}

	return 0;
}

int lua_get_pwd(lua_State* L)
{
	if(lua_gettop(L) != 0)
	{
		lua_nsm_pusherrmsg(L, "pwd: expected 0 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	lua_pushstring(L, get_pwd().c_str());

	return 1;
}

int lua_pwd(lua_State* L)
{
	if(lua_gettop(L) != 0)
	{
		lua_nsm_pusherrmsg(L, "pwd: expected 0 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	std::cout << get_pwd() << std::endl;

	return 0;
}

int lua_sys(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "sys: expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "sys: parameter is not a string");
		lua_error(L);
		return 0;
	}

	std::string sys_call = lua_tostring(L, 1);
	lua_remove(L, 1);

	#if defined _WIN32 || defined _WIN64
		if(unquote(sys_call).substr(0, 2) == "./")
			sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
	#else  //*nix
		if(file_exists("/.flatpak-info"))
			sys_call = "flatpak-spawn --host bash -c " + quote(sys_call);
	#endif

	lua_pushnumber(L, system(sys_call.c_str()));
	
	return 1;
}

int lua_sys_bell(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "sys: expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "sys: parameter is not a string");
		lua_error(L);
		return 0;
	}

	std::string sys_call = lua_tostring(L, 1);
	lua_remove(L, 1);

	#if defined _WIN32 || defined _WIN64
		if(unquote(sys_call).substr(0, 2) == "./")
			sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
	#else  //*nix
		if(file_exists("/.flatpak-info"))
			sys_call = "flatpak-spawn --host bash -c " + quote(sys_call);
	#endif

	int result = system(sys_call.c_str());

	if(result) //checks mode
	{
		lua_getglobal(L, "nsm_mode__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk: variable 'nsm_mode__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		int* modeID = (int*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(*modeID == MODE_INTERP || *modeID == MODE_SHELL)
			std::cout << "\a" << std::flush;
	}

	lua_pushnumber(L, result);
	return 1;
}

int lua_exprtk(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "exprtk: expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "exprtk: parameter is not a string");
		lua_error(L);
		return 0;
	}

	std::string exprStr = lua_tostring(L, 1);
	lua_remove(L, 1);

	lua_getglobal(L, "nsm_exprtk__");

	if(!lua_islightuserdata(L, 1))
	{
		lua_nsm_pusherrmsg(L, "exprtk: variable 'nsm_exprtk__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}
	Expr* expr = (Expr*)lua_topointer(L, 1);
	lua_remove(L, 1);

	if(!expr->compile(exprStr))
	{
		//lua_nsm_pusherrmsg(L, "exprtk: failed to compile expression");

		size_t errLineNo;
		for(size_t i=0; i < expr->parser.error_count(); ++i)
		{
			exprtk::parser_error::type error = expr->parser.get_error(i);

			if(std::to_string(error.token.position) == "18446744073709551615")
				errLineNo = 0;
			else
				errLineNo = 0 + std::count(expr->expr_str.begin(), expr->expr_str.begin() + error.token.position, '\n');

			lua_nsm_pusherrmsg(L, "exprtk: " + exprtk::parser_error::to_str(error.mode) + ": " + error.diagnostic, errLineNo);
		}

		lua_error(L);
		return 0;
	}
	
	lua_pushnumber(L, expr->evaluate());
	return 1;
}

int lua_exprtk_compile(lua_State* L)
{
	int noParams = lua_gettop(L);
	if(noParams == 1)
	{
		if(!lua_isstring(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_compile: expression parameter is not a string");
			lua_error(L);
			return 0;
		}

		std::string exprStr = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtk__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_compile: variable 'nsm_exprtk__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		Expr* expr = (Expr*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(!expr->compile(exprStr))
		{
			size_t errLineNo;
			for(size_t i=0; i < expr->parser.error_count(); ++i)
			{
				exprtk::parser_error::type error = expr->parser.get_error(i);

				if(std::to_string(error.token.position) == "18446744073709551615")
					errLineNo = 0;
				else
					errLineNo = 0 + std::count(expr->expr_str.begin(), expr->expr_str.begin() + error.token.position, '\n');

				lua_nsm_pusherrmsg(L, "exprtk_compile: " + exprtk::parser_error::to_str(error.mode) + ": " + error.diagnostic, errLineNo);
			}

			lua_error(L);
			return 0;
		}
	}
	else if(noParams == 2)
	{
		if(!lua_isstring(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_compile: first name parameter is not a string");
			lua_error(L);
			return 0;
		}
		else if(!lua_isstring(L, 2))
		{
			lua_nsm_pusherrmsg(L, "exprtk_compile: second expression parameter is not a string");
			lua_error(L);
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		lua_remove(L, 1);

		std::string exprStr = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtkset__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_compile: variable 'nsm_exprtkset__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		ExprSet* exprset = (ExprSet*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(!exprset->compile(name, exprStr))
		{
			size_t errLineNo;
			for(size_t i=0; i < exprset->parser.error_count(); ++i)
			{
				exprtk::parser_error::type error = exprset->parser.get_error(i);

				if(std::to_string(error.token.position) == "18446744073709551615")
					errLineNo = 0;
				else
					errLineNo = 0 + std::count(exprStr.begin(), exprStr.begin() + error.token.position, '\n');

				lua_nsm_pusherrmsg(L, "exprtk_compile: " + exprtk::parser_error::to_str(error.mode) + ": " + error.diagnostic, errLineNo);
			}

			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "exprtk_compile: expected 1-2 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	return 0;
}

int lua_exprtk_eval(lua_State* L)
{
	int noParams = lua_gettop(L);
	if(noParams == 0)
	{
		lua_getglobal(L, "nsm_exprtk__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_eval: variable 'nsm_exprtk__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		Expr* expr = (Expr*)lua_topointer(L, 1);
		lua_remove(L, 1);

		lua_pushnumber(L, expr->evaluate());
	}
	else if(noParams == 1)
	{
		if(!lua_isstring(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_eval: name parameter is not a string");
			lua_error(L);
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtkset__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_eval: variable 'nsm_exprtkset__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		ExprSet* exprset = (ExprSet*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(!exprset->expressions.count(name))
		{
			lua_nsm_pusherrmsg(L, "exprtk_eval: no expression named " + quote(name) + " has been compiled");
			lua_error(L);
			return 0;
		}

		lua_pushnumber(L, exprset->evaluate(name));
	}
	else
	{
		lua_nsm_pusherrmsg(L, "exprtk_eval: expected 0-1 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	return 1;
}

int lua_exprtk_load(lua_State* L)
{
	if(lua_gettop(L) == 1)
	{
		if(!lua_isstring(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_load: name parameter is not a string");
			lua_error(L);
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtk__");
		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_load: variable 'nsm_exprtk__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		Expr* expr = (Expr*)lua_topointer(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtkset__");
		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_load: variable 'nsm_exprtkset__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		ExprSet* exprset = (ExprSet*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(exprset->expressions.count(name))
		{
			expr->expr_str = exprset->expr_strs[name];
			expr->expression = exprset->expressions[name];
		}
		else
		{
			lua_nsm_pusherrmsg(L, "exprtk_load: no expression named " + quote(name) + " has been compiled");
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "exprtk_load: expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	return 0;
}

int lua_exprtk_str(lua_State* L)
{
	int noParams = lua_gettop(L);
	if(noParams == 0)
	{
		lua_getglobal(L, "nsm_exprtk__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_str: variable 'nsm_exprtk__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		Expr* expr = (Expr*)lua_topointer(L, 1);
		lua_remove(L, 1);

		lua_pushstring(L, expr->expr_str.c_str());
	}
	else if(noParams == 1)
	{
		if(!lua_isstring(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_str: name parameter is not a string");
			lua_error(L);
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_exprtkset__");

		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "exprtk_str: variable 'nsm_exprtkset__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		ExprSet* exprset = (ExprSet*)lua_topointer(L, 1);
		lua_remove(L, 1);

		if(!exprset->expressions.count(name))
		{
			lua_nsm_pusherrmsg(L, "exprtk_str: no expression named " + quote(name) + " has been compiled");
			lua_error(L);
			return 0;
		}

		lua_pushstring(L, exprset->expr_strs[name].c_str());
	}
	else
	{
		lua_nsm_pusherrmsg(L, "exprtk_str: expected 0-1 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	return 1;
}

int lua_nsm_lang(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "nsm_lang(): expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	std::string* nsm_lang_str_ptr;
	char* nsm_lang_ch_ptr;

	lua_getglobal(L, "nsm_lang_str__");
	lua_getglobal(L, "nsm_lang_ch__");
	if(!lua_islightuserdata(L, 2))
	{
		lua_nsm_pusherrmsg(L, "nsm_lang(): variable 'nsm_lang_str__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "nsm_lang(): parameter is not a string");
		lua_error(L);
		return 0;
	}
	else
	{
		nsm_lang_ch_ptr = (char*)lua_topointer(L, 3);
		lua_remove(L, 3);

		nsm_lang_str_ptr = (std::string*)lua_topointer(L, 2);
		lua_remove(L, 2);

		std::string langStr = lua_tostring(L, 1);
		lua_remove(L, 1);

		int pos = langStr.find_first_of("fFnNtTlLeExX", 0);
		if(pos >= 0)
		{
			if(langStr[pos] == 'x' || langStr[pos] == 'X')
			{
				*nsm_lang_str_ptr = "exprtk";
				*nsm_lang_ch_ptr = 'x';
			}
			if(langStr[pos] == 'f' || langStr[pos] == 'F')
			{
				*nsm_lang_str_ptr = "f++";
				*nsm_lang_ch_ptr = 'f';
			}
			else if(langStr[pos] == 'n' || langStr[pos] == 'N' || langStr[pos] == 't' || langStr[pos] == 'T')
			{
				*nsm_lang_str_ptr = "n++";
				*nsm_lang_ch_ptr = 'n';
			}
			else if(langStr[pos] == 'l' || langStr[pos] == 'L')
			{
				*nsm_lang_str_ptr = "lua";
				*nsm_lang_ch_ptr = 'l';
			}

			lua_pushnumber(L, 1);
			return 1;
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_lang(): do not recognise language " + quote(langStr));
			lua_error(L);
			return 0;
		}
	}
}

int lua_nsm_mode(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "nsm_mode(): expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	int* nsm_mode;

	lua_getglobal(L, "nsm_mode__");
	if(!lua_islightuserdata(L, 2))
	{
		lua_nsm_pusherrmsg(L, "nsm_mode(): variable 'nsm_mode__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}
	else if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "nsm_mode(): parameter is not a string");
		lua_error(L);
		return 0;
	}
	else
	{
		nsm_mode = (int*)lua_topointer(L, 2);
		lua_remove(L, 2);

		std::string modeStr = lua_tostring(L, 1);
		lua_remove(L, 1);

		int pos = modeStr.find_first_of("si", 0);
		if(pos >= 0)
		{
			if(modeStr[pos] == 's')
				*nsm_mode = MODE_SHELL;
			else if(modeStr[pos] == 'i')
				*nsm_mode = MODE_INTERP;

			lua_pushnumber(L, 1);
			return 1;
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_mode(): do not recognise mode " + quote(modeStr));
			lua_error(L);
			return 0;
		}
	}
}

int lua_nsm_write(lua_State* L)
{
	int no_params = lua_gettop(L);

	if(no_params < 2)
	{
		lua_nsm_pusherrmsg(L, "nsm_write(): expected 2+ parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	std::string txt;
	for(int p=1; p<no_params; p++)
	{
		if(lua_isnumber(L, 2))
		{
			double val = lua_tonumber(L, 2);
			lua_remove(L, 2);

			if(val == NSM_ENDL)
				txt += "\r\n";
			else
				txt += std::to_string(val);
		}
		else if(lua_isstring(L, 2))
		{
			txt += lua_tostring(L, 2);
			lua_remove(L, 2);
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_write(): parameter number " + std::to_string(p) + " is not a string or a number");
			lua_error(L);
			return 0;
		}
	}

	if(lua_isnumber(L, 1))
	{
		double param = lua_tonumber(L, 1);
		lua_remove(L, 1);

		if(param == NSM_OFILE)
		{
			std::string* indentAmount;
			std::string* parsedText;
			bool indent = 1;

			lua_getglobal(L, "nsm_indentAmount__");
			if(!lua_islightuserdata(L, 1))
			{
				lua_nsm_pusherrmsg(L, "nsm_write(): variable 'nsm_indentAmount__' should be of type 'lightuserdata'");
				lua_error(L);
				return 0;
			}
			indentAmount = (std::string*)lua_topointer(L, 1);
			lua_remove(L, 1);

			lua_getglobal(L, "nsm_parsedText__");
			if(!lua_islightuserdata(L, 1))
			{
				lua_nsm_pusherrmsg(L, "nsm_write(): variable 'nsm_parsedText__' should be of type 'lightuserdata'");
				lua_error(L);
				return 0;
			}
			parsedText = (std::string*)lua_topointer(L, 1);
			lua_remove(L, 1);

			std::istringstream iss(txt);
			std::string ssLine, oldLine;
			int ssLineNo = 0;

			while(!iss.eof())
			{
				getline(iss, ssLine);
				if(0 < ssLineNo++)
					*parsedText += "\n" + *indentAmount;
				oldLine = ssLine;
				*parsedText += ssLine;
			}
			if(indent)
				*indentAmount += into_whitespace(oldLine);

			lua_pushnumber(L, 1);
			return 1;
		}
		else if(param == NSM_CONS)
		{
			bool* consoleLocked;
			std::mutex* os_mtx;

			lua_getglobal(L, "nsm_consoleLocked__");
			if(!lua_islightuserdata(L, 1))
			{
				lua_nsm_pusherrmsg(L, "nsm_write(): variable 'nsm_consoleLocked__' should be of type 'lightuserdata'");
				lua_error(L);
				return 0;
			}
			consoleLocked = (bool*)lua_topointer(L, 1);
			lua_remove(L, 1);

			lua_getglobal(L, "nsm_os_mtx__");
			if(!lua_islightuserdata(L, 1))
			{
				lua_nsm_pusherrmsg(L, "nsm_write(): variable 'nsm_os_mtx__' should be of type 'lightuserdata'");
				lua_error(L);
				return 0;
			}
			os_mtx = (std::mutex*)lua_topointer(L, 1);
			lua_remove(L, 1);

			if(!*consoleLocked)
				os_mtx->lock();
			std::cout << txt;
			if(!*consoleLocked)
				os_mtx->unlock();

			lua_pushnumber(L, 1);
			return 1;
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_write(): not defined for first parameter of type int, value given = " + std::to_string(param));
			lua_error(L);
			return 0;
		}
	}
	else if(lua_isstring(L, 1))
	{
		Variables* vars;
		std::string varName = lua_tostring(L, 1);
		lua_remove(L, 1);

		lua_getglobal(L, "nsm_vars__");
		if(!lua_islightuserdata(L, 1))
		{
			lua_nsm_pusherrmsg(L, "nsm_write(): variable 'nsm_vars__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}
		vars = (Variables*)lua_topointer(L, 1);
		lua_remove(L, 1);

		VPos vpos;
		if(vars->find(varName, vpos))
		{
			if(vpos.type == "fstream")
			{
				vars->layers[vpos.layer].fstreams[varName] << txt;
				lua_pushnumber(L, 1);
				return 1;
			}
			else if(vpos.type == "ofstream")
			{
				vars->layers[vpos.layer].ofstreams[varName] << txt;
				lua_pushnumber(L, 1);
				return 1;
			}
			else
			{
				//write not defined for varName of type vpos.type
				lua_nsm_pusherrmsg(L, "nsm_write(): not defined for first parameter of type " + quote(vpos.type));
				lua_error(L);
				return 0;
			}
		}
		else
		{
			//no variable named varName
			lua_nsm_pusherrmsg(L, "nsm_write(): " + quote(varName) + " not defined");
			lua_error(L);
			return 0;
		}
	}
	else if(lua_islightuserdata(L, 1))
	{
		std::ofstream* ofs = (std::ofstream*)lua_topointer(L, 1);
		lua_remove(L, 1);

		*ofs << txt;

		lua_pushnumber(L, 1);
		return 1;
	}

	return 0;
}

int lua_nsm_tonumber(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "nsm_tonumber(): expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	if(lua_islightuserdata(L, 1))
	{
		double* var = (double*)lua_topointer(L, 1);

		lua_remove(L, 1);
		lua_pushnumber(L, *var);
		return 1;
	}
	else if(lua_isstring(L, 1))
	{
		lua_getglobal(L, "nsm_vars__");

		if(!lua_islightuserdata(L, 2))
		{
			lua_nsm_pusherrmsg(L, "nsm_tonumber(): variable 'nsm_vars__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}

		std::string varName = lua_tostring(L, 1);
		lua_remove(L, 1);
		Variables* vars = (Variables*)lua_topointer(L, 1);
		lua_remove(L, 1);
		VPos vpos;
		double value = 0;

		if(vars->find(varName, vpos))
		{
			if(vars->get_double_from_var(vpos, value))
			{
				lua_pushnumber(L, value);
				return 1;
			}
			else
			{
				lua_nsm_pusherrmsg(L, "nsm_tonumber(): " + quote(varName) + ": failed to get number");
				lua_error(L);
				return 0;
			}
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_tonumber(): " + quote(varName) + ": variable not defined at this scope");
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "nsm_tonumber(): first parameter should either be of type 'lightuserdata' pointing to a variable or of type 'string' giving a variable name");
		lua_error(L);
		return 0;
	}
}

int lua_nsm_tostring(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "nsm_tostring(): expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	if(lua_islightuserdata(L, 1))
	{
		std::string* var = (std::string*)lua_topointer(L, 1);

		lua_remove(L, 1);
		lua_pushstring(L, var->c_str());
		return 1;
	}
	else if(lua_isstring(L, 1))
	{
		lua_getglobal(L, "nsm_vars__");

		if(!lua_islightuserdata(L, 2))
		{
			lua_nsm_pusherrmsg(L, "nsm_tostring(): variable 'nsm_vars__' should be of type 'lightuserdata'");
			lua_error(L);
			return 0;
		}

		std::string varName = lua_tostring(L, 1);
		lua_remove(L, 1);
		Variables* vars = (Variables*)lua_topointer(L, 1);
		lua_remove(L, 1);
		VPos vpos;
		std::string value = "";
		if(vars->find(varName, vpos))
		{
			if(vars->get_str_from_var(vpos, value, 1, 1))
			{
				lua_pushstring(L, value.c_str());
				return 1;
			}
			else
			{
				lua_nsm_pusherrmsg(L, "nsm_tostring(): " + quote(varName) + ": failed to get string");
				lua_error(L);
				return 0;
			}
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_tostring(): " + quote(varName) + ": variable not defined at this scope");
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "nsm_tostring(): first parameter should either be of type 'lightuserdata' pointing to a variable or of type 'string' giving a variable name");
		lua_error(L);
		return 0;
	}
}

int lua_nsm_tolightuserdata(lua_State* L)
{
	if(lua_gettop(L) != 1)
	{
		lua_nsm_pusherrmsg(L, "nsm_vartolightuserdata(): expected 1 parameter, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	lua_getglobal(L, "nsm_vars__");

	if(!lua_isstring(L, 1))
	{
		lua_nsm_pusherrmsg(L, "nsm_vartolightuserdata(): first parameter for variable name should be of type 'string'");
		lua_error(L);
		return 0;
	}
	else if(!lua_islightuserdata(L, 2))
	{
		lua_nsm_pusherrmsg(L, "nsm_vartolightuserdata(): variable 'nsm_vars__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}

	std::string varName = lua_tostring(L, 1);
	lua_remove(L, 1);

	Variables* vars = (Variables*)lua_topointer(L, 1);
	lua_remove(L, 1);

	VPos vpos;
	if(vars->find(varName, vpos))
	{
		if(vpos.type.substr(0, 5) == "std::")
		{
			if(vpos.type == "std::bool")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].bools[varName]);
			else if(vpos.type == "std::int")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].ints[varName]);
			else if(vpos.type == "std::double")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].doubles[varName]);
			else if(vpos.type == "std::char")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].chars[varName]);
			else if(vpos.type == "std::string")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].strings[varName]);
			else if(vpos.type == "std::long long int")
				lua_pushlightuserdata(L, &vars->layers[vpos.layer].llints[varName]);
		}
		else if(vpos.type == "bool" || vpos.type == "int" || vpos.type == "double")
			lua_pushlightuserdata(L, &vars->layers[vpos.layer].doubles[varName]);
		else if(vpos.type == "char" || vpos.type == "string")
			lua_pushlightuserdata(L, &vars->layers[vpos.layer].strings[varName]);
		else if(vpos.type == "ofstream")
			lua_pushlightuserdata(L, &vars->layers[vpos.layer].ofstreams[varName]);
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_vartolightuserdata(): function not defined for variable " + quote(varName) + " of type " + quote(vpos.type));
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "nsm_vartolightuserdata(): " + quote(varName) + ": variable not defined at this scope");
		lua_error(L);
		return 0;
	}

	// return the number of results 
	return 1;
}

int lua_nsm_setnumber(lua_State* L)
{
	if(lua_gettop(L) != 2)
	{
		lua_nsm_pusherrmsg(L, "nsm_setnumber(): expected 2 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	lua_getglobal(L, "nsm_vars__");

	if(!lua_isnumber(L, 2))
	{
		lua_nsm_pusherrmsg(L, "nsm_setnumber(): second parameter for value should be of type 'number'");
		lua_error(L);
		return 0;
	}
	else if(!lua_islightuserdata(L, 3))
	{
		lua_nsm_pusherrmsg(L, "nsm_setnumber(): variable 'nsm_vars__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}

	if(lua_islightuserdata(L, 1))
	{
		double* var = (double*)lua_topointer(L, 1);
		lua_remove(L, 1);
		double value = lua_tonumber(L, 1);
		lua_remove(L, 1);

		*var = value;

		// return the number of results 
		lua_pushnumber(L, 1);
		return 1;
	}
	else if(lua_isstring(L, 1))
	{
		std::string varName = lua_tostring(L, 1);
		lua_remove(L, 1);
		double value = lua_tonumber(L, 1);
		lua_remove(L, 1);
		Variables* vars = (Variables*)lua_topointer(L, 1);
		lua_remove(L, 1);

		VPos vpos;
		if(vars->find(varName, vpos))
		{
			if(vars->set_var_from_double(vpos, value))
				return 0;
			else
			{
				lua_nsm_pusherrmsg(L, "nsm_setnumber(): " + quote(varName) + ": variable assignment failed");
				lua_error(L);
				return 0;
			}
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_setnumber(): " + quote(varName) + ": variable not defined at this scope");
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "nsm_setnumber(): first parameter should either be of type 'lightuserdata' pointing to a variable or of type 'string' giving a variable name");
		lua_error(L);

		return 0;
	}
}

int lua_nsm_setstring(lua_State* L)
{
	if(lua_gettop(L) != 2)
	{
		lua_nsm_pusherrmsg(L, "nsm_setstring(): expected 2 parameters, got " + std::to_string(lua_gettop(L)));
		lua_error(L);
		return 0;
	}

	lua_getglobal(L, "nsm_vars__");

	if(!lua_isstring(L, 2))
	{
		lua_nsm_pusherrmsg(L, "nsm_setstring(): second parameter for value should be of type 'string'");
		lua_error(L);
		return 0;
	}
	else if(!lua_islightuserdata(L, 3))
	{
		lua_nsm_pusherrmsg(L, "nsm_setstring(): variable 'nsm_vars__' should be of type 'lightuserdata'");
		lua_error(L);
		return 0;
	}

	if(lua_islightuserdata(L, 1))
	{
		std::string* var = (std::string*)lua_topointer(L, 1);
		lua_remove(L, 1);
		std::string value = lua_tostring(L, 1);
		lua_remove(L, 1);

		*var = value;

		// return the number of results 
		lua_pushnumber(L, 1);
		return 1;
	}
	else if(lua_isstring(L, 1))
	{
		std::string varName = lua_tostring(L, 1);
		lua_remove(L, 1);
		std::string value = lua_tostring(L, 1);
		lua_remove(L, 1);
		Variables* vars = (Variables*)lua_topointer(L, 1);
		lua_remove(L, 1);

		VPos vpos;
		if(vars->find(varName, vpos))
		{
			if(vars->set_var_from_str(vpos, value))
				return 0;
			else
			{
				lua_nsm_pusherrmsg(L, "nsm_setstring(): " + quote(varName) + ": variable assignment failed");
				lua_error(L);
				return 0;
			}
		}
		else
		{
			lua_nsm_pusherrmsg(L, "nsm_setstring(): " + quote(varName) + ": variable not defined at this scope");
			lua_error(L);
			return 0;
		}
	}
	else
	{
		lua_nsm_pusherrmsg(L, "nsm_setstring(): first parameter should either be of type 'lightuserdata' pointing to a variable or of type 'string' giving a variable name");
		lua_error(L);

		return 0;
	}
}
	
