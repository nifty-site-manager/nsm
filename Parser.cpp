#include "Parser.h"

std::atomic<long long int> sys_counter;

int find_last_of_special(const std::string& s)
{
    int pos = s.size()-1,
        depth = 0;
    for(; pos>0; --pos) //returning 0 anyway if we reach the end of the for loop
    {
        if(s[pos] == '.' && depth == 0)
            return pos;
        else if(s[pos] == '(' || s[pos] == '[' || s[pos] == '{' || s[pos] == '<')
        {
            if(depth == 1)
                return pos;
            else
                depth--;
        }
        else if(s[pos] == ')' || s[pos] == ']' || s[pos] == '}' || s[pos] == '>')
            depth++;
    }

    return pos;
}

Parser::Parser(std::set<TrackedInfo>* TrackedAll,
                         std::mutex* OS_mtx,
                         const Directory& ContentDir,
                         const Directory& OutputDir,
                         const std::string& ContentExt,
                         const std::string& OutputExt,
                         const std::string& ScriptExt,
                         const Path& DefaultTemplate,
                         const bool& BackupScripts,
                         const std::string& UnixTextEditor,
                         const std::string& WinTextEditor)
{
    //sys_counter = 0; //can't do this here!
    trackedAll = TrackedAll;
    os_mtx = OS_mtx;
    contentDir = ContentDir;
    outputDir = OutputDir;
    contentExt = ContentExt;
    outputExt = OutputExt;
    scriptExt = ScriptExt;
    defaultTemplate = DefaultTemplate;
    backupScripts = BackupScripts;
    consoleLocked = 0;
    unixTextEditor = UnixTextEditor;
    winTextEditor = WinTextEditor;

    mode = -1;
    lolcat = lolcatInit = 0;

    //lua.init(); //don't want this done always

    symbol_table.add_package(basicio_package);
    symbol_table.add_package(fileio_package);
    symbol_table.add_package(vectorops_package);

    //symbol_table.add_stringvar("parsedText", parsedText);
    symbol_table.add_constant("console", NSM_CONS);
    symbol_table.add_constant("endl", NSM_ENDL);
    symbol_table.add_constant("ofile", NSM_OFILE);

    symbol_table.add_function("cd", exprtk_cd_fn);

    exprtk_sys_fn.setModePtr(&mode);
    symbol_table.add_function("sys", exprtk_sys_fn);

    exprtk_nsm_tonumber_fn.setVars(&vars);
    symbol_table.add_function("nsm_tonumber", exprtk_nsm_tonumber_fn);
    exprtk_nsm_tostring_fn.setVars(&vars);
    symbol_table.add_function("nsm_tostring", exprtk_nsm_tostring_fn);

    exprtk_nsm_setnumber_fn.setVars(&vars);
    symbol_table.add_function("nsm_setnumber", exprtk_nsm_setnumber_fn);
    exprtk_nsm_setstring_fn.setVars(&vars);
    symbol_table.add_function("nsm_setstring", exprtk_nsm_setstring_fn);

    exprtk_nsm_write_fn.add_info(&vars, &parsedText, &indentAmount, &consoleLocked, OS_mtx);
    symbol_table.add_function("nsm_write", exprtk_nsm_write_fn);

    expr.register_symbol_table(symbol_table);
    exprset.add_symbol_table(&symbol_table);

    expr.set_expr("1");
    exprset.add_expr("1");
}

int Parser::lua_addnsmfns()
{
    lua_register(lua.L, "cd", lua_cd);
    if(mode == MODE_INTERP || mode == MODE_SHELL)
        lua_register(lua.L, "sys", lua_sys_bell);
    else
        lua_register(lua.L, "sys", lua_sys);
    lua_register(lua.L, "exprtk", lua_exprtk);
    lua_register(lua.L, "nsm_write", lua_nsm_write);
    lua_register(lua.L, "nsm_tonumber", lua_nsm_tonumber);
    lua_register(lua.L, "nsm_tostring", lua_nsm_tostring);
    lua_register(lua.L, "nsm_tolightuserdata", lua_nsm_tolightuserdata);
    lua_register(lua.L, "nsm_setnumber", lua_nsm_setnumber);
    lua_register(lua.L, "nsm_setstring", lua_nsm_setstring);
    lua_pushlightuserdata(lua.L, &mode);
    lua_setglobal(lua.L, "nsm_mode__");
    lua_pushlightuserdata(lua.L, &expr);
    lua_setglobal(lua.L, "nsm_exprtk__");
    lua_pushlightuserdata(lua.L, &vars);
    lua_setglobal(lua.L, "nsm_vars__");
    lua_pushlightuserdata(lua.L, os_mtx);
    lua_setglobal(lua.L, "nsm_os_mtx__");
    lua_pushlightuserdata(lua.L, &parsedText);
    lua_setglobal(lua.L, "nsm_parsedText__");
    lua_pushlightuserdata(lua.L, &indentAmount);
    lua_setglobal(lua.L, "nsm_indentAmount__");
    lua_pushlightuserdata(lua.L, &consoleLocked);
    lua_setglobal(lua.L, "nsm_consoleLocked__");

    lua_pushnumber(lua.L, NSM_CONS);
    lua_setglobal(lua.L, "console");
    lua_pushnumber(lua.L, NSM_ENDL);
    lua_setglobal(lua.L, "endl");
    lua_pushnumber(lua.L, NSM_OFILE);
    lua_setglobal(lua.L, "ofile");

    return 1;
}

int Parser::lolcat_init()
{
    std::string lsCmd;
    #if defined _WIN32 || defined _WIN64
        lsCmd = "dir | ";
    #else
        lsCmd = "ls | ";
    #endif

    std::string suppressor;
    #if defined _WIN32 || defined _WIN64
        suppressor = " >nul 2>&1";
    #else  //*nix
        suppressor = " > /dev/null 2>&1";
    #endif

    if(!system((lsCmd + "lolcat-cc" + suppressor).c_str()))
    {
        #if defined _WIN32 || defined _WIN64
            if(using_powershell_colours())
                lolcatCmd = "lolcat-cc -ps -t";
            else
                lolcatCmd = "lolcat-cc -t";
        #else
            lolcatCmd = "lolcat-cc -t";
        #endif
    }
    else if(!system((lsCmd + "lolcat-c" + suppressor).c_str()))
        lolcatCmd = "lolcat-c";
    else if(!system((lsCmd + "lolcat-rs" + suppressor).c_str()))
        lolcatCmd = "lolcat-rs";
    else if(!system((lsCmd + "lolcat" + suppressor).c_str()))
        lolcatCmd = "lolcat";
    else if(!system((lsCmd + "nift lolcat" + suppressor).c_str()))
    {
        #if defined _WIN32 || defined _WIN64
            if(using_powershell_colours())
                lolcatCmd = "nift lolcat -ps";
            else
                lolcatCmd = "nift lolcat";
        #else
            lolcatCmd = "nift lolcat";
        #endif
    }
    else
        return 0;

    lolcatInit = 1;

    return 1;
}

int Parser::run_script(std::ostream& os, const Path& scriptPath, const bool& makeBackup, const bool& outputWhatDoing)
{
    if(file_exists(scriptPath.str()))
    {
        if(outputWhatDoing)
            os << "running " << scriptPath << ".." << std::endl;

        int result;
        int c = sys_counter++;
        size_t pos = scriptPath.str().substr(1, scriptPath.str().size()-1).find_first_of('.');
        std::string cScriptExt = "";
        if(pos != std::string::npos)
            cScriptExt = scriptPath.str().substr(pos+1, scriptPath.str().size()-pos-1);

        if(cScriptExt == ".f")
        {
            std::string toProcess = string_from_file(scriptPath.str()),
                        parsedText;
            std::set<Path> antiDepsOfReadPath;
            int lineNo = 0;

            if(toProcess.substr(0, 2) == "#!")
            {
                strip_leading_line(toProcess);
                ++lineNo;
            }

            result = f_read_and_process_fast(0, toProcess, lineNo, scriptPath, antiDepsOfReadPath, parsedText, os);

            if(!is_whitespace(parsedText))
            {
                os_mtx->lock();
                os << parsedText;
                os_mtx->unlock();
            }
        }
        else if(cScriptExt == ".n")
        {
            std::string toProcess = string_from_file(scriptPath.str()),
                        parsedText;
            std::set<Path> antiDepsOfReadPath;
            int lineNo = 0;

            if(toProcess.substr(0, 2) == "#!")
            {
                strip_leading_line(toProcess);
                ++lineNo;
            }

            result = n_read_and_process_fast(1, 0, toProcess, lineNo, scriptPath, antiDepsOfReadPath, parsedText, os);

            if(!is_whitespace(parsedText))
            {
                os_mtx->lock();
                os << parsedText;
                os_mtx->unlock();
            }
        }
        else if(cScriptExt == ".lua")
        {
            if(!lua.initialised)
            {
                lua.init();
                lua_addnsmfns();
            }

            std::string toProcess = string_from_file(scriptPath.str());
            int lineNo = 1;

            if(toProcess.substr(0, 2) == "#!")
            {
                strip_leading_line(toProcess);
                ++lineNo;
            }

            result = luaL_dostring(lua.L, toProcess.c_str());

            if(result)
            {
                std::string errStr = lua_tostring(lua.L, -1);
                int errLineNo = lineNo;
                process_lua_error(errStr, errLineNo);

                if(!consoleLocked)
                    os_mtx->lock();
                start_err(os, scriptPath, errLineNo) << "lua: " << errStr << std::endl;
                os_mtx->unlock();
            }
        }
        else if(cScriptExt == ".exprtk")
        {
            std::string toProcess = string_from_file(scriptPath.str());
            int lineNo = 1;

            if(toProcess.substr(0, 2) == "#!")
            {
                strip_leading_line(toProcess);
                ++lineNo;
            }

            if(!expr.set_expr(toProcess))
            {
                result = 1;
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(os, scriptPath) << "exprtk: failed to compile script" << std::endl;
                print_exprtk_parser_errs(os, expr.parser, toProcess, scriptPath, lineNo);
                os_mtx->unlock();
            }
            else
            {
                result = 0;
                expr.evaluate();
            }
        }
        else
        {
            std::string execPath = "./.script" + std::to_string(c) + cScriptExt;
            std::string output_filename = ".script.out" + std::to_string(c);
            //std::string output_filename = scriptPath.str() + ".out" + std::to_string(c);

            #if defined _WIN32 || defined _WIN64
                if(unquote(execPath).substr(0, 2) == "./")
                    execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
            #else  //*nix
            #endif

            //copies script to backup location
            if(makeBackup)
            {
                if(cpFile(scriptPath.str(), scriptPath.str() + ".backup", -1, Path("", ""), os, consoleLocked, os_mtx))
                {
                    os_mtx->lock();
                    start_err(os) << "run_script(" << scriptPath << "): failed to copy " << scriptPath << " to " << quote(scriptPath.str() + ".backup") << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            //moves script to main directory
            //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
            //sometimes this fails (on Windows) for some reason, so keeps trying until successful
            int mcount = 0;
            while(rename(scriptPath.str().c_str(), execPath.c_str()))
            {
                if(++mcount == 100)
                {
                    os_mtx->lock();
                    start_warn(os) << "run_script(" << scriptPath << "): have tried to move " << scriptPath << " to " << quote(execPath) << " 100 times already, may need to abort" << std::endl;
                    os_mtx->unlock();
                }
            }

            //checks whether we're running from flatpak
            if(file_exists("/.flatpak-info"))
                result = system(("flatpak-spawn --host bash -c " + quote(execPath) + " > " + output_filename).c_str());
            else
                result = system((execPath + " > " + output_filename).c_str());

            //moves script back to its original location
            //sometimes this fails (on Windows) for some reason, so keeps trying until successful
            mcount = 0;
            while(rename(execPath.c_str(), scriptPath.str().c_str()))
            {
                if(++mcount == 100)
                {
                    os_mtx->lock();
                    start_warn(os) << "run_script(" << scriptPath << "): have tried to move " << quote(execPath) << " to " << scriptPath << " 100 times already, may need to abort" << std::endl;
                    os_mtx->unlock();
                }
            }

            //deletes backup copy
            if(makeBackup)
                remove_file(Path("", scriptPath.str() + ".backup"));

            std::string str = string_from_file(output_filename);
            if(!is_whitespace(str))
            {
                os_mtx->lock();
                os << str << std::endl;
                os_mtx->unlock();
            }
            remove_file(Path("./", output_filename));
        }

        if(result)
        {
            os_mtx->lock();
            start_err(os) << "run_script(" << scriptPath << "): script failed" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    return 0;
}

std::string get_env(const char* name)
{
    const char* value = std::getenv(name);
    if(value != NULL)
        return std::string(value);
    return "";
}

int Parser::refresh_completions()
{
    std::string path = get_env("PATH");
    std::string pathDir;
    std::vector<std::string> pathDirFiles;
    size_t sLinePos;

    for(size_t linePos = 0; linePos < path.size(); ++linePos)
    {
        sLinePos = linePos;
        linePos = path.find_first_of(":;", sLinePos);
        if(linePos == std::string::npos)
            break;
        pathDir = path.substr(sLinePos, linePos-sLinePos);

        if(dir_exists(pathDir))
        {
            pathDirFiles = lsVec(pathDir.c_str());
            for(size_t j=0; j<pathDirFiles.size(); ++j)
                tabCompletionStrs.push_back(pathDirFiles[j]);
        }
    }

    return 0;
}

int Parser::shell(std::string& lang, std::ostream& eos)
{
    mode = MODE_SHELL;
    return interactive(lang, eos);
}

int Parser::interpreter(std::string& lang, std::ostream& eos)
{
    mode = MODE_INTERP;
    return interactive(lang, eos);
}

int Parser::interactive(std::string& lang, std::ostream& eos)
{
    bool addPath;
    vars.precision = 6;
    vars.fixedPrecision = vars.scientificPrecision = 0;

    if(lang != "f++" && lang != "n++" && lang != "lua" && lang != "exprtk")
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(mode == MODE_INTERP)
            start_err(eos) << "interp: unknown language " << c_light_blue << lang << c_white << std::endl;
        else
            start_err(eos) << "sh: unknown language " << c_light_blue << lang << c_white << std::endl;
        os_mtx->unlock();
        return 1;
    }

    depFiles.clear();
    includedFiles.clear();

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    parsedText = "";
    contentAdded = 0;

    symbol_table.add_package(basicio_package);
    symbol_table.add_package(fileio_package);
    symbol_table.add_package(vectorops_package);

    exprtk_nsm_lang<double> exprtk_nsm_lang_fn;
    exprtk_nsm_lang_fn.setLangPtr(&lang);
    symbol_table.add_function("nsm_lang", exprtk_nsm_lang_fn);
    exprtk_nsm_mode<double> exprtk_nsm_mode_fn;
    exprtk_nsm_mode_fn.setModePtr(&mode);
    symbol_table.add_function("nsm_mode", exprtk_nsm_mode_fn);

    if(!lua.initialised)
    {
        lua.init();
        lua_addnsmfns();

        lua_pushlightuserdata(lua.L, &lang);
        lua_setglobal(lua.L, "nsm_lang__");
        lua_register(lua.L, "nsm_lang", lua_nsm_lang);
        lua_register(lua.L, "nsm_mode", lua_nsm_mode);
    }

    Path emptyPath("", "");

    //creates anti-deps set
    std::set<Path> antiDepsOfReadPath;

    refresh_completions();

    int result = 1;
    while(1)
    {
        std::string toProcess, inLine;
        parsedText = "";

        if(mode == MODE_SHELL) 
            addPath = 1;
        else
            addPath = 0;

        int netBrackets = 0;
        result = getline(lang, addPath, '$', lolcat, inLine, 1, tabCompletionStrs);
        for(size_t i=0; i<inLine.size(); ++i)
        {
            if(inLine[i] == '(' || inLine[i] == '{' || inLine[i] == '[')
                ++netBrackets;
            else if(inLine[i] == ')' || inLine[i] == '}' || inLine[i] == ']')
                --netBrackets;
        }
        if(result == NSM_KILL)
            return NSM_KILL;

        while(netBrackets || 
              result == NSM_SENTER || 
              (inLine.size() && inLine[inLine.size()-1] == '\\'))
        {
            if(inLine.size() && result != NSM_SENTER && netBrackets && inLine[inLine.size()-1] == '=')
            {
                inLine = inLine.substr(0, inLine.size()-1) + "\n";
                break;
            }
            else if(inLine.size() && inLine[inLine.size()-1] == '\\')
                toProcess += inLine.substr(0, inLine.size()-1) + "\n";
            else
                toProcess += inLine + "\n";

            inLine = "";
            result = getline(lang, addPath, '>', lolcat, inLine, 1, tabCompletionStrs);
            for(size_t i=0; i<inLine.size(); ++i)
            {
                if(inLine[i] == '(' || inLine[i] == '{' || inLine[i] == '[' || inLine[i] == '<')
                    ++netBrackets;
                else if(inLine[i] == ')' || inLine[i] == '}' || inLine[i] == ']' || inLine[i] == '>')
                    --netBrackets;
            }
            if(result == NSM_KILL)
                return NSM_KILL;
        }
        toProcess += inLine;

        indentAmount = "";

        if(lang[0] == 'n')
            result = n_read_and_process_fast(1, 0, toProcess, 0, emptyPath, antiDepsOfReadPath, parsedText, eos);
        else if(lang[0] == 'f')
            result = f_read_and_process_fast(0, toProcess, 0, emptyPath, antiDepsOfReadPath, parsedText, eos);
        else if(toProcess == "exit" || toProcess == "quit")
            result = NSM_QUIT;
        else if(lang[0] == 'e')
        {
            if(!exprset.add_expr(toProcess))
            {
                result = 1;
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, emptyPath, 1) << c_light_blue << "exprtk" << c_white << ": failed to compile expression" << std::endl;
                print_exprtk_parser_errs(eos, exprset.parser, toProcess, emptyPath, 1);
                if(!consoleLocked)
                    os_mtx->unlock();
            }
            else
                result = exprset.evaluate(toProcess);
        }
        else if(lang[0] == 'l')
        {
            result = luaL_dostring(lua.L, toProcess.c_str());

            if(result)
            {
                std::string errStr = lua_tostring(lua.L, -1);
                lua_remove(lua.L, 1);
                int errLineNo = 1;
                process_lua_error(errStr, errLineNo);

                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, emptyPath, errLineNo) << "lua: " << errStr << std::endl;
                os_mtx->unlock();
            }
        }

        if(result == NSM_KILL)
            return result;
        else if(result == NSM_QUIT || result == NSM_EXIT)
            break;
        else if(result == LANG_NPP)
            lang = "n++";
        else if(result == LANG_FPP)
            lang = "f++";
        else if(result == LANG_LUA)
            lang = "lua";
        else if(result == LANG_EXPRTK)
            lang = "exprtk";
        else if(result == MODE_SHELL)
        {
            mode = MODE_SHELL;
            addPath = 1;
        }
        else if(result == MODE_INTERP)
        {
            mode = MODE_INTERP;
            addPath = 0;
        }
        else if(parsedText != "")
        {
            if(lolcat)
                rnbwcout(parsedText);
            else
                std::cout << parsedText << std::endl;
        }
    }

    vars = Variables();

    if(consoleLocked && !result)
    {
        if(!result)
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, consoleLockedPath, consoleLockedOnLine) << "console.lock() called with no following console.unlock()" << std::endl;
            result = 1;
        }
        consoleLocked = 0;
        os_mtx->unlock();
        return result;
    }

    return result;
}

int Parser::run(const Path& path, char lang, std::ostream& eos)
{
    mode = MODE_RUN;
    vars.precision = 6;
    vars.fixedPrecision = vars.scientificPrecision = 0;

    if(!file_exists(path.str()))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos) << "run: cannot run " << c_light_blue << path << c_white << " as file does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    if(lang == '?')
    {
        int pos = path.file.find_first_of('.');
        std::string ext = path.file.substr(pos, path.file.size()-pos);

        if(ext.find_first_of('f') != std::string::npos)
            lang = 'f';
        else if(ext.find_first_of('n') != std::string::npos)
            lang = 'n';
        if(ext.find_first_of('l') != std::string::npos)
            lang = 'l';
        else if(ext.find_first_of('x') != std::string::npos)
            lang = 'x';
        else //if(ext.find_first_of('f') != std::string::npos)
            lang = 'f';
        /*else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos) << "run: cannot determine chosen language from extension " << quote(ext) << ", ";
            eos << "valid options include '.n', '.f', '.exprtk', '.lua', ";
            eos << "or you can specify the language using eg. `nsm run -n++ " << path << "`" << std::endl;
            os_mtx->unlock();
            return 1;
        }*/
    }

    depFiles.clear();
    includedFiles.clear();

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    parsedText = "";
    contentAdded = 0;

    //adds content and template paths to dependencies
    depFiles.insert(path);

    //opens up path file to start parsing from
    int lineNo = 1;
    std::string scriptStr = string_from_file(path.str());

    if(scriptStr.substr(0, 2) == "#!")
    {
        strip_leading_line(scriptStr);
        ++lineNo;
    }

    //creates anti-deps of template set
    std::set<Path> antiDepsOfReadPath;

    //starts read_and_process from path file
    int result = 1;

    if(lang == 'n')
        result = n_read_and_process_fast(1, 0, scriptStr, lineNo-1, path, antiDepsOfReadPath, parsedText, eos);
    else if(lang == 'f')
        result = f_read_and_process_fast(0, scriptStr, lineNo-1, path, antiDepsOfReadPath, parsedText, eos);
    else if(lang == 'l')
    {
        lua.init();

        result = luaL_dostring(lua.L, scriptStr.c_str());

        if(result)
        {
            std::string errStr = lua_tostring(lua.L, -1);
            int errLineNo = lineNo;
            process_lua_error(errStr, errLineNo);

            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, path, errLineNo) << "lua: " << errStr << std::endl;
            os_mtx->unlock();
        }
    }
    else if(lang == 'x')
    {
        std::string strippedScriptStr = scriptStr;
        strip_surrounding_whitespace_multiline(strippedScriptStr);
        if(strippedScriptStr != "")
        {
            if(!expr.set_expr(scriptStr))
            {
                result = 1;
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, path) << "exprtk: failed to compile script" << std::endl;
                print_exprtk_parser_errs(eos, expr.parser, scriptStr, path, lineNo);
                os_mtx->unlock();
            }
            else
                expr.evaluate();
        }
    }

    /*std::cout << "----" << std::endl;
    std::cout << parsedText << std::endl;
    std::cout << "----" << std::endl;*/

    vars = Variables();

    if(consoleLocked)
    {
        if(result == NSM_RET || !result)
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, consoleLockedPath, consoleLockedOnLine) << "console.lock() called with no following console.unlock()" << std::endl;
            result = 1;
        }
        consoleLocked = 0;
        os_mtx->unlock();
        return result;
    }

    if(result == NSM_RET && isDouble(parsedText))
        return std::strtod(parsedText.c_str(), NULL);

    return result;
}

int Parser::build(const TrackedInfo& ToBuild,
                  std::atomic<double>& noPagesFinished,
                  std::atomic<int>& noPagesToBuild,
                  std::ostream& eos)
{
    mode = MODE_BUILD;
    sys_counter = sys_counter%1000000000000000;
    toBuild = ToBuild;
    vars.precision = 6;
    vars.fixedPrecision = vars.scientificPrecision = 0;
    bool blankTemplate = 0;
    if(toBuild.templatePath.str() == "")
        blankTemplate = 1;

    //ensures content and template files exist
    if(!file_exists(toBuild.contentPath.str()))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos) << "cannot build " << toBuild.outputPath << " as content file " << toBuild.contentPath << " does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    if(!blankTemplate && !file_exists(toBuild.templatePath.str()))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos) << "cannot build " << toBuild.outputPath << " as template file " << toBuild.templatePath << " does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    depFiles.clear();
    includedFiles.clear();

    //checks for non-default script extension
    Path extPath = toBuild.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

    std::string cScriptExt;
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, cScriptExt);
        ifs.close();
    }
    else
        cScriptExt = scriptExt;

    //checks for pre-build scripts
    if(file_exists("pre-build" + scriptExt))
        depFiles.insert(Path("", "pre-build" + scriptExt));
    Path prebuildScript = toBuild.contentPath;
    prebuildScript.file = prebuildScript.file.substr(0, prebuildScript.file.find_first_of('.')) + "-pre-build" + cScriptExt;
    if(file_exists(prebuildScript.str()))
        depFiles.insert(prebuildScript);
    if(run_script(eos, prebuildScript, backupScripts, 0))
        return 1;

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    parsedText = "";
    contentAdded = 0;

    pagesInfo.reset();

    //sets pagesDir for pagination
    pagesInfo.pagesDir = toBuild.outputPath.str();
    size_t pos = pagesInfo.pagesDir.find_first_of('.');
    pagesInfo.pagesDir = pagesInfo.pagesDir.substr(0, pos) + "/";

    //checks number of pagination pages from previous build
    Path paginationPath = toBuild.outputPath.getPaginationPath();
    std::string paginationPathStr = paginationPath.str();
    size_t oldNoPaginationPages = 0;
    if(file_exists(paginationPathStr))
    {
        std::string str;
        std::ifstream ifs(paginationPathStr);
        ifs >> str >> oldNoPaginationPages;
        ifs.close();
    }

    //adds content and template paths to dependencies
    depFiles.insert(toBuild.contentPath);
    if(!blankTemplate)
        depFiles.insert(toBuild.templatePath);

    //opens up template file to start parsing from
    std::string fileStr;
    if(blankTemplate)
    {
        contentAdded = 1;
        fileStr = string_from_file(toBuild.contentPath.str());
    }
    else
        fileStr = string_from_file(toBuild.templatePath.str());

    //creates anti-deps of template set
    std::set<Path> antiDepsOfReadPath;

    //starts read_and_process from templatePath
    int result = n_read_and_process_fast(1, fileStr, 0, toBuild.templatePath, antiDepsOfReadPath, parsedText, eos);

    //pagination
    size_t noItems = pagesInfo.items.size();
    if((result == NSM_RET || !result) && noItems)
    {
        pagesInfo.noPages = std::ceil((double)noItems/(double)pagesInfo.noItemsPerPage);
        noPagesToBuild += pagesInfo.noPages;

        //creates/updates pagination file
        chmod(paginationPathStr.c_str(), 0666);
        std::ofstream ofs(paginationPathStr);
        ofs << "noPages " << pagesInfo.noPages << "\n";
        ofs.close();
        //makes sure user can't accidentally write to pagination file
        chmod(paginationPathStr.c_str(), 0444); 

        //creates pages
        Path(pagesInfo.pagesDir, "").ensureDirExists();

        pos = parsedText.find("__paginate_here__");
        if(pos == std::string::npos)
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos) << c_light_blue << quote(toBuild.name) << c_white << ": pagination items added with no paginate call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        parsedText.replace(pos, 17, "");
        pagesInfo.splitFile = std::pair<std::string, std::string>(parsedText.substr(0, pos), parsedText.substr(pos, parsedText.size()-pos));
        pagesInfo.indentAmount = "";
        for(int p=pos-1; p>=0 && parsedText[p] != '\n'; --p)
        {
            if(parsedText[p] == '\t')
                pagesInfo.indentAmount += "\t";
            else
                pagesInfo.indentAmount += " ";
        }

        Directory outputDirBackup = outputDir;
        Path outputPathBackup = toBuild.outputPath;

        std::string page;
        int i=0, I, iStart;
        for(size_t p=0; p<pagesInfo.noPages; ++p)
        {
            page = "";

            iStart = i;
            I = std::min(noItems, (p+1)*pagesInfo.noItemsPerPage);
            for(; i<I; ++i)
            {
                if(i != iStart)
                {
                    page += pagesInfo.separator;
                    outputDir = pagesInfo.pagesDir;
                    toBuild.outputPath = Path(pagesInfo.pagesDir, std::to_string(p+1) + outputExt);
                }
                if(parse_replace('n', pagesInfo.items[i], "paginate item", pagesInfo.itemCallPaths[i], antiDepsOfReadPath, pagesInfo.itemLineNos[i], "item", pagesInfo.itemCallLineNos[i], eos))
                    return 1;
                page += pagesInfo.items[i];
            }

            pagesInfo.pages.push_back(page);
            noPagesFinished = noPagesFinished + 0.05;
        }
        pagesInfo.items.clear();

        for(size_t p=pagesInfo.noPages; p<oldNoPaginationPages; ++p)
        {
            Path removePath(pagesInfo.pagesDir, std::to_string(p+1) + outputExt);
            chmod(removePath.str().c_str(), 0666);
            remove_file(removePath);
        }

        outputDir = outputDirBackup;
        toBuild.outputPath = outputPathBackup;

        /*os_mtx->lock();
        std::cout << "noItems: " << noItems << std::endl;
        std::cout << "pagesInfo.noPages: " << pagesInfo.noPages << std::endl;
        std::cout << "pagesInfo.noItemsPerPage: " << pagesInfo.noItemsPerPage << std::endl;

        std::cout << "pagesInfo.pagesDir: " << quote(pagesInfo.pagesDir) << std::endl;
        std::cout << "outputExt: " << quote(outputExt) << std::endl;
        std::cout << "pagesInfo.pages.size(): " << pagesInfo.pages.size() << std::endl;
        os_mtx->unlock();*/
    }
    else if(oldNoPaginationPages)
    {
        chmod(paginationPathStr.c_str(), 0666);
        remove_file(paginationPath);
        rmdir(pagesInfo.pagesDir.c_str()); 

        for(size_t p=0; p<oldNoPaginationPages; ++p)
        {
            Path removePath(pagesInfo.pagesDir, std::to_string(p+1) + outputExt);
            chmod(removePath.str().c_str(), 0666);
            remove_file(removePath);
        }
    }

    vars = Variables();

    //deconstructs and lua (if initialised) (has low initialise time, high deconstruct/reset time)
    lua.~Lua();

    //symbol_table.clear_functions();
    symbol_table.clear_local_constants();
    symbol_table.clear_strings();
    symbol_table.clear_variables();
    symbol_table.clear_vectors();
    symbol_table.add_stringvar("parsedText", parsedText);

    if(consoleLocked)
    {
        if(result == NSM_RET || !result)
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, consoleLockedPath, consoleLockedOnLine) << "console.lock() called with no following console.unlock()" << std::endl;
            result = 1;
        }
        consoleLocked = 0;
        os_mtx->unlock();
        return 1;
    }

    if(result == NSM_RET || !result)
    {
        //ensures @inputcontent was found inside template dag
        if(!contentAdded)
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos) << "content file " << toBuild.contentPath << " has not been used as a dependency within template file " << toBuild.templatePath << " or any of its dependencies" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        if(!noItems)
        {
            //makes sure directory for output file exists
            toBuild.outputPath.ensureDirExists();

            //makes sure we can write to output file
            chmod(toBuild.outputPath.str().c_str(), 0644);

            //writes processed text to output file
            std::ofstream outputFile(toBuild.outputPath.str());
            outputFile << parsedText << "\n";
            outputFile.close();

            //makes sure user can't accidentally write to output file
            chmod(toBuild.outputPath.str().c_str(), 0444);
        }

        //checks for post-build scripts
        if(file_exists("post-build" + scriptExt))
            depFiles.insert(Path("", "post-build" + scriptExt));
        Path postbuildScript = toBuild.contentPath;
        postbuildScript.file = postbuildScript.file.substr(0, postbuildScript.file.find_first_of('.')) + "-post-build" + cScriptExt;
        if(file_exists(postbuildScript.str()))
            depFiles.insert(postbuildScript);
        if(run_script(eos, postbuildScript, backupScripts, 0))
            return 1; //should an output file be listed as failing to build if the post-build script fails?

        //gets path for storing info file
        Path infoPath = toBuild.outputPath.getInfoPath();

        //makes sure info file exists
        infoPath.ensureDirExists();

        //makes sure we can write to info file_
        chmod(infoPath.str().c_str(), 0644);

        //writes info file
        std::ofstream infoStream(infoPath.str());
        infoStream << dateTimeInfo.currentTime() << " " << dateTimeInfo.currentDate() << "\n";
        //what if files are modified mid-build? at worst files will be rebuilt next build, but could be annoying for larger projects
        //infoStream << dateTimeInfo.cTime << " " << dateTimeInfo.cDate << "\n";
        infoStream << toBuild << "\n\n";
        for(auto depFile=depFiles.begin(); depFile != depFiles.end(); depFile++)
            infoStream << *depFile << "\n";
        infoStream.close();

        //makes sure user can't accidentally write to info file
        chmod(infoPath.str().c_str(), 0444);
    }

    return result;
}

int Parser::n_read_and_process(const bool& indent,
                                  const std::string& inStr,
                                  int lineNo,
                                  const Path& readPath,
                                  std::set<Path> antiDepsOfReadPath,
                                  std::string& outStr,
                                  std::ostream& eos)
{
    //adds read path to anti dependencies of read path
    if(file_exists(readPath.str()))
        antiDepsOfReadPath.insert(readPath);

    return n_read_and_process_fast(indent, 1, inStr, lineNo, readPath, antiDepsOfReadPath, outStr, eos);
}

int Parser::n_read_and_process_fast(const bool& indent,
                                  const std::string& inStr,
                                  int lineNo,
                                  const Path& readPath,
                                  std::set<Path>& antiDepsOfReadPath,
                                  std::string& outStr,
                                  std::ostream& eos)
{
    return n_read_and_process_fast(indent, 1, inStr, lineNo, readPath, antiDepsOfReadPath, outStr, eos);
}

int Parser::n_read_and_process_fast(const bool& indent,
                                  const bool& addOutput,
                                  const std::string& inStr,
                                  int lineNo,
                                  const Path& readPath,
                                  std::set<Path>& antiDepsOfReadPath,
                                  std::string& outStr,
                                  std::ostream& eos)
{
    std::string baseIndentAmount = indentAmount;
    std::string beforePreBaseIndentAmount;
    if(!indent) // not sure if this is needed?
        baseIndentAmount = "";
    int baseCodeBlockDepth = codeBlockDepth;

    int openCodeLineNo = 0;
    bool firstLine = 1, lastLine = 0;
    size_t linePos = 0;
    while(linePos < inStr.size())
    {
        lineNo++;
        if(!firstLine) //should be end-of-line character
            linePos++;

        if(linePos >= inStr.size())
            break;

        if(indent && addOutput && lineNo > 1 && !firstLine && !lastLine) //could check !is.eof() rather than last line (trying to be clear)
        {
            indentAmount = baseIndentAmount;
            /*if(codeBlockDepth)
                outStr += "\n";
            else*/
            outStr += "\n" + baseIndentAmount;
        }
        firstLine = 0;

        for(; linePos < inStr.size() && inStr[linePos] != '\n';)
        {
            if(inStr[linePos] == '\\') //checks whether to escape
            {
                linePos++;
                if(inStr[linePos] == '@')
                {
                    outStr += "@";
                    linePos++;
                    if(indent)
                        indentAmount += " ";
                }
                else if(inStr[linePos] == '#')
                {
                    outStr += "#";
                    linePos++;
                    //if(indent)
                    indentAmount += " ";
                }
                else if(inStr[linePos] == '$')
                {
                    outStr += "$";
                    linePos++;
                    //if(indent)
                    indentAmount += " ";
                }
                else
                {
                    outStr += "\\";
                    if(indent)
                        indentAmount += " ";
                }

                continue;
            }
            else if(inStr[linePos] == '<') //checks about code blocks and html comments opening
            {
                bool finished = 0; //need this for <#-- comments at end of files

                //checks whether we're going down code block depth
                if(htmlCommentDepth == 0 && inStr.substr(linePos+1, 4) == "/pre")
                {
                    codeBlockDepth--;
                    if(codeBlockDepth == 0)
                        baseIndentAmount = beforePreBaseIndentAmount;
                    if(codeBlockDepth < baseCodeBlockDepth)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err(eos, readPath, lineNo) << "</pre> close tag has no preceding <pre*> open tag" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(inStr.substr(linePos, 4) == "<#--") //checks for raw multi-line comment
                {
                    linePos += 4;

                    size_t endPos = inStr.find("--#>", linePos);
                    if(endPos == std::string::npos)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err(eos, readPath, lineNo) << "open comment '<#--' has no close '--#>'" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    lineNo += std::count(inStr.begin() + linePos, inStr.begin() + endPos, '\n');

                    linePos = endPos + 4;

                    if(linePos < inStr.size() && inStr[linePos] == '!')
                        ++linePos;
                    else
                    {
                        //skips to next non-whitespace
                        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                        {
                            if(inStr[linePos] == '@')
                                linePos = inStr.find("\n", linePos);
                            else
                            {
                                if(inStr[linePos] == '\n')
                                    ++lineNo;
                                ++linePos;
                            }

                            if(linePos >= inStr.size())
                            {
                                finished = 1;
                                break;
                            }
                        }
                    }

                    continue;

                    //--linePos; // linePos is incemented again further below (a bit gross!)
                }//else checks whether to escape <
                else if(codeBlockDepth > 0 && inStr.substr(linePos+1, 4) != "code" && inStr.substr(linePos+1, 5) != "/code")
                {
                    outStr += "&lt;";
                    if(indent)
                        indentAmount += into_whitespace("&lt;");
                }
                else if(addOutput)
                {
                    outStr += '<';
                    if(indent)
                        indentAmount += " ";
                }

                if(!finished)
                {
                    //checks whether we're going up code block depth
                    if(htmlCommentDepth == 0 && inStr.substr(linePos+1, 3) == "pre")
                    {
                        if(codeBlockDepth == 0)
                        {
                            beforePreBaseIndentAmount = baseIndentAmount;
                            baseIndentAmount = "";
                        }
                        if(codeBlockDepth == baseCodeBlockDepth)
                            openCodeLineNo = lineNo;
                        codeBlockDepth++;
                    }

                    //checks whether we're going up html comment depth
                    if(inStr.substr(linePos+1, 3) == "!--")
                        htmlCommentDepth++;
                }

                linePos++;
            }
            else if(inStr[linePos] == '-') //checks about html comments closing
            {
                //checks whether we're going down html depth
                if(inStr.substr(linePos, 3) == "-->")
                    htmlCommentDepth--;

                if(inStr.substr(linePos, 4) == "--#>")
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, lineNo) << "close comment '--#>' has no open '<#--'" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(addOutput)
                {
                    outStr += '-';
                    if(indent)
                        indentAmount += " ";
                }
                linePos++;
            }
            else if(inStr[linePos] == '@') //checks for function calls
            {
                linePos++;
                int ret_val = read_and_process_fn(indent, baseIndentAmount, 'n', addOutput, inStr, lineNo, linePos, readPath, antiDepsOfReadPath, outStr, eos);

                if(ret_val == NSM_CONT)
                    return 0;
                else if(ret_val)
                    return ret_val;
            }
            else if(inStr[linePos] == '$' && (inStr[linePos+1] == '{' || inStr[linePos+1] == '['))
            {
                int ret_val = read_and_process_fn(indent, baseIndentAmount, 'n', addOutput, inStr, lineNo, linePos, readPath, antiDepsOfReadPath, outStr, eos);

                if(ret_val == NSM_CONT)
                    return 0;
                else if(ret_val)
                    return ret_val;
            }
            else //regular character
            {
                if(inStr[linePos] == '\n')
                {
                    ++linePos;
                    if(linePos >= inStr.size())
                        break;
                    if(addOutput)
                    {
                        indentAmount = baseIndentAmount;
                        /*if(codeBlockDepth) //do we want this? //baseIndentAmount updated at start of code block
                            outStr += "\n";
                        else*/
                        outStr += "\n" + baseIndentAmount;
                    }
                }
                else
                {
                    if(addOutput)
                    {
                        if(indent)
                        {
                            if(inStr[linePos] == '\t')
                                indentAmount += '\t';
                            else
                                indentAmount += ' ';
                        }

                        outStr += inStr[linePos];
                    }
                    ++linePos;
                }
            }
        }
    }

    if(codeBlockDepth > baseCodeBlockDepth)
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, openCodeLineNo) << "<pre*> open tag has no following </pre> close tag" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    return 0;
}

int Parser::f_read_and_process(const bool& addOutput,
                               const std::string& inStr,
                               int lineNo,
                               const Path& readPath,
                               std::set<Path> antiDepsOfReadPath,
                               std::string& outStr,
                               std::ostream& eos)
{
    //adds read path to anti dependencies of read path
    if(file_exists(readPath.str()))
        antiDepsOfReadPath.insert(readPath);

    return f_read_and_process_fast(addOutput, inStr, lineNo, readPath, antiDepsOfReadPath, outStr, eos);
}

int Parser::f_read_and_process_fast(const bool& addOutput,
                                  const std::string& inStr,
                                  int lineNo,
                                  const Path& readPath,
                                  std::set<Path>& antiDepsOfReadPath,
                                  std::string& outStr,
                                  std::ostream& eos)
{
    std::string baseIndentAmount = indentAmount;
    size_t linePos = 0;
    ++lineNo;
    while(linePos < inStr.size())
    {
        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || inStr[linePos] == '#'))
        {
            if(inStr[linePos] == '#')
                linePos = inStr.find('\n', linePos);
            else
            {
                if(inStr[linePos] == '\n')
                {
                    ++lineNo;
                    if(addOutput)
                    {
                        indentAmount = baseIndentAmount;
                        outStr += "\n" + baseIndentAmount;
                    }
                }
                else
                {
                    if(addOutput)
                    {
                        outStr += inStr[linePos];
                        indentAmount += " ";
                    }
                }
                ++linePos;
            }
        }

        if(linePos >= inStr.size())
            return 0;

        if(inStr[linePos] == '{' ||
           inStr[linePos] == '}' ||
           inStr[linePos] == '(' ||
           inStr[linePos] == ')' ||
           inStr[linePos] == '[' ||
           inStr[linePos] == ']' )//||
           //isdigit(inStr[linePos]))
        {
            if(addOutput)
            {
                outStr += inStr[linePos];
                indentAmount += " ";
            }
            ++linePos;

            continue;
        }
        else if(inStr[linePos] == '\\') //checks whether to escape
        {
            linePos++;
            if(inStr[linePos] == '@')
            {
                outStr += "@";
                linePos++;
                //if(indent)
                indentAmount += " ";
            }
            else if(inStr[linePos] == '#')
            {
                outStr += "#";
                linePos++;
                //if(indent)
                indentAmount += " ";
            }
            else if(inStr[linePos] == '$')
            {
                outStr += "$";
                linePos++;
                //if(indent)
                indentAmount += " ";
            }
            else
            {
                outStr += "\\";
                //if(indent)
                indentAmount += " ";
            }

            continue;
        }
        else if(inStr[linePos] == '@') //ignores @ for function calls
            ++linePos;
        else if(inStr[linePos] == '<')
        {
            if(inStr.substr(linePos, 4) == "<#--") //checks for raw multi-line comment
            {
                linePos += 4;

                size_t endPos = inStr.find("--#>", linePos);
                if(endPos == std::string::npos)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, lineNo) << "open comment '<#--' has no close '--#>'" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lineNo += std::count(inStr.begin() + linePos, inStr.begin() + endPos, '\n');

                linePos = endPos + 4;

                continue;
            }
        }
        else if(inStr[linePos] == '-') 
        {
            if(inStr.substr(linePos, 4) == "--#>")
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << "close comment '--#>' has no open '<#--'" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        /*else if(isdigit(inStr[linePos]))
        {
            do
            {
                if(addOutput)
                {
                    outStr += inStr[linePos];
                    indentAmount += " ";
                }
                ++linePos;
            }while(isdigit(inStr[linePos]));

            continue;
        }*/

        //int ret_val = read_and_process_fn(0, "", 'f', 1, inStr, lineNo, linePos, readPath, antiDepsOfReadPath, outStr, eos);
        int ret_val = read_and_process_fn(0, "", 'f', addOutput, inStr, lineNo, linePos, readPath, antiDepsOfReadPath, outStr, eos);

        if(ret_val == NSM_CONT)
            return 0;
        else if(ret_val)
            return ret_val;
    }

    return 0;
}

int Parser::read_and_process_fn(const bool& indent,
                                const std::string& baseIndentAmount,
                                const char& lang,
                                const bool& addOutput,
                                const std::string& inStr,
                                int& lineNo,
                                size_t& linePos,
                                const Path& readPath,
                                std::set<Path>& antiDepsOfReadPath,
                                std::string& outStr,
                                std::ostream& eos)
{
    int sLinePos = linePos, sLineNo = lineNo, conditionLineNo;

    if(linePos < inStr.size())
    {
        if(inStr[linePos] == '`') //expression
        {
            int sLineNo = lineNo;
            std::string expr_str;
            bool parseExpr = 0, doubleQuoted = 0;
            ++linePos;

            if(inStr[linePos] == '`')
                doubleQuoted = 1;

            while(inStr[linePos] != '`')
            {
                if(inStr[linePos] == '@' || inStr[linePos] == '$' || inStr[linePos] == '(')
                    parseExpr = 1;
                else if(inStr[linePos] == '\n')
                    ++lineNo;
                expr_str += inStr[linePos];
                ++linePos;
                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "`expr`" << c_white << ": no close ` for expression" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            ++linePos;

            if(doubleQuoted)
            {
                parseExpr = 0;
                if(inStr[linePos] == '`')
                    ++linePos;
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "``expr``" << c_white << ": no close `` for expression" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(parseExpr && parse_replace(lang, expr_str, "expression", readPath, antiDepsOfReadPath, sLineNo, "`" + expr_str + "`:", sLineNo, eos))
               return 1;

            std::string value;
            if(parseExpr)
            {
                if(!expr.set_expr(expr_str))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "``" << c_white << ": exprtk: failed to compile expression" << std::endl;
                    print_exprtk_parser_errs(eos, expr.parser, expr_str, readPath, sLineNo);
                    os_mtx->unlock();
                    return 1;
                }
                value = vars.double_to_string(expr.evaluate(), 0);
            }
            else
            {
                if(!exprset.add_expr(expr_str))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "``" << c_white << ": exprtk: failed to compile expression " << std::endl;
                    print_exprtk_parser_errs(eos, exprset.parser, expr_str, readPath, sLineNo);
                    os_mtx->unlock();
                    return 1;
                }
                value = vars.double_to_string(exprset.evaluate(expr_str), 0);
            }

            outStr += value;
            if(indent)
                indentAmount += into_whitespace(value);

            return 0;
        }
        else if(inStr[linePos] == '#')
        {
            if(inStr.substr(linePos, 3) == "#--")
            {
                linePos += 3;
                int openLine = lineNo;

                std::string commentStr, commentOutput;

                size_t endPos = inStr.find("--#", linePos);

                if(endPos != std::string::npos)
                    commentStr = inStr.substr(linePos, endPos-linePos);
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, openLine) << "open comment #-- has no close --#" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lineNo += count(commentStr.begin(), commentStr.end(), '\n');

                linePos = endPos + 3;

                if(lang == 'n')
                {
                    if(linePos < inStr.size() && inStr[linePos] == '!')
                        ++linePos;
                    else
                    {
                        //skips to next non-whitespace
                        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                        {
                            if(inStr[linePos] == '@')
                                linePos = inStr.find("\n", linePos);
                            else
                            {
                                if(inStr[linePos] == '\n')
                                    ++lineNo;
                                ++linePos;
                            }

                            if(linePos >= inStr.size())
                                break;
                        }
                    }
                }

                //parses comment stringstream
                if(lang == 'n')
                {
                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    if(n_read_and_process_fast(0, 0, commentStr, openLine-1, readPath, antiDepsOfReadPath, commentOutput, eos))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err(eos, readPath, openLine) << "failed to parse @#-- comment #--" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;
                }
                else if(lang == 'f')
                {
                    if(f_read_and_process_fast(0, commentStr, openLine-1, readPath, antiDepsOfReadPath, commentOutput, eos))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err(eos, readPath, openLine) << "failed to parse #-- comment #--" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                return 0;
            }
            else //if(inStr.substr(linePos, 1) == "#")
            {
                linePos = inStr.find("\n", linePos);

                return 0;
            }
        }
        else if((lang == 'n' || !addOutput) && inStr[linePos] == '\\')
        {
            if(inStr.substr(linePos, 2) == "\\\\")
            {
                linePos += 2;
                outStr += "\\";
                if(indent)
                    indentAmount += " ";

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "\\t")
            {
                linePos += 2;
                outStr += "\t";
                if(indent)
                    indentAmount += "\t";

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "\\n")
            {
                linePos += 2;
                indentAmount = baseIndentAmount;
                /*if(codeBlockDepth) //do we need this? //baseIndentAmount updated at start of code block
                    outStr += "\n";
                else*/
                outStr += "\n" + baseIndentAmount;

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "\\<")
            {
                linePos += 2;
                outStr += "&lt;";
                if(indent)
                    indentAmount += into_whitespace("&lt;");

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "\\@")
            {
                linePos += 2;
                outStr += "&commat;";
                if(indent)
                    indentAmount += into_whitespace("&commat;");

                return 0;
            }
            else
            {
                if(linePos && inStr.substr(linePos-1, 2) == "@\\")
                {
                    outStr += "@\\";
                    if(indent)
                        indentAmount += "  ";
                }
                else
                {
                    outStr += "\\";
                        if(indent)
                            indentAmount += " ";
                }

                ++linePos;

                return 0;
            }
        }
        else if(inStr[linePos] == '/')
        {
            if(inStr.substr(linePos, 2) == "/*")
            {
                linePos += 2;

                size_t endPos = inStr.find("*/", linePos);
                if(endPos == std::string::npos)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, lineNo) << "open comment '/*' has no close '*/'" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lineNo += std::count(inStr.begin() + linePos, inStr.begin() + endPos, '\n');

                linePos = endPos + 2;

                if(lang == 'n')
                {
                    if(linePos < inStr.size() && inStr[linePos] == '!')
                        ++linePos;
                    else
                    {
                        //skips to next non-whitespace
                        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                        {
                            if(inStr[linePos] == '@')
                                linePos = inStr.find("\n", linePos);
                            else
                            {
                                if(inStr[linePos] == '\n')
                                    ++lineNo;
                                ++linePos;
                            }

                            if(linePos >= inStr.size())
                                break;
                        }
                    }
                }

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "//")
            {
                linePos = inStr.find("\n", linePos);

                return 0;
            }
        }
        else if(inStr[linePos] == '!')
        {
            if(inStr.substr(linePos, 3) == "!\\n")
            {
                linePos = inStr.find("\n", linePos) + 1;

                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                    ++linePos;

                return 0;
            }
        }
        else if(inStr[linePos] == '-')
        {
            if(linePos && inStr.substr(linePos-1, 4) == "@---" && inStr[linePos + 3] != '{' && inStr[linePos + 3] != '(')
            { //can delete this later
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << "@--- text --- comments have changed to @#-- text --#" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inStr.substr(linePos, 3) == "-->")
            {
                linePos += 2;

                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }

                return 0;
            }
            else if(inStr.substr(linePos, 4) == "--//")
            {
                linePos += 4;

                std::string restOfLine;
                get_line(inStr, restOfLine, linePos);

                std::string commentOutput;


                if(lang == 'n')
                {
                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    if(n_read_and_process_fast(0, 0, restOfLine, lineNo-1, readPath, antiDepsOfReadPath, commentOutput, eos))
                    {
                        if(!consoleLocked) //console should have already been unlocked anyway when error was thrown in read_and_process
                            os_mtx->lock();
                        start_err(eos, readPath, lineNo) << "failed to parse '--//' comment" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;
                }
                else if(lang == 'f')
                {
                    if(f_read_and_process_fast(0, restOfLine, lineNo-1, readPath, antiDepsOfReadPath, commentOutput, eos))
                    {
                        if(!consoleLocked) //console should have already been unlocked anyway when error was thrown in read_and_process
                            os_mtx->lock();
                        start_err(eos, readPath, lineNo) << "failed to parse --// comment" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                return 0;
            }
            else if(inStr.substr(linePos, 2) == "--#")
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lang == 'f')
                    start_err(eos, readPath, lineNo) << "close comment --# has no open #--" << std::endl;
                else
                    start_err(eos, readPath, lineNo) << "close comment --# has no open @#--" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(!addOutput && inStr[linePos] == '<')
        {
            //removes trailing whitespace (multiline) from built file
            if(inStr.substr(linePos, 3) == "<--")
            {
                linePos += 3;

                strip_trailing_whitespace_multiline(outStr);

                int pos = outStr.size()-1;
                for(; pos>=0;--pos)
                    if(outStr[pos] == '\n')
                        break;

                indentAmount = into_whitespace(outStr.substr(pos+1, outStr.size()-pos));

                return 0;
            }
        }
        else if(!addOutput && inStr[linePos] == '*')
        {
            if(inStr.substr(linePos, 2) == "*/")
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lang == 'f')
                    start_err(eos, readPath, lineNo) << "close comment */ has no open /*" << std::endl;
                else
                    start_err(eos, readPath, lineNo) << "close comment */ has no open @/*" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(inStr[linePos] == 'p')
        {
            if(inStr.substr(linePos, 9) == "pagetitle") //can delete this later
            {
                outStr += toBuild.title.str;
                if(indent)
                    indentAmount += into_whitespace(toBuild.title.str);
                linePos += std::string("pagetitle").length();

                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "please change @pagetitle to $[title], @pagetitle will stop working in a few versions" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();

                return 0;
            }
        }
        else if(inStr[linePos] == 'i')
        {
            if(inStr.substr(linePos, 12) == "inputcontent")
            {
                contentAdded = 1;
                linePos += std::string("inputcontent").length();
                std::string toParse;
                if(lang == 'n')
                    toParse = "@input(" + quote(toBuild.contentPath.str()) + ")";
                else
                    toParse = "input(" + quote(toBuild.contentPath.str()) + ")";
                if(n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, lineNo) << "failed to insert content" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "please change @inputcontent to @content, @inputcontent will stop working in a few versions" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();

                return 0;
            }
        }
    }

    bool zeroParams = 0;
    bool oneParamOpt = 0;
    std::string funcName;
    std::string optionsStr, paramsStr;
    std::vector<std::string> options, params;
    bool doNotParse = 0, 
         parseFnName = 0, 
         parseOptions = 0, 
         parseParams = 0, 
         replaceVars = 1;

    //reads function name
    parseFnName = 0;
    if(read_func_name(funcName, parseFnName, linePos, inStr, readPath, lineNo, eos))
        return 1;

    //reads options
    if(linePos < inStr.size() && inStr[linePos] == '{')
    {
        if(read_optionsStr(optionsStr, parseOptions, linePos, inStr, readPath, lineNo, funcName, eos))
            return 1;

        if(parseOptions)
            if(parse_replace(lang, optionsStr, "options string", readPath, antiDepsOfReadPath, lineNo, funcName, sLineNo, eos))
                return 1;

        size_t pos=0;
        if(read_options(options, pos, optionsStr, readPath, lineNo, funcName, eos))
            return 1;
    }

    if(options.size())
    {
        for(size_t o=0; o<options.size(); o++)
        {
            if(options[o] == "1p")
                oneParamOpt = 1;
            else if(options[o] == "!p")
                doNotParse = 1;
            else if(options[o] == "!v")
                replaceVars = 0;
        }
    }

    if(parseFnName && !doNotParse)
    {
        if(parse_replace(lang, funcName, "function name", readPath, antiDepsOfReadPath, lineNo, funcName, sLineNo, eos))
            return 1;
    }

    bool hasIncrement = 0;
    if(linePos >= inStr.size() || (inStr[linePos] != '(' && inStr[linePos] != '['))
    {
        size_t funcNameSize = funcName.size();
        if(funcNameSize > 2)
        {
            std::string str = funcName.substr(0, 2);
            if(str == "++" || str == "--")
            {
                params.push_back(funcName.substr(2, funcNameSize-2));
                VPos vpos;
                if(vars.find(params[0], vpos))
                {
                    hasIncrement = 1;
                    funcName = str;
                }
                else
                    params.clear();
            }
            else
            {
                str = funcName.substr(funcNameSize-2, 2);
                if(str == "++" || str == "--")
                {
                    params.push_back(funcName.substr(0, funcNameSize-2));
                    VPos vpos;
                    if(vars.find(params[0], vpos))
                    {
                        hasIncrement = 1;
                        funcName = str + str[0];
                    }
                    else
                        params.clear();
                }
            }
        }
        
        if(!hasIncrement && lang == 'f' && addOutput)
        {
            if(inStr[sLinePos] == '"')
                funcName = "\"" + funcName + "\"";
            else if(inStr[sLinePos] == '\'')
                funcName = "'" + funcName + "'";
            outStr += funcName + optionsStr;
            if(indent)
                indentAmount += into_whitespace(funcName + optionsStr);
            return 0;
        }

        zeroParams = 1;
    }

    std::string brackets = "()";

    //reads parameters
    if(funcName == "if" || funcName == "for" || funcName == "while" || funcName == "do-while")
    {
        conditionLineNo = lineNo;
        if(read_params(params, ';', linePos, inStr, readPath, lineNo, funcName, eos))
            return 1;
    }
    else if(funcName == "||" || funcName == "&&")
    {

    }
    else if(funcName == ":=")
    {
        if(read_paramsStr(paramsStr, parseParams, linePos, inStr, readPath, lineNo, funcName, eos))
            return 1;

        if(parseParams && !doNotParse)
            if(parse_replace(lang, paramsStr, "params string", readPath, antiDepsOfReadPath, lineNo, funcName, sLineNo, eos))
                return 1;
    }
    else if(linePos < inStr.size() && (inStr[linePos] == '(' || inStr[linePos] == '['))
    {
        if(linePos < inStr.size() && inStr[linePos] == '[')
            brackets = "[]";
        if(doNotParse)
        {
            if(read_params(params, linePos, inStr, readPath, lineNo, funcName + brackets, eos))
                return 1;
        }
        else
        {
            if(read_paramsStr(paramsStr, parseParams, linePos, inStr, readPath, lineNo, funcName + brackets, eos))
                return 1;

            if(parseParams)
                if(parse_replace(lang, paramsStr, "params string", readPath, antiDepsOfReadPath, lineNo, funcName + brackets, sLineNo, eos))
                    return 1;

            size_t pos=0;
            if(oneParamOpt)
                params.push_back(unquote(paramsStr.substr(1, paramsStr.size()-2)));
            else if(read_params(params, pos, paramsStr, readPath, lineNo, funcName + brackets, eos))
                return 1;
        }
    }

    if(funcName[0] == '$')
    {
        if(funcName == "$")
        {
            if(params.size() != 1)
            {
                outStr += "$";
                linePos = sLinePos+1;
                if(indent)
                    indentAmount += " ";
                return 0;
            }

            std::string newline = "\n";
            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "\\n")
                        newline = "\\n";
                }
            }

            VPos vpos;
            std::string varName = params[0];
            if(vars.typeDefs.count(varName))
            {
                std::istringstream iss(vars.typeDefs[varName]);
                std::string ssLine, oldLine;
                int ssLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, ssLine);
                    if(0 < ssLineNo++)
                        outStr += newline + indentAmount;
                    oldLine = ssLine;
                    outStr += ssLine;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            else if(vars.find(varName, vpos)) //should this go after hard-coded variables?
            {
                if(!vars.add_str_from_var(vpos, outStr, 1, indent, indentAmount))
                {
                    std::string type = vpos.type.substr(0, vpos.type.find_first_of('<')),
                                toParse = "@'" + type + ".$'" + optionsStr + paramsStr;

                    int result;

                    if(lang == 'f')
                        result = f_read_and_process_fast(addOutput, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos);
                    else
                        result = n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos);

                    if(result)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << c_light_blue << "$" << brackets << c_white << ": cannot get string of var type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
            }
            else if(varName == "paginate.no_items_per_page")
            {
                std::string val = std::to_string(pagesInfo.noItemsPerPage);
                outStr += val;
                if(indent)
                    indentAmount += into_whitespace(val);
            }
            else if(varName == "paginate.no_pages")
            {
                std::string val = std::to_string((int) std::ceil((double)pagesInfo.items.size()/(double)pagesInfo.noItemsPerPage));
                int x = pagesInfo.pages.size();
                if(x)
                    val = std::to_string(x);
                else
                    val = std::to_string((int) std::ceil((double)pagesInfo.items.size()/(double)pagesInfo.noItemsPerPage));

                outStr += val;
                if(indent)
                    indentAmount += into_whitespace(val);
            }
            else if(varName == "paginate.page")
            {
                if(pagesInfo.cPageNo < 1 || pagesInfo.cPageNo-1 >= pagesInfo.pages.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "$[" << varName << "]: no pagination page " << pagesInfo.cPageNo << std::endl;
                    os_mtx->unlock();
                    return 1;   
                }

                std::istringstream iss(pagesInfo.pages[pagesInfo.cPageNo-1]);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + baseIndentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            else if(varName == "paginate.page_no")
            {
                std::string val = std::to_string(pagesInfo.cPageNo);
                outStr += val;
                if(indent)
                    indentAmount += into_whitespace(val);
            }
            else if(varName == "paginate.separator")
            {
                std::istringstream iss(pagesInfo.separator);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + baseIndentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            else if(varName == "line-no")
            {
                std::string val = std::to_string(lineNo);
                outStr += val;
                if(indent)
                    indentAmount += into_whitespace(val);
            }
            else if(varName == "title")
            {
                outStr += toBuild.title.str;
                if(indent)
                    indentAmount += into_whitespace(toBuild.title.str);
            }
            else if(varName == "name")
            {
                outStr += toBuild.name;
                if(indent)
                    indentAmount += into_whitespace(toBuild.name);
            }
            else if(varName == "content-path")
            {
                outStr += toBuild.contentPath.str();
                if(indent)
                    indentAmount += into_whitespace(toBuild.contentPath.str());
            }
            else if(varName == "output-path")
            {
                outStr += toBuild.outputPath.str();
                if(indent)
                    indentAmount += into_whitespace(toBuild.outputPath.str());
            }
            else if(varName == "content-ext")
            {
                //checks for non-default content extension
                Path extPath = toBuild.outputPath.getInfoPath();
                extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

                if(file_exists(extPath.str()))
                {
                    std::string ext;

                    std::ifstream ifs(extPath.str());
                    getline(ifs, ext);
                    ifs.close();

                    outStr += ext;
                    if(indent)
                        indentAmount += into_whitespace(ext);
                }
                else
                {
                    outStr += contentExt;
                    if(indent)
                        indentAmount += into_whitespace(contentExt);
                }
            }
            else if(varName == "output-ext")
            {
                //checks for non-default output extension
                Path extPath = toBuild.outputPath.getInfoPath();
                extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

                if(file_exists(extPath.str()))
                {
                    std::string ext;

                    std::ifstream ifs(extPath.str());
                    getline(ifs, ext);
                    ifs.close();

                    outStr += ext;
                    if(indent)
                        indentAmount += into_whitespace(ext);
                }
                else
                {
                    outStr += outputExt;
                    if(indent)
                        indentAmount += into_whitespace(outputExt);
                }
            }
            else if(varName == "script-ext")
            {
                //checks for non-default script extension
                Path extPath = toBuild.outputPath.getInfoPath();
                extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

                if(file_exists(extPath.str()))
                {
                    std::string ext;

                    std::ifstream ifs(extPath.str());
                    getline(ifs, ext);
                    ifs.close();

                    outStr += ext;
                    if(indent)
                        indentAmount += into_whitespace(ext);
                }
                else
                {
                    outStr += scriptExt;
                    if(indent)
                        indentAmount += into_whitespace(scriptExt);
                }
            }
            else if(varName == "template-path")
            {
                outStr += toBuild.templatePath.str();
                if(indent)
                    indentAmount += into_whitespace(toBuild.templatePath.str());
            }
            else if(varName == "content-dir")
            {
                if(contentDir.size() && (contentDir[contentDir.size()-1] == '/' || contentDir[contentDir.size()-1] == '\\'))
                {
                    outStr += contentDir.substr(0, contentDir.size()-1);
                    if(indent)
                        indentAmount += into_whitespace(contentDir.substr(0, contentDir.size()-1));
                }
                else
                {
                    outStr += contentDir;
                    if(indent)
                        indentAmount += into_whitespace(contentDir);
                }
            }
            else if(varName == "output-dir")
            {
                if(outputDir.size() && (outputDir[outputDir.size()-1] == '/' || outputDir[outputDir.size()-1] == '\\'))
                {
                    outStr += outputDir.substr(0, outputDir.size()-1);
                    if(indent)
                        indentAmount += into_whitespace(outputDir.substr(0, outputDir.size()-1));
                }
                else
                {
                    outStr += outputDir;
                    if(indent)
                        indentAmount += into_whitespace(outputDir);
                }
            }
            else if(varName == "default-content-ext")
            {
                outStr += contentExt;
                if(indent)
                    indentAmount += into_whitespace(contentExt);
            }
            else if(varName == "default-output-ext")
            {
                outStr += outputExt;
                if(indent)
                    indentAmount += into_whitespace(outputExt);
            }
            else if(varName == "default-script-ext")
            {
                outStr += scriptExt;
                if(indent)
                    indentAmount += into_whitespace(scriptExt);
            }
            else if(varName == "default-template")
            {
                outStr += defaultTemplate.str();
                if(indent)
                    indentAmount += into_whitespace(defaultTemplate.str());
            }
            else if(varName == "build-timezone")
            {
                outStr += dateTimeInfo.cTimezone;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cTimezone);
            }
            else if(varName == "load-timezone")
            {
                outStr += "<script>document.write(new Date().toString().split(\"(\")[1].split(\")\")[0])</script>";
                if(indent)
                    indentAmount += std::string(82, ' ');
            }
            else if(varName == "timezone" || varName == "current-timezone")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.cTimezone;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cTimezone);
            }
            else if(varName == "build-time")
            {
                outStr += dateTimeInfo.cTime;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cTime);
            }
            else if(varName == "build-UTC-time")
            {
                outStr += dateTimeInfo.currentUTCTime();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
            }
            else if(varName == "build-date")
            {
                outStr += dateTimeInfo.cDate;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cDate);
            }
            else if(varName == "build-UTC-date")
            {
                outStr += dateTimeInfo.currentUTCDate();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
            }
            else if(varName == "current-time")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.cTime;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cTime);
            }
            else if(varName == "current-UTC-time")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.currentUTCTime();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
            }
            else if(varName == "current-date")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.cDate;
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.cDate);
            }
            else if(varName == "current-UTC-date")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.currentUTCDate();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
            }
            else if(varName == "load-time")
            {
                outStr += "<script>document.write((new Date().toLocaleString()).split(\",\")[1])</script>";
                if(indent)
                    indentAmount += std::string(76, ' ');
            }
            else if(varName == "load-UTC-time")
            {
                outStr += "<script>document.write((new Date().toISOString()).split(\"T\")[1].split(\".\")[0])</script>";
                if(indent)
                    indentAmount += std::string(87, ' ');
            }
            else if(varName == "load-date")
            {
                outStr += "<script>document.write((new Date().toLocaleString()).split(\",\")[0])</script>";
                if(indent)
                    indentAmount += std::string(76, ' ');
            }
            else if(varName == "load-UTC-date")
            {
                outStr += "<script>document.write((new Date().toISOString()).split(\"T\")[0])</script>";
                if(indent)
                    indentAmount += std::string(73, ' ');
            }
            else if(varName == "build-YYYY")
            {
                outStr += dateTimeInfo.currentYYYY();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
            }
            else if(varName == "build-YY")
            {
                outStr += dateTimeInfo.currentYY();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentYY());
            }
            else if(varName == "current-YYYY")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.currentYYYY();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
            }
            else if(varName == "current-YY")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.currentYY();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentYY());
            }
            else if(varName == "load-YYYY")
            {
                outStr += "<script>document.write(new Date().getFullYear())</script>";
                if(indent)
                    indentAmount += std::string(57, ' ');
            }
            else if(varName == "load-YY")
            {
                outStr += "<script>document.write(new Date().getFullYear()%100)</script>";
                if(indent)
                    indentAmount += std::string(61, ' ');
            }
            else if(varName == "build-OS")
            {
                outStr += dateTimeInfo.currentOS();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentOS());
            }
            else if(varName == "current-OS")
            { //this is left for backwards compatibility
                outStr += dateTimeInfo.currentOS();
                if(indent)
                    indentAmount += into_whitespace(dateTimeInfo.currentOS());
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "$[" << varName << "]: no typedef, funcdef or variable named " << quote(varName) << std::endl;
                os_mtx->unlock();
                return 1;

                /*outStr += "@";
                linePos = sLinePos;
                if(indent)
                    indentAmount += " ";*/
            }

            return 0;
        }
    }
    else if(funcName[0] == 'i')
    {
        if(funcName == "input" || funcName == "inject")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string inputPathStr = params[0];
            bool ifExists = 0, inputRaw = 0, noIndent = 0;
            char inpLang = lang;

            Path inputPath;
            inputPath.set_file_path_from(inputPathStr);
            depFiles.insert(inputPath);

            if(inputPath == toBuild.contentPath)
                contentAdded = 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "f++")
                        inpLang = 'f';
                    else if(options[o] == "n++")
                        inpLang = 'n';
                    else if(options[o] == "raw")
                        inputRaw = 1;
                    else if(options[o] == "if-exists")
                        ifExists = 1;
                    else if(options[o] == "!i")
                        noIndent = 1;
                }
            }

            std::string oldIndent = indentAmount;
            if(noIndent)
                indentAmount = "";

            //ensures insert file exists
            if(file_exists(inputPathStr))
            {
                if(!inputRaw)
                {
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "inputting file " << inputPath << " would result in an input loop" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::string fileStr = string_from_file(inputPath.str());

                    //adds insert file
                    if(inpLang == 'f')
                    {
                        if(f_read_and_process(1, fileStr, 0, inputPath, antiDepsOfReadPath, outStr, eos) > 0)
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << "(" << inputPath << ") failed" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(n_read_and_process(1, fileStr, 0, inputPath, antiDepsOfReadPath, outStr, eos) > 0)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << "(" << inputPath << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    std::ifstream ifs(inputPath.str());
                    std::string fileLine, oldLine;
                    int fileLineNo = 0;

                    while(!ifs.eof())
                    {
                        getline(ifs, fileLine);
                        if(0 < fileLineNo++)
                            outStr += "\n" + indentAmount;
                        oldLine = fileLine;
                        outStr += fileLine;
                    }
                    indentAmount += into_whitespace(oldLine);

                    ifs.close();
                }
            }
            else if(ifExists)
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "inputting/injecting file " << inputPath << " failed as path does not exist" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(noIndent)
                indentAmount = oldIndent;

            return 0;
        }
        else if(funcName == "item")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "item: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string item;
            int itemLineNo;

            if(read_block(item, linePos, inStr, readPath, lineNo, itemLineNo, "item", eos))
                   return 1;

            pagesInfo.items.push_back(item);
            pagesInfo.itemLineNos.push_back(itemLineNo);
            pagesInfo.itemCallLineNos.push_back(sLineNo);
            pagesInfo.itemCallPaths.push_back(readPath);

            //skips to next non-whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || 
                                             inStr[linePos] == '\t' || 
                                             inStr[linePos] == '\n' || 
                                             (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find('\n', linePos);
                else
                {
                    if(inStr[linePos] == '\n')
                        ++lineNo;
                    ++linePos;
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "include")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "include: no files specified to include" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            char incLang = lang;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "f++")
                        incLang = 'f';
                    else if(options[o] == "n++")
                        incLang = 'n';
                }
            }

            Path inputPath;
            std::string includeOutput;
            std::string oldIndent = indentAmount;

            for(size_t p=0; p<params.size(); p++)
            {
                inputPath.set_file_path_from(params[p]);

                if(!includedFiles.count(inputPath))
                {
                    includedFiles.insert(inputPath);

                    if(!file_exists(params[p]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "include: including file " << inputPath << " failed as path does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "include: including file " << inputPath << " would result in an input loop" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::string fileStr = string_from_file(inputPath.str());

                    //parses include file
                    indentAmount = "";
                    if(incLang == 'f')
                    {
                        if(f_read_and_process(0, fileStr, 0, inputPath, antiDepsOfReadPath, includeOutput, eos) > 0)
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "include(" << inputPath << ") failed" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(n_read_and_process(0, fileStr, 0, inputPath, antiDepsOfReadPath, includeOutput, eos) > 0)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "include(" << inputPath << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
            }

            indentAmount = oldIndent;

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "if")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "if: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool addScope = 1, addOut = addOutput, addWhitespace = 0;
            char iLang = lang;
            std::string block, parsedCondition, whitespace;
            std::vector<std::string> blocks, conditions;
            std::vector<int> bLineNos, cLineNos;
            int bLineNo;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "f++")
                        iLang = 'f';
                    else if(options[o] == "n++")
                        iLang = 'n';
                    else if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                    if(options[o] == "s")
                        addScope = 1;
                    else if(options[o] == "!s")
                        addScope = 0;
                }
            }

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, "if(" + params[0] + ")", eos))
                return 1;

            conditions.push_back(params[0]);
            cLineNos.push_back(conditionLineNo);
            blocks.push_back(block);
            bLineNos.push_back(bLineNo);

            if(read_else_blocks(conditions, cLineNos, blocks, bLineNos, whitespace, linePos, inStr, readPath, lineNo, "if(" + params[0] + ")", eos))
                return 1;

            bool result;

            for(size_t b=0; b<blocks.size(); b++)
            {
                if(b < conditions.size())
                {
                    if(get_bool(result, conditions[b]))
                    {}
                    else
                    {
                        if(conditions[b] != "" && expr.set_expr(conditions[b]))
                            result = expr.evaluate();
                        else
                        {
                            parsedCondition = conditions[b];

                            if(parse_replace('f', parsedCondition, "if/else-if condition", readPath, antiDepsOfReadPath, conditionLineNo, "if(" + params[0] + ")", sLineNo, eos))
                                return 1;

                            if(replace_var(parsedCondition, readPath, conditionLineNo, "if/else-if()", eos))
                                return 1;

                            if(!get_bool(result, parsedCondition))
                            {
                                if(parsedCondition != "" && expr.set_expr(parsedCondition))
                                    result = expr.evaluate();
                                else
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err(eos, readPath, conditionLineNo) << "if/else-if: cannot convert " << quote(parsedCondition) << " to bool" << std::endl;
                                    start_err(eos, readPath, conditionLineNo) << "if/else-if: possible errors from ExprTk:" << std::endl;
                                    print_exprtk_parser_errs(eos, expr.parser, expr.expr_str, readPath, sLineNo);
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                        }
                    }
                }
                else
                    result = 1;

                if(result != 0)
                {
                    if(addOut)
                        addWhitespace = 1;

                    if(addScope)
                    {
                        std::string cScope = vars.layers[vars.layers.size()-1].scope;
                        vars.add_layer(cScope);
                        //vars.add_layer(vars.layers[vars.layers.size()-1].scope); //this is broken on windows!
                    }

                    int ret_val;
                    if(iLang == 'n')
                        ret_val = n_read_and_process_fast(1, addOut, blocks[b], bLineNos[b]-1, readPath, antiDepsOfReadPath, outStr, eos);
                    else
                        ret_val = f_read_and_process_fast(addOut, blocks[b], bLineNos[b]-1, readPath, antiDepsOfReadPath, outStr, eos);

                    if(addScope)
                        vars.layers.pop_back();

                    if(ret_val == NSM_BREAK)
                        return NSM_BREAK;
                    else if(ret_val)
                        return ret_val;

                    break;
                }
            }

            if(addWhitespace)
            {
                std::istringstream iss(whitespace);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + baseIndentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }

            return 0;
        }
        else if(funcName == "is_const")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "is_const: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                if(addOutput)
                {
                    if(vars.layers[vpos.layer].constants.count(params[0]))
                        outStr += "1";
                    else
                        outStr += "0";

                    if(indent)
                        indentAmount += " ";
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "is_const: no variables or functions named " << c_light_blue << params[0] << c_white << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "is_private")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "is_private: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                if(addOutput)
                {
                    if(vars.layers[vpos.layer].privates.count(params[0]))
                        outStr += "1";
                    else
                        outStr += "0";

                    if(indent)
                        indentAmount += " ";
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "is_private: no variables or functions named " << c_light_blue << params[0] << c_white << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "imginclude")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "imginclude: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string imgPathStr = params[0];
            Path imgPath;
            imgPath.set_file_path_from(imgPathStr);

            //warns user if img file doesn't exist
            if(!file_exists(imgPathStr.c_str()))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "imginclude: image file " << imgPath << " does not exist" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            Path pathToIMGFile(pathBetween(toBuild.outputPath.dir, imgPath.dir), imgPath.file);

            std::string imgInclude = "<img src=\"" + pathToIMGFile.str() + "\"";
            if(options.size())
                for(size_t o=0; o<options.size(); ++o)
                    imgInclude += " " + options[o];
            imgInclude += ">";

            outStr += imgInclude;
            if(indent)
                indentAmount += into_whitespace(imgInclude);

            return 0;
        }
        else if(funcName == "in")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                clear_console_line();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "in: expected 0 or 1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool fromFile = 0, readBlock = 0, parseBlock = 1;
            int bLineNo;
            std::string userInput, inputMsg;
            if(params.size() > 0)
                inputMsg = params[0];

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        readBlock = 1;
                    else if(options[o] == "!pb")
                        parseBlock = 0;
                    else if(options[o] == "from-file")
                        fromFile = 1;
                }
            }

            if(readBlock)
            {
                if(params.size() != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "in{block}: expected 0 parameters, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(read_block(inputMsg, linePos, inStr, readPath, lineNo, bLineNo, "in{block}", eos))
                   return 1;

                if(parseBlock && parse_replace(lang, inputMsg, "in block", readPath, antiDepsOfReadPath, bLineNo, "in{block}()", sLineNo, eos))
                    return 1;
            }

            if(!fromFile)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(inputMsg.size())
                    std::cout << inputMsg << std::endl;
                getline(std::cin, userInput);
                if(!consoleLocked)
                    os_mtx->unlock();

                if(n_read_and_process_fast(0, userInput, 0, Path("", "user input"), antiDepsOfReadPath, outStr, eos) > 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "failed to parse user input" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            else
            {
                std::string output_filename = ".@userfilein" + std::to_string(sys_counter++);
                int result;

                std::string contExtPath = ".nsm/" + outputDir + toBuild.name + ".ext";

                if(file_exists(contExtPath))
                {
                    std::string ext;
                    std::ifstream ifs(contExtPath);
                    getline(ifs, ext);
                    ifs.close();
                    contExtPath += ext;
                }
                else
                    output_filename += contentExt;

                std::ofstream ofs(output_filename);
                ofs << inputMsg;
                ofs.close();

                os_mtx->lock();
                #if defined _WIN32 || defined _WIN64
                    result = system((winTextEditor + " " + output_filename).c_str());
                #else  //*nix
                    result = system((unixTextEditor + " " + output_filename).c_str());
                #endif
                os_mtx->unlock();

                if(result)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "in{from-file}: text editor system call failed" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(!file_exists(output_filename))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "in{from-file}: user did not save file" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                Path inputPath;
                inputPath.set_file_path_from(output_filename);
                //ensures insert file isn't an anti dep of read path
                if(antiDepsOfReadPath.count(inputPath))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "in{from-file}: inputting file " << inputPath << " would result in an input loop" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                std::string fileStr = string_from_file(inputPath.str());

                //adds insert file
                if(n_read_and_process_fast(1, fileStr, 0, inputPath, antiDepsOfReadPath, outStr, eos) > 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "in{from-file}: failed to parse input user saved to file" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                //indent amount updated inside read_and_process

                remove_file(Path("", output_filename));
            }

            return 0;
        }
    }
    else if(funcName[0] == 'l')
    {
        if(funcName == "lua")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "lua: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int bLineNo;
            bool blockOpt = 0, parseBlock = 0, addOut = addOutput;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "pb")
                        parseBlock = 1;
                    else if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                }
            }

            if(params.size() == 0)
            {
                blockOpt = 1;
                params.push_back("");
                if(read_block(params[0], linePos, inStr, readPath, lineNo, bLineNo, "lua{block}", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, params[0], "lua block", readPath, antiDepsOfReadPath, bLineNo, "lua{block}", sLineNo, eos))
                    return 1;
            }

            //makes sure lua is initialised
            if(!lua.initialised)
                lua.init();

            int result = luaL_dostring(lua.L, params[0].c_str());

            if(result)
            {
                std::string errStr = lua_tostring(lua.L, -1);
                int errLineNo = sLineNo;
                if(blockOpt)
                    errLineNo = bLineNo;
                process_lua_error(errStr, errLineNo);

                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, errLineNo) << "lua: " << errStr << std::endl;
                os_mtx->unlock();
            }

            if(addOut)
            {
                std::string resultStr = std::to_string(result);
                outStr += resultStr;
                if(indent)
                    indentAmount += into_whitespace(resultStr);
            }

            return result;
        }
        else if(funcName.substr(0, 4) == "lua_")
        {
            std::string luaFnName = funcName.substr(4, funcName.size()-4);

            //makes sure lua is initialised
            if(!lua.initialised)
                lua.init();

            if(luaFnName == "addnsmfns")
            {
                if(params.size() != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 0 parameters, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_addnsmfns();

                return 0;
            }
            else if(luaFnName == "gettop")
            {
                if(params.size() != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 0 parameters, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                std::string val = std::to_string(lua_gettop(lua.L));

                outStr += val;
                if(indent)
                    indentAmount += into_whitespace(val);

                return 0;
            }
            else if(luaFnName == "getglobal")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_getglobal(lua.L, params[0].c_str());

                return 0;
            }
            else if(luaFnName == "setglobal")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_setglobal(lua.L, params[0].c_str());

                return 0;
            }
            else if(luaFnName == "pop")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(!isInt(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": number of stack items to remove from top should be an integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_pop(lua.L, std::atoi(params[0].c_str()));

                return 0;
            }
            else if(luaFnName == "remove")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(!isInt(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": stack index parameter should be an integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_remove(lua.L, std::atoi(params[0].c_str()));

                return 0;
            }
            else if(luaFnName == "tonumber")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(!isInt(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": stack index parameter should be an integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                int i = std::atoi(params[0].c_str());

                if(lua_type(lua.L, i) == LUA_TNUMBER)
                {
                    std::string val = std::to_string(lua_tonumber(lua.L, i));

                    outStr += val;
                    if(indent)
                        indentAmount += into_whitespace(val);
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "lua_tonumber: item number " << params[0] << " on Lua stack is not a number" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                return 0;
            }
            else if(luaFnName == "tostring")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(!isInt(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": stack index parameter should be an integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                int i = std::atoi(params[0].c_str());

                if(lua_type(lua.L, i) == LUA_TSTRING)
                {
                    std::string val = std::string(lua_tostring(lua.L, i));

                    outStr += val;
                    if(indent)
                        indentAmount += into_whitespace(val);
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "lua_tostring: item number " << params[0] << " on Lua stack is not a string" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                return 0;
            }
            else if(luaFnName == "pushlightuserdata")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                VPos vpos;
                if(vars.find(params[0], vpos))
                {
                    if(vpos.type == "bool" || vpos.type == "int" || vpos.type == "double" || vpos.type == "std::double")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].doubles[vpos.name]);
                    else if(vpos.type == "char" || vpos.type == "string" || vpos.type == "std::string")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].strings[vpos.name]);
                    else if(vpos.type == "std::bool")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].bools[vpos.name]);
                    else if(vpos.type == "std::int")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].ints[vpos.name]);
                    else if(vpos.type == "std::char")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].chars[vpos.name]);
                    else if(vpos.type == "std::llint")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].llints[vpos.name]);
                    else if(vpos.type == "std::vector<double>")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].doubVecs[vpos.name]);
                    else if(vpos.type == "fstream")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].fstreams[vpos.name]);
                    else if(vpos.type == "ifstream")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].ifstreams[vpos.name]);
                    else if(vpos.type == "ofstream")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].ofstreams[vpos.name]);
                    else if(vpos.type == "function")
                        lua_pushlightuserdata(lua.L, &vars.layers[vpos.layer].functions[vpos.name]);
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": do not recognise the type " << quote(vpos.type) << " for " << quote(params[0]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": no variables named " << quote(params[0]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                return 0;
            }
            else if(luaFnName == "pushnumber")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(!isDouble(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": parameter to push on to Lua stack should be a double, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_pushnumber(lua.L, std::strtod(params[0].c_str(), NULL));

                return 0;
            }
            else if(luaFnName == "pushstring")
            {
                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                lua_pushstring(lua.L, params[0].c_str());

                return 0;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": function does not exist in this scope" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(funcName == "link")
        {
            if(params.size() != 1 && params.size() != 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "link: expected 1-2 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size() == 1)
                params.push_back(params[0]);

            std::string str;
            int langOpt = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "html")
                        langOpt = 0;
                    else if(options[o] == "md")
                        langOpt = 1;
                }
            }
            

            if(langOpt == 0)
                str = "<a href=\"" + params[0] + "\">" + params[1] + "</a>";
            else if(langOpt == 1)
                str = "[" + params[1] + "](" + params[0] + ")";

            outStr += str;
            if(indent)
                indentAmount += into_whitespace(str);

            return 0;
        }
        else if(funcName == "layer")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "layer: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size() == 0)
            {
                outStr += std::to_string(vars.layers.size()-1);
                if(indent)
                    indentAmount += into_whitespace(std::to_string(vars.layers.size()-1));
            }
            else if(params.size() == 1)
            {
                VPos vpos;

                if(vars.find(params[0], vpos))
                {
                    outStr += std::to_string(vpos.layer);
                    if(indent)
                        indentAmount += into_whitespace(std::to_string(vpos.layer));
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "layer: no variables/functions named " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
            else if(funcName == "ls")
                funcName = "dir";
        #endif
        else if(funcName == "lst")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() == 0)
                params.push_back("./");
            else if(params.size() > 1) 
            {
                for(size_t p=0; p+1<params.size(); ++p)
                    options.push_back(params[p]);
                params = std::vector<std::string>(1, params[params.size()-1]);
            }

            params[0] = replace_home_dir(params[0]);

            std::string separator = " ";
            std::vector<std::string> shOptions;
            size_t pos = params[0].find_first_of('*');
            bool hasUnknownOptions = 0;
            bool toConsole = 0;
            bool incHidden = 0;
            bool fullPath = 0;

            if(lolcat)
                toConsole = 0;
            else if((mode == MODE_INTERP || mode == MODE_SHELL) && (&outStr == &parsedText))
                toConsole = 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "1")
                        separator = "\n";
                    else if(options[o] == ",")
                        separator = ", ";
                    else if(options[o] == "a")
                        incHidden = 1;
                    else if(options[o] == "!c")
                        toConsole = 0;
                    else if(options[o] == "c")
                        toConsole = 1;
                    else if(options[o] == "P")
                        fullPath = 1;
                    else if(options[o] == "!p" || options[o] == "!v" || options[o] == "1p")
                    {}
                    else
                    {
                        hasUnknownOptions = 1;
                        shOptions.push_back(options[o]);
                    }
                }
            }

            if(!hasUnknownOptions && params.size() == 1 && (pos == std::string::npos || pos == params[0].size()-1))
            {
                std::set<std::string> paths;
                Path lsPath;

                if(pos == params[0].size()-1)
                {
                    params[0] = params[0].substr(0, params[0].size()-1);
                    lsPath.set_file_path_from(params[0].c_str());
                    makeSearchable(lsPath);

                    if(fullPath)
                    {
                        std::string dir;
                        std::set<std::string> lsPaths = lsSetStar(lsPath, incHidden);
                        size_t pos;

                        if(params[0].size() && 
                           params[0][params[0].size()-1] != '/' && 
                           params[0][params[0].size()-1] != '\\' && 
                           dir_exists(params[0]))
                        {
                            pos = params[0].size();
                            params[0] += "/";
                        }
                        else
                            pos = params[0].find_last_of("/\\");

                        if(pos != std::string::npos)
                            dir = params[0].substr(0, pos+1);

                        for(auto path=lsPaths.begin(); path!=lsPaths.end(); ++path)
                            paths.insert(dir + *path);
                    }
                    else
                        paths = lsSetStar(lsPath, incHidden);

                    if(!paths.size())
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "lst: cannot access " << quote(params[0]) << ": no such file or directory" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(pos == std::string::npos)
                {
                    if(!path_exists(params[0]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "lst: cannot access " << quote(params[0]) << ": no such file or directory" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(params[0].find_last_of("/\\") != params[0].size()-1 && dir_exists(params[0]))
                        params[0] += "/";

                    lsPath.set_file_path_from(params[0].c_str());
                    makeSearchable(lsPath);
                    if(dir_exists(lsPath.str()))
                    {
                        if(fullPath)
                        {
                            std::string dir;
                            std::set<std::string> lsPaths = lsSetStar(lsPath, incHidden);
                            size_t pos;

                            if(params[0].size() && 
                               params[0][params[0].size()-1] != '/' && 
                               params[0][params[0].size()-1] != '\\' && 
                               dir_exists(params[0]))
                            {
                                pos = params[0].size();
                                params[0] += "/";
                            }
                            else
                                pos = params[0].find_last_of("/\\");

                            if(pos != std::string::npos)
                                dir = params[0].substr(0, pos+1);

                            for(auto path=lsPaths.begin(); path!=lsPaths.end(); ++path)
                                paths.insert(dir + *path);
                        }
                        else
                            paths = lsSetStar(lsPath, incHidden);
                    }
                    else if(file_exists(lsPath.str()))
                    {
                        lsPath.dir = lsPath.file = "";
                        paths.insert(params[0]);
                    }
                }
                

                if(toConsole)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    if(fullPath)
                        coutPaths("", paths, separator, 1, 0);
                    else
                        coutPaths(lsPath.dir, paths, separator, 1, 0);
                    std::cout << std::endl;
                    if(!consoleLocked)
                        os_mtx->unlock();

                    if(linePos < inStr.size() && inStr[linePos] == '!')
                        ++linePos;
                    else
                    {
                        //skips to next non-whitespace
                        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                        {
                            if(inStr[linePos] == '@')
                                linePos = inStr.find("\n", linePos);
                            else
                            {
                                if(inStr[linePos] == '\n')
                                    ++lineNo;
                                ++linePos;
                            }

                            if(linePos >= inStr.size())
                                break;
                        }
                    }
                }
                else if(paths.size())
                {
                    auto path=paths.begin();

                    if(separator == "\n")
                    {
                        std::string endPath;

                        if(path->find_first_of(' ') == std::string::npos)
                            endPath = *path;
                        else
                            endPath = quote(*path);
                        outStr += endPath;
                        ++path;
                        for(; path!=paths.end(); ++path)
                        {
                            if(path->find_first_of(' ') == std::string::npos)
                                endPath = *path;
                            else
                                endPath = quote(*path);
                            outStr += "\n" + indentAmount + endPath;
                        }

                        indentAmount += into_whitespace(endPath);
                    }
                    else
                    {
                        std::string lsStr;
                        if(path->find_first_of(' ') == std::string::npos)
                            lsStr = *path;
                        else
                            lsStr = quote(*path);
                        ++path;
                        for(; path!=paths.end(); ++path)
                        {
                            if(path->find_first_of(' ') == std::string::npos)
                                lsStr += separator + *path;
                            else
                                lsStr += separator + quote(*path);
                        }
                        outStr += lsStr;
                        indentAmount += into_whitespace(lsStr);
                    }
                }
            }
            else
            {
                #if defined _WIN32 || defined _WIN64
                    funcName = "dir /b"; 
                #else  //*nix
                    funcName = "ls -d";
                #endif
                if(toConsole)
                    return try_system_call_console(funcName, shOptions, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, outStr);
                else
                    return try_system_call_inject(funcName, shOptions, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, outStr);
            }

            return 0;
        }
        else if(funcName == "lolcat")
        {
            bool readBlock = 0, parseBlock = 1;
            int bLineNo;
            std::string txt;

            if(!lolcatInit && !lolcat_init())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, sLineNo) << "could not find 'lolcat' installed on the machine" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        readBlock = 1;
                    else if(options[o] == "!pb")
                        parseBlock = 0;
                }
            }

            if(readBlock)
            {
                if(params.size() != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "lolcat{block}: expected 0 parameters, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(read_block(txt, linePos, inStr, readPath, lineNo, bLineNo, "lolcat", eos))
                   return 1;

                if(parseBlock && parse_replace(lang, txt, "lolcat block", readPath, antiDepsOfReadPath, bLineNo, "lolcat", sLineNo, eos))
                    return 1;
            }

            VPos vpos;
            std::string gIndentAmount;
            for(size_t p=0; p<params.size(); p++)
            {
                if(params[p] == "endl")
                    txt += "\r\n";
                else if(replaceVars && vars.find(params[p], vpos))
                {
                    gIndentAmount = "";
                    if(!vars.add_str_from_var(vpos, txt, 1, indent, gIndentAmount))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "lolcat: cannot get string of var type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                    txt += params[p];
            }

            if(!consoleLocked)
                os_mtx->lock();
            if(lolcatInit)
                rnbwcout(txt);
            else
                eos << txt << std::endl;
            if(!consoleLocked)
                os_mtx->unlock();

            return 0;
        }
        else if(funcName == "lolcat.on")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "lolcat.on: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(lolcat)
            {
                if(mode == MODE_INTERP || mode == MODE_SHELL)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    rnbwcout("lolcat already activated\a");
                    if(!consoleLocked)
                        os_mtx->unlock();
                }

                return 0;
            }

            if(!lolcatInit)
            {
                if(lolcat_init())
                {
                    #if defined _WIN32 || defined _WIN64
                        if(using_powershell_colours())
                            lolcat_powershell();
                    #endif
                    lolcat = 1;
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_warn(eos, readPath, sLineNo) << "could not find 'lolcat' installed on the machine" << std::endl;
                    if(!consoleLocked)
                        os_mtx->unlock();
                }
            }
            else
            {
                #if defined _WIN32 || defined _WIN64
                    if(using_powershell_colours())
                        lolcat_powershell();
                #endif
                lolcat = 1;
            }

            if(lolcat && (mode == MODE_INTERP || mode == MODE_SHELL))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                rnbwcout("lolcat activated");
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "lolcat.off")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "lolcat.off: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(mode == MODE_INTERP || mode == MODE_SHELL)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lolcat)
                    std::cout << "lolcat deactivated" << std::endl;
                else
                    std::cout << "lolcat not activated\a" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            lolcat = 0;

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == 'p')
    {
        if(funcName == "pathto")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathto: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool fromName = 0, toFile = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "name")
                        fromName = 1;
                    else if(options [o] == "file")
                        toFile = 1;
                }

                if(fromName && toFile)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "pathto(" << params[0] << ") failed, cannot have both options file and name" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(fromName == 0 && toFile == 0)
                fromName = toFile = 1;

            if(fromName)
            {
                Name targetName = params[0];
                TrackedInfo targetInfo;
                targetInfo.name = targetName;

                if(trackedAll->count(targetInfo))
                {
                    toFile = 0;
                    Path targetPath = trackedAll->find(targetInfo)->outputPath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    outStr += pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(!toFile) //throws error if target targetName isn't being tracked by Nift
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "pathto(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(toFile)
            {
                std::string targetFilePath = params[0];

                if(path_exists(targetFilePath.c_str()))
                {
                    fromName = 0;
                    Path targetPath;
                    targetPath.set_file_path_from(targetFilePath);

                    Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    outStr += pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(!fromName) //throws error if targetFilePath doesn't exist
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "pathto: file " << targetFilePath << " does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "pathto(" << params[0] << "): " << quote(params[0]) << " is neither a tracked name nor a file that exists" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "pathtopage")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtopage: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            Name targetName = params[0];
            TrackedInfo targetInfo;
            targetInfo.name = targetName;

            if(trackedAll->count(targetInfo))
            {
                Path targetPath = trackedAll->find(targetInfo)->outputPath;
                //targetPath.set_file_path_from(targetPathStr);

                Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                //adds path to target
                outStr += pathToTarget.str();
                if(indent)
                    indentAmount += into_whitespace(pathToTarget.str());
            }
            else //throws error if target targetName isn't being tracked by Nift
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtopage(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "pathtopageno")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtopageno: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(!isPosInt(params[0]))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtopageno: page number should be a non-negative integer, got " << params[0] << std::endl;
                os_mtx->unlock();
                return 1;
            }

            size_t pageNo = std::atoi(params[0].c_str());

            if(pageNo > pagesInfo.pages.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtopageno: " << toBuild.name << " has no page number " << params[0];
                eos << ", currently has " << pagesInfo.pages.size() << " pages" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            Path targetPath;

            if(pageNo > 1)
                targetPath = Path(pagesInfo.pagesDir, params[0] + outputExt);
            else
                targetPath.set_file_path_from(pagesInfo.pagesDir.substr(0, pagesInfo.pagesDir.size()-1) + outputExt);

            //targetPath.set_file_path_from(targetPathStr);

            Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

            //adds path to target
            outStr += pathToTarget.str();
            if(indent)
                indentAmount += into_whitespace(pathToTarget.str());

            return 0;
        }
        else if(funcName == "pathtofile")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtofile: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string targetFilePath = params[0];

            if(path_exists(targetFilePath.c_str()))
            {
                Path targetPath;
                targetPath.set_file_path_from(targetFilePath);

                Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                //adds path to target
                outStr += pathToTarget.str();
                if(indent)
                    indentAmount += into_whitespace(pathToTarget.str());
            }
            else //throws error if targetFilePath doesn't exist
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pathtofile: file " << targetFilePath << " does not exist" << std::endl;
                os_mtx->unlock();
                std::cout << "WTF" << std::endl;
                return 1;
            }

            return 0;
        }
        else if(funcName == "private")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ',', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            std::string toParse = "@:=";
            toParse += "{private";
            for(size_t o=0; o<options.size(); ++o)
                toParse += ", " + options[o];
            toParse += '}';
            if(params.size())
            {
                int depth=0;
                for(size_t pos=0; pos<params[0].size(); ++pos)
                {
                    if(params[0][pos] == '<' || params[0][pos] == '[')
                        ++depth;
                    else if(params[0][pos] == '>' || params[0][pos] == ']')
                        --depth;
                    else if(params[0][pos] == '"')
                        pos = params[0].find_first_of('"', pos+1);
                    else if(params[0][pos] == '\'')
                        pos = params[0].find_first_of('\'', pos+1);
                    else if(depth == 0 && params[0][pos] == ' ')
                    {
                        params[0][pos] = ',';
                        break;
                    }
                }
                toParse += "(" + params[0];
                for(size_t p=1; p<params.size(); ++p)
                    toParse += ", " + params[p];
                toParse += ')';
            }

            if(n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) <<  "private: private definition failed" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "paginate")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            pagesInfo.callPath = readPath;
            pagesInfo.callLineNo = sLineNo;
            parsedText += "__paginate_here__";

            for(; linePos < inStr.size() && inStr[linePos] != '\n'; ++linePos)
            {
                if(inStr[linePos] != ' ' && inStr[linePos] != '\t')
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate: expected nothing after call on this line " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            //skips to next non-whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || 
                                             inStr[linePos] == '\t' || 
                                             inStr[linePos] == '\n' || 
                                             (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find('\n', linePos);
                else
                {
                    if(inStr[linePos] == '\n')
                        ++lineNo;
                    ++linePos;
                }
            }

            return 0;
        }
        else if(funcName == "paginate.no_items_per_page")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate.no_items_per_page: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(!isPosInt(params[0]))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate.no_items_per_page: expected a positive integer parameter, got " << params[0] << std::endl;
                os_mtx->unlock();
                return 1;
            }

            pagesInfo.noItemsPerPage = std::atoi(params[0].c_str());

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "paginate.separator")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate.separator: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size() == 1)
            {
                pagesInfo.separator = params[0];
                pagesInfo.separatorLineNo = sLineNo;
            }
            else
            {
                if(read_block(pagesInfo.separator, linePos, inStr, readPath, lineNo, pagesInfo.separatorLineNo, "paginate.separator", eos))
                       return 1;

                pagesInfo.separator = "\n" + pagesInfo.separator + "\n\n";
                --pagesInfo.separatorLineNo;

                if(parse_replace(lang, pagesInfo.separator, "paginate.separator", readPath, antiDepsOfReadPath, pagesInfo.separatorLineNo, "item", sLineNo, eos))
                    return 1;

                if(linePos < inStr.size() && inStr[linePos] == '!')
                    ++linePos;
                else
                {
                    //skips to next non-whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                    {
                        if(inStr[linePos] == '@')
                            linePos = inStr.find("\n", linePos);
                        else
                        {
                            if(inStr[linePos] == '\n')
                                ++lineNo;
                            ++linePos;
                        }

                        if(linePos >= inStr.size())
                            break;
                    }
                }
            }

            return 0;
        }
        else if(funcName == "paginate.template")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "paginate.template: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            pagesInfo.templateCallLineNo = sLineNo;
            if(read_block(pagesInfo.templateStr, linePos, inStr, readPath, lineNo, pagesInfo.templateLineNo, "paginate.template", eos))
                   return 1;

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "parse")
        {
            if(params.size() != 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "parse: expected 2 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size() == 2)
            {
                if(isPosInt(params[0]))
                {
                    int noParses = std::atoi(params[0].c_str());
                    for(int p=1; p<noParses; p++)
                        if(parse_replace(lang, params[1], "parse string", readPath, antiDepsOfReadPath, lineNo, "parse", sLineNo, eos))
                            return 1;

                    outStr += params[1];
                    if(indent)
                        indentAmount += into_whitespace(params[1]);
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "parse: first parameter should be a positive integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "precision")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "precision: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size())
            {
                if(params[0] == "fixed")
                    vars.fixedPrecision = 1;
                else if(params[0] == "!fixed")
                    vars.fixedPrecision = 0;
                else if(params[0] == "scientific")
                    vars.scientificPrecision = 1;
                else if(params[0] == "!scientific")
                    vars.scientificPrecision = 0;
                else if(params[0] == "hexfloat")
                    vars.fixedPrecision = vars.scientificPrecision = 1;
                else if(params[0] == "!hexfloat")
                    vars.fixedPrecision = vars.scientificPrecision = 0;
                else if(params[0] == "defaultfloat")
                    vars.fixedPrecision = vars.scientificPrecision = 0;
                else if(!isNonNegInt(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "precision: parameter should be 'fixed', '!fixed', 'scientific', '!scientific', 'hexfloat', '!hexfloat', 'defaultfloat' or a non-negative integer, got " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else
                    vars.precision = std::atoi(params[0].c_str());
            }
            else
            {
                outStr += std::to_string(vars.precision); //check this
                if(indent)
                    indentAmount += into_whitespace(std::to_string(vars.precision));
            }

            return 0;
        }
        else if(funcName == "pwd")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "pwd: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string pwd = get_pwd();
            outStr += pwd;
            if(indent)
                indentAmount += into_whitespace(pwd);

            return 0;
        }
        else if(funcName == "poke")
        {
            if(params.size() == 0)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "poke: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool createFiles = 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "dirs")
                        createFiles = 0;
                }
            }

            Path path;
            for(size_t p=0; p<params.size(); p++)
            {
                path.set_file_path_from(params[p]);
                if(createFiles)
                    path.ensureFileExists();
                else
                {
                    if(path.file.size())
                    {
                        if(path.dir.size())
                            path.dir += "/" + path.file + "/";
                        else
                            path.dir = "./" + path.file + "/";
                    }
                    path.ensureDirExists();
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == 'c')
    {
        if(funcName == "content")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo)  << "content: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            contentAdded = 1;
            std::string toParse = "@input";
            if(options.size())
            {
                toParse += "{" + options[0];
                for(size_t o=1; o<options.size(); o++)
                    toParse += ", " + options[o];
                toParse += '}';
            }
            toParse += "(" + quote(toBuild.contentPath.str()) + ")";

            if(n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) <<  "failed to insert content" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "continue")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "continue: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return NSM_CONT; //check this??
        }
        else if(funcName == "close")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "close: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vpos.type == "fstream")
                        vars.layers[vpos.layer].fstreams[params[p]].close();
                    else if(vpos.type == "ifstream")
                        vars.layers[vpos.layer].ifstreams[params[p]].close();
                    else if(vpos.type == "ofstream")
                        vars.layers[vpos.layer].ofstreams[params[p]].close();
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "close is not defined for variable " << quote(params[p]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "close: no variables named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "cpy")
        {
            if(params.size() < 2)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool backupFiles = 0,
                 forceFile = 0,
                 ifNew = 0,
                 overwrite = 1,
                 changePermissions = 0, 
                 prompt = 0,
                 verbose = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "b")
                        backupFiles = 1;
                    else if(options[o] == "f")
                        changePermissions = 1;
                    else if(options[o] == "i")
                        prompt = 1;
                    else if(options[o] == "n")
                        overwrite = 0;
                    else if(options[o] == "T")
                        forceFile = 1;
                    else if(options[o] == "u")
                        ifNew = 1;
                    else if(options[o] == "v")
                        verbose = 1;
                }
            }

            size_t t = params.size()-1;
            Path source, target;
            std::string sourceStr, 
                        targetStr = replace_home_dir(params[t]),
                        targetParam = params[t];
            char promptInput = 'n';

            params.pop_back();

            if(params.size() == 1 && 
                (forceFile || !dir_exists(targetStr)) && 
                params[0].find_first_of('*') == std::string::npos)
            {
                if(params[0].size() && (params[0][params[0].size()-1] == '/' || params[0][params[0].size()-1] == '\\'))
                {
                    int pos=params[0].size()-2;
                    while(pos>=0 && (params[0][pos] == '/' || params[0][pos] == '\\'))
                        --pos;
                    params[0] = params[0].substr(0, pos+1);
                }

                sourceStr = replace_home_dir(params[0]);
                source.set_file_path_from(sourceStr);
                target.set_file_path_from(targetStr);

                if(!path_exists(sourceStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot copy ";
                    eos << quote(params[0]) << " as path does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(overwrite || !file_exists(targetStr))
                {
                    if(!ifNew || source.modified_after(target))
                    {
                        if(changePermissions)
                        {
                            if(file_exists(targetStr))
                                chmod(targetStr.c_str(), 0666);
                            else if(dir_exists(targetStr))
                                chmod(targetStr.c_str(), S_IRWXU);
                        }

                        if((prompt && promptInput != 'a') || verbose)
                        {
                            eos << quote(params[0]) << " -> " << quote(params[1]) << std::endl;

                            if(prompt && promptInput != 'a')
                            {
                                eos << "? " << std::flush;
                                promptInput = nsm_getch();
                                if(promptInput == 'Y' || promptInput == '1')
                                    promptInput = 'y';
                                else if(promptInput == 'A')
                                    promptInput = 'a';
                            }
                            
                            eos << std::endl;
                        }

                        if(!prompt || promptInput == 'y' || promptInput == 'a')
                        {
                            if(backupFiles && path_exists(targetStr))
                                rename(targetStr.c_str(), (targetStr + "~").c_str());

                            if(dir_exists(sourceStr))
                            {
                                if(cpDir(sourceStr, targetStr, sLineNo, readPath, eos, consoleLocked, os_mtx))
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to copy directory ";
                                    eos << quote(params[0]) << " to " << quote(targetParam) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                            else if(cpFile(sourceStr, targetStr, sLineNo, readPath, eos, consoleLocked, os_mtx))
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to copy file ";
                                eos << quote(params[0]) << " to " << quote(targetParam) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                    }
                }
            }
            else
            {
                if(!dir_exists(targetStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot copy files to ";
                    eos << quote(targetStr) << " as directory does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                char endChar = targetStr[targetStr.size()-1];
                if(endChar != '/' && endChar != '\\')
                    targetStr += "/";
                target.set_file_path_from(targetStr);

                struct stat  oldDirPerms;
                if(changePermissions)
                {
                    stat(targetStr.c_str(), &oldDirPerms);
                    chmod(target.dir.c_str(), S_IRWXU);
                }
                else if(!can_write(targetStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot write to " << quote(target.dir) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                std::vector<Path> sources;
                for(size_t p=0; p<params.size(); ++p)
                {
                    if(params[p].size() && (params[p][params[p].size()-1] == '/' || params[p][params[p].size()-1] == '\\'))
                    {
                        int pos=params[p].size()-2; 
                        while(pos>=0 && (params[p][pos] == '/' || params[p][pos] == '\\'))
                            --pos;
                        params[p] = params[p].substr(0, pos+1);
                    }

                    sourceStr = replace_home_dir(params[p]);
                    source.set_file_path_from(sourceStr);
                    sources.push_back(source);

                    if(!path_exists(sourceStr))
                    {
                        if(sourceStr.find_first_of('*') != std::string::npos)
                        {
                            std::string parsedTxt, toParse = "@lst{!c, 1, P}(" + sourceStr + ")";
                            int parseLineNo = 0;

                            if(n_read_and_process_fast(indent, toParse, parseLineNo, readPath, antiDepsOfReadPath, parsedTxt, eos))
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": failed to list " << quote(sourceStr) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            std::istringstream iss(parsedTxt);
                            while(getline(iss, sourceStr))
                                params.push_back(sourceStr);

                            continue;
                        }
                        else
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot copy ";
                            eos << quote(params[p]) << " as path does not exist" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    target.file = source.file;
                    targetStr = target.str();

                    if(overwrite || !file_exists(targetStr))
                    {
                        if(!ifNew || source.modified_after(target))
                        {
                            if(changePermissions && file_exists(targetStr))
                                chmod(targetStr.c_str(), 0666);

                            if((prompt && promptInput != 'a') || verbose)
                            {
                                if(targetParam.size())
                                {
                                    endChar = targetParam[targetParam.size()-1];
                                    if(endChar != '/' && endChar != '\\')
                                        targetParam += '/';
                                }

                                eos << quote(params[p]) << " -> " << quote(targetParam + source.file);

                                if(prompt && promptInput != 'a')
                                {
                                    eos << "? " << std::flush;
                                    promptInput = nsm_getch();
                                    if(promptInput == 'Y' || promptInput == '1')
                                        promptInput = 'y';
                                    else if(promptInput == 'A')
                                        promptInput = 'a';
                                }
                                
                                eos << std::endl;
                            }

                            if(!prompt || promptInput == 'y' || promptInput == 'a')
                            {
                                if(backupFiles && path_exists(targetStr))
                                    rename(targetStr.c_str(), (targetStr + "~").c_str());


                                if(dir_exists(sourceStr))
                                {
                                    if(cpDir(sourceStr, targetStr, sLineNo, readPath, eos, consoleLocked, os_mtx))
                                    {
                                        if(targetParam.size())
                                        {
                                            endChar = targetParam[targetParam.size()-1];
                                            if(endChar != '/' && endChar != '\\')
                                                targetParam += '/';
                                        }

                                        if(!consoleLocked)
                                            os_mtx->lock();
                                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to copy directory ";
                                        eos << quote(params[p]) << " to " << quote(targetParam + source.file) << std::endl;
                                        os_mtx->unlock();
                                        return 1;
                                    }
                                }
                                else if(cpFile(sourceStr, targetStr, sLineNo, readPath, eos, consoleLocked, os_mtx))
                                {
                                    if(targetParam.size())
                                    {
                                        endChar = targetParam[targetParam.size()-1];
                                        if(endChar != '/' && endChar != '\\')
                                            targetParam += '/';
                                    }

                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to copy file ";
                                    eos << quote(params[p]) << " to " << quote(targetParam + source.file) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                        }
                    }
                }

                if(changePermissions)
                    chmod(target.dir.c_str(), oldDirPerms.st_mode);
            }
            
            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "const")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ',', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            std::string toParse = "@:=";
            toParse += "{const";
            for(size_t o=0; o<options.size(); ++o)
                toParse += ", " + options[o];
            toParse += '}';
            if(params.size())
            {
                int depth=0;
                for(size_t pos=0; pos<params[0].size(); ++pos)
                {
                    if(params[0][pos] == '<' || params[0][pos] == '[')
                        ++depth;
                    else if(params[0][pos] == '>' || params[0][pos] == ']')
                        --depth;
                    else if(params[0][pos] == '"')
                        pos = params[0].find_first_of('"', pos+1);
                    else if(params[0][pos] == '\'')
                        pos = params[0].find_first_of('\'', pos+1);
                    else if(depth == 0 && params[0][pos] == ' ')
                    {
                        params[0][pos] = ',';
                        break;
                    }
                }
                toParse += "(" + params[0];
                for(size_t p=1; p<params.size(); ++p)
                    toParse += ", " + params[p];
                toParse += ')';
            }

            if(n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) <<  "const: constant definition failed" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "cd")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "cd: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string target = replace_home_dir(params[0]);

            if(!dir_exists(target))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "cd: cannot change directory to " << quote(params[0]) << " as it is not a directory" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(chdir(target.c_str()))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "cd: failed to change directory to " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "console")
        {
            bool readBlock = 0, parseBlock = 1;
            int bLineNo;
            std::string txt;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        readBlock = 1;
                    else if(options[o] == "!pb")
                        parseBlock = 0;
                }
            }

            if(readBlock)
            {
                if(params.size() != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "console{block}: expected 0 parameters, got " << params.size() << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(read_block(txt, linePos, inStr, readPath, lineNo, bLineNo, "console", eos))
                   return 1;

                if(parseBlock && parse_replace(lang, txt, "console block", readPath, antiDepsOfReadPath, bLineNo, "console", sLineNo, eos))
                    return 1;
            }

            VPos vpos;
            std::string gIndentAmount;
            for(size_t p=0; p<params.size(); p++)
            {
                if(params[p] == "endl")
                    txt += "\r\n";
                else if(replaceVars && vars.find(params[p], vpos))
                {
                    gIndentAmount = "";
                    if(!vars.add_str_from_var(vpos, txt, 1, indent, gIndentAmount))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "console: cannot get string of var type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                    txt += params[p];
            }

            if(!consoleLocked)
                os_mtx->lock();
            if(lolcat)
                rnbwcout(txt);
            else
                eos << txt << std::endl;
            if(!consoleLocked)
                os_mtx->unlock();

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "console.lock")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "console.lock: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(consoleLocked)
            {
                //os_mtx->lock(); //is already locked from this thread
                start_err_ml(eos, readPath, sLineNo, lineNo) << "console.lock: console is already locked" << std::endl;
                os_mtx->unlock();
                consoleLocked = 0;
                return 1;
            }
            else
            {
                consoleLocked = 1;
                consoleLockedOnLine = lineNo;
                consoleLockedPath = readPath;
            }

            os_mtx->lock();

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "console.unlock")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "console.unlock: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(consoleLocked)
                consoleLocked = 0;
            else
            {
                os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "console.unlock: console is not locked from this thread" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            os_mtx->unlock();

            if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "console.locked")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "console.locked: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            outStr += std::to_string(consoleLocked);
            if(indent)
                indentAmount += " ";

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
            else if(funcName == "cat")
                funcName = "type";
            else if(funcName == "cp")
                funcName = "copy";
        #else
            else if(funcName == "copy")
                funcName = "cp";
        #endif
        else if(funcName == "cssinclude")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "cssinclude: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string cssPathStr = params[0];
            Path cssPath;
            cssPath.set_file_path_from(cssPathStr);

            //warns user if css file doesn't exist
            if(!file_exists(cssPathStr.c_str()))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "cssinclude: css file " << cssPath << " does not exist" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            Path pathToCSSFile(pathBetween(toBuild.outputPath.dir, cssPath.dir), cssPath.file);

            std::string cssInclude = "<link rel=\"stylesheet\" type=\"text/css\" href=\"" + pathToCSSFile.str() + "\"";
            if(options.size())
                for(size_t o=0; o<options.size(); ++o)
                    cssInclude += " " + options[o];
            cssInclude += ">";

            outStr += cssInclude;
            if(indent)
                indentAmount += into_whitespace(cssInclude);

            return 0;
        }
    }
    else if(funcName[0] == '!')
    {
        if(funcName == "!")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "!: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "!", eos))
                return 1;

            bool result;

            if(!get_bool(result, params[0], readPath, lineNo, "!", eos))
                return 1;

            outStr += std::to_string(!result);
            indentAmount += " ";

            return 0;
        }
        else if(funcName == "!=")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "!=: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "!=", eos))
                return 1;

            bool doDouble = 0, doString = 0;

            for(size_t p=0; p<params.size(); p++)
            {
                if(isDouble(params[p]))
                {
                    if(!doDouble && !isInt(params[p]))
                        doDouble = 1;
                }
                else
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=0; p+1<params.size(); p++)
                {
                    for(size_t q=p+1; q<params.size(); q++)
                    {
                        if(params[p] == params[q])
                        {
                            result = "0";
                            break;
                        }
                    }
                }
            }
            else if(doDouble)
            {
                std::vector<double> doubles(params.size(), 0.0);

                for(size_t p=0; p<params.size(); p++)
                    doubles[p] = std::strtod(params[p].c_str(), NULL);

                for(size_t d=0; d+1<doubles.size(); d++)
                {
                    for(size_t e=d+1; e<doubles.size(); e++)
                    {
                        if(doubles[d] == doubles[e])
                        {
                            d = doubles.size();
                            result = "0";
                            break;
                        }
                    }
                }
            }
            else
            {
                std::vector<double> ints(params.size(), 0);

                for(size_t p=0; p<params.size(); p++)
                    ints[p] = std::atoi(params[p].c_str());

                for(size_t i=0; i+1<ints.size(); i++)
                {
                    for(size_t j=i+1; j<ints.size(); j++)
                    {
                        if(ints[i] == ints[j])
                        {
                            i = ints.size();
                            result = "0";
                            break;
                        }
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }
    else if(funcName[0] == '|')
    {
        if(funcName == "||")
        {
            bool return_value = 0;

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "||", eos))
                return 1;

            if(read_params(params, linePos, inStr, readPath, lineNo, funcName, eos))
                return 1;

            if(doNotParse)
                parseParams = 0;
            else
                parseParams = 1;

            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "||: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            for(size_t p=0; p<params.size(); p++)
            {
                if(parseParams && parse_replace(lang, params[p], "||(param[" + std::to_string(p) + "])", readPath, antiDepsOfReadPath, lineNo, "@||", sLineNo, eos))
                    return 1;

                if(replace_var(params[p], readPath, conditionLineNo, "||", eos))
                    return 1;

                if(!get_bool(return_value, params[p], readPath, conditionLineNo, "||", eos))
                    return 1;

                if(return_value)
                    break;
            }

            outStr += std::to_string(return_value);
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }
    else if(funcName[0] == '&')
    {
        if(funcName == "&&")
        {
            bool return_value = 1;

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "&&", eos))
                return 1;

            if(read_params(params, linePos, inStr, readPath, lineNo, funcName, eos))
                return 1;

            if(doNotParse)
                parseParams = 0;
            else
                parseParams = 1;

            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "&&: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            for(size_t p=0; p<params.size(); p++)
            {
                if(parseParams && parse_replace(lang, params[p], "&&(param[" + std::to_string(p) + "])", readPath, antiDepsOfReadPath, lineNo, "@&&", sLineNo, eos))
                    return 1;

                if(replace_var(params[p], readPath, conditionLineNo, "&&", eos))
                    return 1;

                if(!get_bool(return_value, params[p], readPath, conditionLineNo, "&&", eos))
                    return 1;

                if(!return_value)
                    break;
            }

            outStr += std::to_string(return_value);
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }
    else if(funcName[0] == '=')
    {
        if(funcName == "==")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "==: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "==", eos))
                return 1;

            bool doDouble = 0, doString = 0;

            for(size_t p=0; p<params.size(); p++)
            {
                if(isDouble(params[p]))
                {
                    if(!doDouble && !isInt(params[p]))
                        doDouble = 1;
                }
                else
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                {
                    if(params[p-1] != params[p])
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else if(doDouble)
            {
                double cDouble, nDouble = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                {
                    cDouble = nDouble;
                    nDouble = std::strtod(params[p].c_str(), NULL);
                    if(cDouble != nDouble)
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else
            {
                int cInt, nInt = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                {
                    cInt = nInt;
                    nInt = std::atoi(params[p].c_str());
                    if(cInt != nInt)
                    {
                        result = "0";
                        break;
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
        else if(funcName == "=")
        {
            if(params.size() < 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "=: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool readBlock = 0, parseBlock = 0;
            int bLineNo;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        readBlock = 1;
                    else if(options[o] == "pb")
                        parseBlock = 1;
                }
            }

            if(readBlock)
            {
                params.push_back("");
                if(read_block(params[params.size()-1], linePos, inStr, readPath, lineNo, bLineNo, "=", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, params[params.size()-1], "= block", readPath, antiDepsOfReadPath, bLineNo, "=", sLineNo, eos))
                    return 1;
            }
            else if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "=: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            std::string value = params[params.size()-1];
            for(size_t p=0; p+1<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(!set_var_from_str(vpos, value, readPath, sLineNo, "=", eos))
                        return 1;
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "=: no variable named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == '<')
    {
        if(funcName == "<")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "<: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "<", eos))
                return 1;

            bool doDouble = 0, doString = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                {
                    if(params[p-1] >= params[p])
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else if(doDouble)
            {
                double cDouble, nDouble = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                {
                    cDouble = nDouble;
                    nDouble = std::strtod(params[p].c_str(), NULL);
                    if(cDouble >= nDouble)
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else
            {
                int cInt, nInt = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                {
                    cInt = nInt;
                    nInt = std::atoi(params[p].c_str());
                    if(cInt >= nInt)
                    {
                        result = "0";
                        break;
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
        else if(funcName == "<=")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "<=: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "<=", eos))
                return 1;

            bool doDouble = 0, doString = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                {
                    if(params[p-1] > params[p])
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else if(doDouble)
            {
                double cDouble, nDouble = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                {
                    cDouble = nDouble;
                    nDouble = std::strtod(params[p].c_str(), NULL);
                    if(cDouble > nDouble)
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else
            {
                int cInt, nInt = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                {
                    cInt = nInt;
                    nInt = std::atoi(params[p].c_str());
                    if(cInt > nInt)
                    {
                        result = "0";
                        break;
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }
    else if(funcName[0] == '>')
    {
        if(funcName == ">")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << ">: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, ">", eos))
                return 1;

            bool doDouble = 0, doString = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                {
                    if(params[p-1] <= params[p])
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else if(doDouble)
            {
                double cDouble, nDouble = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                {
                    cDouble = nDouble;
                    nDouble = std::strtod(params[p].c_str(), NULL);
                    if(cDouble <= nDouble)
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else
            {
                int cInt, nInt = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                {
                    cInt = nInt;
                    nInt = std::atoi(params[p].c_str());
                    if(cInt <= nInt)
                    {
                        result = "0";
                        break;
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
        else if(funcName == ">=")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << ">=: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, ">=", eos))
                return 1;

            bool doDouble = 0, doString = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            std::string result = "1";

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                {
                    if(params[p-1] <= params[p])
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else if(doDouble)
            {
                double cDouble, nDouble = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                {
                    cDouble = nDouble;
                    nDouble = std::strtod(params[p].c_str(), NULL);
                    if(cDouble < nDouble)
                    {
                        result = "0";
                        break;
                    }
                }
            }
            else
            {
                int cInt, nInt = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                {
                    cInt = nInt;
                    nInt = std::atoi(params[p].c_str());
                    if(cInt < nInt)
                    {
                        result = "0";
                        break;
                    }
                }
            }

            outStr += result;
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }
    else if(funcName[0] == '+')
    {
        if(funcName == "+")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "+: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "+", eos))
                return 1;

            bool doDouble = 0, doString = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    doString = 1;
                    break;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                    {
                        doDouble = 1;
                        doString = 0;
                    }
                    else if(options[o] == "s" || options[o] == "string")
                    {
                        doString = 1;
                        doDouble = 0;
                    }
                }
            }

            if(doString)
            {
                for(size_t p=1; p<params.size(); p++)
                    params[0] += params[p];
            }
            else if(doDouble)
            {
                double ans = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                    ans += std::strtod(params[p].c_str(), NULL);

                params[0] = vars.double_to_string(ans, 0);
            }
            else
            {
                int ans = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                    ans += std::atoi(params[p].c_str());

                params[0] = std::to_string(ans);
            }

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "+=")
        {
            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "+=: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            if(vars.find(params[0], vpos))
            {
                if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "+=", eos))
                    return 1;

                bool doDouble = 0, doString = 0;
                int typeInt;

                for(size_t p=0; p<params.size(); p++)
                {
                    typeInt = getTypeInt(params[p]);

                    if(typeInt == 1)
                        doDouble = 1;
                    else if(typeInt == 2)
                    {
                        doString = 1;
                        break;
                    }
                }

                if(options.size())
                {
                    for(size_t o=0; o<options.size(); o++)
                    {
                        if(options[o] == "d" || options[o] == "double")
                        {
                            doDouble = 1;
                            doString = 0;
                        }
                        else if(options[o] == "s" || options[o] == "string")
                        {
                            doString = 1;
                            doDouble = 0;
                        }
                    }
                }

                if(doString)
                {
                    for(size_t p=1; p<params.size(); p++)
                        params[0] += params[p];
                }
                else if(doDouble)
                {
                    double ans = std::strtod(params[0].c_str(), NULL);
                    for(size_t p=1; p<params.size(); p++)
                        ans += std::strtod(params[p].c_str(), NULL);

                    params[0] = vars.double_to_string(ans, 0);
                }
                else
                {
                    int ans = std::atoi(params[0].c_str());
                    for(size_t p=1; p<params.size(); p++)
                        ans += std::atoi(params[p].c_str());

                    params[0] = std::to_string(ans);
                }

                if(!set_var_from_str(vpos, params[0], readPath, sLineNo, "+=", eos))
                    return 1;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "+=: there are no variables defined as " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "++")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "++: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vars.layers[vpos.layer].constants.count(params[p]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "++: attempted illegal change of constant variable " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[vpos.layer].privates.count(params[p]))
                    {
                        if(!vars.layers[vpos.layer].inScopes[params[p]].count(vars.layers[vars.layers.size()-1].scope))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "++: attempted illegal change of private variable " << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(p == 0 && addOutput)
                    {
                        if(!vars.add_str_from_var(vpos, outStr, 1, indent, indentAmount))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "++: cannot get string of var type " << quote(vpos.type) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(vpos.type.substr(0, 5) == "std::")
                    {
                        if(vpos.type == "std::int")
                            ++vars.layers[vpos.layer].ints[params[p]];
                        else if(vpos.type == "std::bool")
                        {
                            if(vars.layers[vpos.layer].bools[params[p]])
                                vars.layers[vpos.layer].bools[params[p]] = 0;
                            else
                                vars.layers[vpos.layer].bools[params[p]] = 1;
                        }
                        else if(vpos.type == "std::double")
                            ++vars.layers[vpos.layer].doubles[params[p]];
                        else if(vpos.type == "std::char")
                            ++vars.layers[vpos.layer].chars[params[p]];
                        else if(vpos.type == "std::llint")
                            ++vars.layers[vpos.layer].llints[params[p]];
                    }
                    else if(vpos.type == "int")
                        ++vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "bool")
                    {
                        if(vars.layers[vpos.layer].doubles[params[p]])
                            vars.layers[vpos.layer].doubles[params[p]] = 0;
                        else
                            vars.layers[vpos.layer].doubles[params[p]] = 1;
                    }
                    else if(vpos.type == "double")
                        ++vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "char" && vars.layers[vpos.layer].strings[params[p]].size())
                        ++vars.layers[vpos.layer].strings[params[p]][0];
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "++: operator not defined for variable ";
                        eos << quote(params[p]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "++: no variable named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "+++")
        {
            if(hasIncrement)
                funcName = "++";

            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vars.layers[vpos.layer].constants.count(params[p]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": attempted illegal change of constant variable " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[vpos.layer].privates.count(params[p]))
                    {
                        if(!vars.layers[vpos.layer].inScopes[params[p]].count(vars.layers[vars.layers.size()-1].scope))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": attempted illegal change of private variable " << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(vpos.type.substr(0, 5) == "std::")
                    {
                        if(vpos.type == "std::int")
                            ++vars.layers[vpos.layer].ints[params[p]];
                        else if(vpos.type == "std::bool")
                        {
                            if(vars.layers[vpos.layer].bools[params[p]])
                                vars.layers[vpos.layer].bools[params[p]] = 0;
                            else
                                vars.layers[vpos.layer].bools[params[p]] = 1;
                        }
                        else if(vpos.type == "std::double")
                            ++vars.layers[vpos.layer].doubles[params[p]];
                        else if(vpos.type == "std::char")
                            ++vars.layers[vpos.layer].chars[params[p]];
                        else if(vpos.type == "std::llint")
                            ++vars.layers[vpos.layer].llints[params[p]];
                    }
                    else if(vpos.type == "int")
                        ++vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "bool")
                    {
                        if(vars.layers[vpos.layer].doubles[params[p]])
                            vars.layers[vpos.layer].doubles[params[p]] = 0;
                        else
                            vars.layers[vpos.layer].doubles[params[p]] = 1;
                    }
                    else if(vpos.type == "double")
                        ++vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "char" && vars.layers[vpos.layer].strings[params[p]].size())
                        ++vars.layers[vpos.layer].strings[params[p]][0];
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": operator not defined for variable ";
                        eos << quote(params[p]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(p == 0 && addOutput)
                    {
                        if(!vars.add_str_from_var(vpos, outStr, 1, indent, indentAmount))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot get string of var type " << quote(vpos.type) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": no variable named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == '-')
    {
        if(funcName == "-")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "-: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "-", eos))
                return 1;

            bool doDouble = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "-: cannot perform operator on " << params[p] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                        doDouble = 1;
                }
            }

            if(doDouble)
            {
                double ans;
                if(params.size() == 1)
                    ans = -std::strtod(params[0].c_str(), NULL);
                else
                {
                    ans = std::strtod(params[0].c_str(), NULL);
                    for(size_t p=1; p<params.size(); p++)
                        ans -= std::strtod(params[p].c_str(), NULL);
                }

                params[0] = vars.double_to_string(ans, 0);
            }
            else
            {
                int ans;
                if(params.size() == 1)
                    ans = -std::atoi(params[0].c_str());
                else
                {
                    ans = std::atoi(params[0].c_str());
                    for(size_t p=1; p<params.size(); p++)
                        ans -= std::atoi(params[p].c_str());
                }

                params[0] = std::to_string(ans);
            }

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "-=")
        {
            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "-=: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "-=", eos))
                    return 1;

                bool doDouble = 0;
                int typeInt;

                for(size_t p=0; p<params.size(); p++)
                {
                    typeInt = getTypeInt(params[p]);

                    if(typeInt == 1)
                        doDouble = 1;
                    else if(typeInt == 2)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "-=: operator not defined for parameter " << p << " = " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(options.size())
                {
                    for(size_t o=0; o<options.size(); o++)
                    {
                        if(options[o] == "d" || options[o] == "double")
                            doDouble = 1;
                    }
                }

                if(doDouble)
                {
                    double ans = std::strtod(params[0].c_str(), NULL);
                    for(size_t p=1; p<params.size(); p++)
                        ans -= std::strtod(params[p].c_str(), NULL);

                    params[0] = vars.double_to_string(ans, 0);
                }
                else
                {
                    int ans = std::atoi(params[0].c_str());
                    for(size_t p=1; p<params.size(); p++)
                        ans -= std::atoi(params[p].c_str());

                    params[0] = std::to_string(ans);
                }

                if(!set_var_from_str(vpos, params[0], readPath, sLineNo, "-=", eos))
                    return 1;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "-=: there are no variables defined as " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "--")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "--: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vars.layers[vpos.layer].constants.count(params[p]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "--: attempted illegal change of constant variable " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[vpos.layer].privates.count(params[p]))
                    {
                        if(!vars.layers[vpos.layer].inScopes[params[p]].count(vars.layers[vars.layers.size()-1].scope))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "--: attempted illegal change of private variable " << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(p == 0 && addOutput)
                    {
                        if(!vars.add_str_from_var(vpos, outStr, 1, indent, indentAmount))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "--: cannot get string of var type " << quote(vpos.type) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(vpos.type.substr(0, 5) == "std::")
                    {
                        if(vpos.type == "std::int")
                            --vars.layers[vpos.layer].ints[params[p]];
                        else if(vpos.type == "std::bool")
                        {
                            if(vars.layers[vpos.layer].bools[params[p]])
                                vars.layers[vpos.layer].bools[params[p]] = 0;
                            else
                                vars.layers[vpos.layer].bools[params[p]] = 1;
                        }
                        else if(vpos.type == "std::double")
                            --vars.layers[vpos.layer].doubles[params[p]];
                        else if(vpos.type == "std::char")
                            --vars.layers[vpos.layer].chars[params[p]];
                        else if(vpos.type == "std::llint")
                            --vars.layers[vpos.layer].llints[params[p]];
                    }
                    else if(vpos.type == "int")
                        --vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "bool")
                    {
                        if(vars.layers[vpos.layer].doubles[params[p]])
                            vars.layers[vpos.layer].doubles[params[p]] = 0;
                        else
                            vars.layers[vpos.layer].doubles[params[p]] = 1;
                    }
                    else if(vpos.type == "double")
                        --vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "char" && vars.layers[vpos.layer].strings[params[p]].size())
                        --vars.layers[vpos.layer].strings[params[p]][0];
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "--: operator not defined for variable ";
                        eos << quote(params[p]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "--: no variable named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "---")
        {
            if(hasIncrement)
                funcName = "--";

            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vars.layers[vpos.layer].constants.count(params[p]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": attempted illegal change of constant variable " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[vpos.layer].privates.count(params[p]))
                    {
                        if(!vars.layers[vpos.layer].inScopes[params[p]].count(vars.layers[vars.layers.size()-1].scope))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": attempted illegal change of private variable " << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(vpos.type.substr(0, 5) == "std::")
                    {
                        if(vpos.type == "std::int")
                            --vars.layers[vpos.layer].ints[params[p]];
                        else if(vpos.type == "std::bool")
                        {
                            if(vars.layers[vpos.layer].bools[params[p]])
                                vars.layers[vpos.layer].bools[params[p]] = 0;
                            else
                                vars.layers[vpos.layer].bools[params[p]] = 1;
                        }
                        else if(vpos.type == "std::double")
                            --vars.layers[vpos.layer].doubles[params[p]];
                        else if(vpos.type == "std::char")
                            --vars.layers[vpos.layer].chars[params[p]];
                        else if(vpos.type == "std::llint")
                            --vars.layers[vpos.layer].llints[params[p]];
                    }
                    else if(vpos.type == "int")
                        --vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "bool")
                    {
                        if(vars.layers[vpos.layer].doubles[params[p]])
                            vars.layers[vpos.layer].doubles[params[p]] = 0;
                        else
                            vars.layers[vpos.layer].doubles[params[p]] = 1;
                    }
                    else if(vpos.type == "double")
                        --vars.layers[vpos.layer].doubles[params[p]];
                    else if(vpos.type == "char" && vars.layers[vpos.layer].strings[params[p]].size())
                        --vars.layers[vpos.layer].strings[params[p]][0];
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": operator not defined for variable ";
                        eos << quote(params[p]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(p == 0 && addOutput)
                    {
                        if(!vars.add_str_from_var(vpos, outStr, 1, indent, indentAmount))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot get string of var type " << quote(vpos.type) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": no variable named " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == '*')
    {
        if(funcName == "*")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "*: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "*", eos))
                return 1;

            bool doDouble = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "*: cannot perform operator on " << params[p] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                        doDouble = 1;
                }
            }

            if(doDouble)
            {
                double ans = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                    ans *= std::strtod(params[p].c_str(), NULL);

                params[0] = vars.double_to_string(ans, 0);
            }
            else
            {
                int ans = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                    ans *= std::atoi(params[p].c_str());

                params[0] = std::to_string(ans);
            }

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "*=")
        {
            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "*=: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            if(vars.find(params[0], vpos))
            {
                if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "*=", eos))
                    return 1;

                bool doDouble = 0;
                int typeInt;

                for(size_t p=0; p<params.size(); p++)
                {
                    typeInt = getTypeInt(params[p]);

                    if(typeInt == 1)
                        doDouble = 1;
                    else if(typeInt == 2)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "*=: operator not defined for parameter " << p << " = " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(options.size())
                {
                    for(size_t o=0; o<options.size(); o++)
                    {
                        if(options[o] == "d" || options[o] == "double")
                            doDouble = 1;
                    }
                }

                if(doDouble)
                {
                    double ans = std::strtod(params[0].c_str(), NULL);
                    for(size_t p=1; p<params.size(); p++)
                        ans *= std::strtod(params[p].c_str(), NULL);

                    params[0] = vars.double_to_string(ans, 0);
                }
                else
                {
                    int ans = std::atoi(params[0].c_str());
                    for(size_t p=1; p<params.size(); p++)
                        ans *= std::atoi(params[p].c_str());

                    params[0] = std::to_string(ans);
                }

                if(!set_var_from_str(vpos, params[0], readPath, sLineNo, "*=", eos))
                    return 1;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "*=: there are no variables defined as " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
    }
    else if(funcName[0] == '/')
    {
        if(funcName == "/")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "/: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool oldFP = vars.fixedPrecision;
            vars.fixedPrecision = 1;

            /*if(parse_replace(lang, params, "/ params", readPath, antiDepsOfReadPath, sLineNo, "@/()", sLineNo, eos))
                return 1;*/ //should change fixed precision earlier for this function!!

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "/", eos))
                return 1;

            vars.fixedPrecision = oldFP;

            bool doDouble = 0;
            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt == 1)
                    doDouble = 1;
                else if(typeInt == 2)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "/: cannot perform operator on " << params[p] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "d" || options[o] == "double")
                        doDouble = 1;
                }
            }

            if(doDouble)
            {
                double ans = std::strtod(params[0].c_str(), NULL);
                for(size_t p=1; p<params.size(); p++)
                    ans /= std::strtod(params[p].c_str(), NULL);

                params[0] = vars.double_to_string(ans, 0);
            }
            else
            {
                int ans = std::atoi(params[0].c_str());
                for(size_t p=1; p<params.size(); p++)
                    ans /= std::atoi(params[p].c_str());

                params[0] = std::to_string(ans);
            }

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "/=")
        {
            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "/=: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            if(vars.find(params[0], vpos))
            {
                bool oldFP = vars.fixedPrecision;
                vars.fixedPrecision = 1;

                /*if(parse_replace(lang, params, "/ params", readPath, antiDepsOfReadPath, sLineNo, "@/()", sLineNo, eos))
                    return 1;*/ //should change fixed precision earlier for this function!!

                if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "/=", eos))
                    return 1;

                vars.fixedPrecision = oldFP;

                bool doDouble = 0;
                int typeInt;

                for(size_t p=0; p<params.size(); p++)
                {
                    typeInt = getTypeInt(params[p]);

                    if(typeInt == 1)
                        doDouble = 1;
                    else if(typeInt == 2)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "/=: operator not defined for parameter " << p << " = " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(options.size())
                {
                    for(size_t o=0; o<options.size(); o++)
                    {
                        if(options[o] == "d" || options[o] == "double")
                            doDouble = 1;
                    }
                }

                if(doDouble)
                {
                    double ans = std::strtod(params[0].c_str(), NULL);
                    for(size_t p=1; p<params.size(); p++)
                        ans /= std::strtod(params[p].c_str(), NULL);

                    params[0] = vars.double_to_string(ans, 0);
                }
                else
                {
                    int ans = std::atoi(params[0].c_str());
                    for(size_t p=1; p<params.size(); p++)
                        ans /= std::atoi(params[p].c_str());

                    params[0] = std::to_string(ans);
                }

                if(!set_var_from_str(vpos, params[0], readPath, sLineNo, "/=", eos))
                    return 1;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "/=: there are no variables defined as " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
    }
    else if(funcName[0] == '%')
    {
        if(funcName == "%")
        {
            if(params.size() != 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "%: expected 2 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool oldFP = vars.fixedPrecision;
            vars.fixedPrecision = 1;

            /*if(parse_replace(lang, params, "/ params", readPath, antiDepsOfReadPath, sLineNo, "@/()", sLineNo, eos))
                return 1;*/ //should change fixed precision earlier for this function!!

            if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "%", eos))
                return 1;

            vars.fixedPrecision = oldFP;

            int typeInt;

            for(size_t p=0; p<params.size(); p++)
            {
                typeInt = getTypeInt(params[p]);

                if(typeInt != 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "%: cannot perform operator on " << params[p] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            int ans = std::atoi(params[0].c_str())%std::atoi(params[1].c_str());

            params[0] = std::to_string(ans);

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "%=")
        {
            if(params.size() != 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "%=: expected 2 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            if(vars.find(params[0], vpos))
            {
                bool oldFP = vars.fixedPrecision;
                vars.fixedPrecision = 1;

                /*if(parse_replace(lang, params, "/ params", readPath, antiDepsOfReadPath, sLineNo, "@%()", sLineNo, eos))
                    return 1;*/ //should change fixed precision earlier for this function!!

                if(replaceVars && replace_vars(params, 0, readPath, sLineNo, "%=", eos))
                    return 1;

                vars.fixedPrecision = oldFP;

                int typeInt;

                for(size_t p=0; p<params.size(); p++)
                {
                    typeInt = getTypeInt(params[p]);

                    if(typeInt != 0)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "%=: operator not defined for parameter " << p << " = " << quote(params[p]) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                int ans = std::atoi(params[0].c_str())%std::atoi(params[1].c_str());

                params[0] = std::to_string(ans);

                if(!set_var_from_str(vpos, params[0], readPath, sLineNo, "%=", eos))
                    return 1;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "%=: there are no variables defined as " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
    }
    else if(funcName[0] == ':')
    {
        if(funcName == ":=")
        {
            std::string varType;
            std::vector<std::pair<std::string, std::vector<std::string> > > inputVars;
            bool constants = 0, privates = 0, addToExpr = 1;
            int pOption = -1;
            size_t layer = vars.layers.size()-1;
            std::string vScope, bScope;
            std::unordered_set<std::string> inScopes;

            size_t pos=0;
            if(read_def(varType, inputVars, pos, paramsStr, readPath, lineNo, ":=", eos))
                return 1;

            /*if(!valid_type(varType, readPath, antiDepsOfReadPath, sLineNo, ":=()", sLineNo, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: invalid type: " << quote(varType) << std::endl;
                os_mtx->unlock();
                return 1;
            }*/

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "!exprtk")
                        addToExpr = 0;
                    if(options[o] == "const")
                        constants = 1;
                    else if(options[o] == "private")
                    {
                        privates = 1;
                        pOption = o;
                    }
                    else if(options[o].substr(0, 6) == "layer=")
                    {
                        std::string str = unquote(options[o].substr(6, options[o].size()-6));

                        if(!isNonNegInt(str))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: specified layer " << quote(str) << " is not a non-negative integer" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        layer = std::atoi(str.c_str());
                        if(layer >= vars.layers.size())
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: specified layer " << quote(str) << " should be less than the number of layers" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(options[o].substr(0, 7) == "scope+=")
                    {
                        privates = 1;
                        inScopes.insert(unquote(options[o].substr(7, options[o].size()-7)));
                    }
                }
            }

            bScope = vars.layers[layer].scope;
            if(pOption != -1)
                options[pOption] = "scope+=" + vars.layers[layer].scope;

            if(vars.basic_types.count(varType))
            {
                for(size_t v=0; v<inputVars.size(); v++)
                {
                    //checks whether variable exists at current scope
                    if(inputVars[v].first == "")
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: cannot have variable named " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[layer].typeOf.count(inputVars[v].first))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: redeclaration of variable/function name " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.typeDefs.count(inputVars[v].first))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: cannot have type and variable named " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else
                    {
                        vars.layers[layer].typeOf[inputVars[v].first] = varType;
                        int pos = find_last_of_special(inputVars[v].first);
                        if(pos)
                            vScope = bScope + inputVars[v].first.substr(0, pos) + ".";
                        else
                            vScope = bScope;
                        vars.layers[layer].inScopes[inputVars[v].first].insert(vScope);
                        for(auto it=inScopes.begin(); it!=inScopes.end(); it++)
                            vars.layers[layer].inScopes[inputVars[v].first].insert(*it);
                        vars.layers[layer].scopeOf[inputVars[v].first] = vScope;

                        if(varType == "bool")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: bool definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isInt(inputVars[v].second[0]))
                            {
                                vars.layers[layer].doubles[inputVars[v].first] = (bool)std::atoi(inputVars[v].second[0].c_str());

                                if(addToExpr)
                                    symbol_table.add_variable(inputVars[v].first, vars.layers[layer].doubles[inputVars[v].first]);
                            }
                            else if(isDouble(inputVars[v].second[0]))
                            {
                                vars.layers[layer].doubles[inputVars[v].first] = (bool)std::strtod(inputVars[v].second[0].c_str(), NULL);

                                if(addToExpr)
                                    symbol_table.add_variable(inputVars[v].first, vars.layers[layer].doubles[inputVars[v].first]);
                            }
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not a boolean" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "int")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: int definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isInt(inputVars[v].second[0]))
                            {
                                vars.layers[layer].doubles[inputVars[v].first] = std::atoi(inputVars[v].second[0].c_str());

                                if(addToExpr)
                                    symbol_table.add_variable(inputVars[v].first, vars.layers[layer].doubles[inputVars[v].first]);
                            }
                            else if(isDouble(inputVars[v].second[0]))
                            {
                                vars.layers[layer].doubles[inputVars[v].first] = (int)std::strtod(inputVars[v].second[0].c_str(), NULL);

                                if(addToExpr)
                                    symbol_table.add_variable(inputVars[v].first, vars.layers[layer].doubles[inputVars[v].first]);
                            }
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an integer" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "double")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: double definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isDouble(inputVars[v].second[0]))
                            {
                                vars.layers[layer].doubles[inputVars[v].first] = std::strtod(inputVars[v].second[0].c_str(), NULL);

                                if(addToExpr)
                                    symbol_table.add_variable(inputVars[v].first, vars.layers[layer].doubles[inputVars[v].first]);
                            }
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not a double" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "char")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back(" ");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: character definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(inputVars[v].second[0].size() == 1)
                            {
                                vars.layers[layer].strings[inputVars[v].first] = inputVars[v].second[0];

                                if(addToExpr)
                                    symbol_table.add_stringvar(inputVars[v].first, vars.layers[layer].strings[inputVars[v].first]);
                            }
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not a character" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "string")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back(" ");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: string definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else
                            {
                                vars.layers[layer].strings[inputVars[v].first] = inputVars[v].second[0];

                                if(addToExpr)
                                    symbol_table.add_stringvar(inputVars[v].first, vars.layers[layer].strings[inputVars[v].first]);
                            }
                        }
                        else if(varType == "std::bool")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::bool definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isInt(inputVars[v].second[0]))
                                vars.layers[layer].bools[inputVars[v].first] = (bool)std::atoi(inputVars[v].second[0].c_str());
                            else if(isDouble(inputVars[v].second[0]))
                                vars.layers[layer].bools[inputVars[v].first] = (bool)std::strtod(inputVars[v].second[0].c_str(), NULL);
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an std::bool" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "std::int")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::int definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isInt(inputVars[v].second[0]))
                                vars.layers[layer].ints[inputVars[v].first] = std::atoi(inputVars[v].second[0].c_str());
                            else if(isDouble(inputVars[v].second[0]))
                                vars.layers[layer].ints[inputVars[v].first] = (int)std::strtod(inputVars[v].second[0].c_str(), NULL);
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an std::int" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "std::double")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::double definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isDouble(inputVars[v].second[0]))
                                vars.layers[layer].doubles[inputVars[v].first] = std::strtod(inputVars[v].second[0].c_str(), NULL);
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an std::double" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "std::char")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back(" ");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::char definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(inputVars[v].second[0].size() == 1)
                                vars.layers[layer].chars[inputVars[v].first] = inputVars[v].second[0][0];
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an std::char" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "std::string")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::string definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else
                                vars.layers[layer].strings[inputVars[v].first] = inputVars[v].second[0];
                        }
                        else if(varType == "std::llint")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            if(inputVars[v].second.size() != 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::llint definition for " << quote(inputVars[v].first) << " should have 1 input variable, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(isInt(inputVars[v].second[0]))
                                vars.layers[layer].llints[inputVars[v].first] = std::atoll(inputVars[v].second[0].c_str());
                            else if(isDouble(inputVars[v].second[0]))
                                vars.layers[layer].llints[inputVars[v].first] = (long long int)std::strtod(inputVars[v].second[0].c_str(), NULL);
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not an integer" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(varType == "std::vector<double>")
                        {
                            if(!inputVars[v].second.size())
                                inputVars[v].second.push_back("0");

                            int noParams = inputVars[v].second.size();

                            if(noParams > 2)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: std::vector<double> definition for " << quote(inputVars[v].first) << " should have 0-2 input variables, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            size_t vSize = 0;
                            double vValue = 0.0;

                            if(noParams > 0)
                            {
                                if(!isNonNegInt(inputVars[v].second[0]))
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: size given for " << quote(inputVars[v].first) << " is not a non-negative integer" << std::endl;
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: size given = " << quote(inputVars[v].second[0]) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                                else
                                    vSize = std::atoi(inputVars[v].second[0].c_str());
                            }

                            if(noParams == 2)
                            {
                                if(!isDouble(inputVars[v].second[1]))
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given for " << quote(inputVars[v].first) << " is not a double" << std::endl;
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: value given = " << quote(inputVars[v].second[1]) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                                else
                                    vValue = std::strtod(inputVars[v].second[1].c_str(), NULL);
                            }
                            vars.layers[layer].doubVecs[inputVars[v].first] = std::vector<double>(vSize, vValue);

                            if(addToExpr)
                            {
                                if(!vSize)
                                    vars.layers[layer].doubVecs[inputVars[v].first].push_back(0.0);
                                symbol_table.add_vector(inputVars[v].first, vars.layers[layer].doubVecs[inputVars[v].first]);
                                if(!vSize)
                                    vars.layers[layer].doubVecs[inputVars[v].first].pop_back();
                            }

                            if(add_fn(inputVars[v].first + ".push_back", 'n',
                                      "@std::vector.push_back{@join(options, ', ')}(" + inputVars[v].first + ", @join(params, ', ')" + ")",
                                      "function", layer, 1 /*isConst*/, privates, 1 /*?isUnscoped?*/, 1 /*addOut*/,
                                      vars.layers[layer].inScopes[inputVars[v].first], readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;
                        }
                        else if(varType == "fstream")
                        {
                            if(inputVars[v].second.size() > 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: fstream definition for " << quote(inputVars[v].first) << " should have 0-1 input variables, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(!inputVars[v].second.size())
                                vars.layers[layer].fstreams[inputVars[v].first] = std::fstream();
                            else
                            {
                                vars.layers[layer].fstreams[inputVars[v].first].open(inputVars[v].second[0]);

                                if(!vars.layers[layer].fstreams[inputVars[v].first].is_open())
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: failed to open path for " << quote(inputVars[v].first) << std::endl;
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: path given = " << quote(inputVars[v].second[0]) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }

                            //should add join of options to options for @close call
                            if(add_fn(inputVars[v].first + ".close", 'n', "@close(" + inputVars[v].first + ")", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;

                            if(add_fn(inputVars[v].first + ".open", 'n', "@open(" + inputVars[v].first + ", " + "$[params[0]])", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;
                        }
                        else if(varType == "ifstream")
                        {
                            if(inputVars[v].second.size() > 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: ifstream definition for " << quote(inputVars[v].first) << " should have 0-1 input variables, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(!inputVars[v].second.size())
                                vars.layers[layer].ifstreams[inputVars[v].first] = std::ifstream();
                            else if(file_exists(inputVars[v].second[0]))
                            {
                                vars.layers[layer].ifstreams[inputVars[v].first].open(inputVars[v].second[0]);

                                if(!vars.layers[layer].ifstreams[inputVars[v].first].is_open())
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: failed to open input path for " << quote(inputVars[v].first) << std::endl;
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: path given = " << quote(inputVars[v].second[0]) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                            else
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: path given for " << quote(inputVars[v].first) << " is not an existing file" << std::endl;
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: path given = " << quote(inputVars[v].second[0]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            if(add_fn(inputVars[v].first + ".close", 'n', "@close(" + inputVars[v].first + ")", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;

                            if(add_fn(inputVars[v].first + ".open", 'n', "@open(" + inputVars[v].first + ", " + "$[params[0]])", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;
                        }
                        else if(varType == "ofstream")
                        {
                            if(inputVars[v].second.size() > 1)
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: ofstream definition for " << quote(inputVars[v].first) << " should have 0-1 input variables, got " << inputVars[v].second.size() << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else if(!inputVars[v].second.size())
                                vars.layers[layer].ofstreams[inputVars[v].first] = std::ofstream();
                            else
                            {
                                vars.layers[layer].ofstreams[inputVars[v].first].open(inputVars[v].second[0]);

                                if(!vars.layers[layer].ofstreams[inputVars[v].first].is_open())
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: failed to open output path for " << quote(inputVars[v].first) << std::endl;
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: path given = " << quote(inputVars[v].second[0]) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }

                            if(add_fn(inputVars[v].first + ".close", 'n', "@close(" + inputVars[v].first + ")", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;

                            if(add_fn(inputVars[v].first + ".open", 'n', "@open(" + inputVars[v].first + ", " + "$[params[0]])", "function", layer, 1 /*isConst*/,
                                      privates, 1 /*?isUnscoped?*/, 1 /*addOut*/, vars.layers[layer].inScopes[inputVars[v].first],
                                      readPath, sLineNo, ":=(" + varType + ")", eos))
                                return 1;
                        }
                    }
                }
            }
            else if(valid_type(varType, readPath, antiDepsOfReadPath, lineNo, ":=(" + varType + ")", sLineNo, eos))
            {
                std::vector<std::string> types;
                std::string preType, typesStr;
                size_t pos = varType.find_first_of('<');
                if(pos < varType.size())
                {
                    preType = varType.substr(0, pos);
                    typesStr = varType.substr(pos, varType.size()-pos);

                    pos = 0;
                    if(read_params(types, pos, typesStr, readPath, lineNo, ":=(" + varType + ")", eos))
                        return 1;
                }
                else
                    preType = varType;

                for(size_t v=0; v<inputVars.size(); v++)
                {
                    //checks whether variable exists at current scope
                    if(inputVars[v].first == "")
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: cannot have variable named " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.layers[layer].typeOf.count(inputVars[v].first))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: redeclaration of variable/function name " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else if(vars.typeDefs.count(inputVars[v].first))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: cannot have type and variable named " << quote(inputVars[v].first) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else
                    {
                        /*if(preType == "array")
                        {
                            std::cout << "arrayay" << std::endl;
                        }
                        else if(preType == "map")
                        {
                            std::cout << "mapyay" << std::endl;
                        }
                        else if(preType == "vector")
                        {
                            std::cout << "vecyay" << std::endl;
                        }
                        else */if(vars.typeDefs.count(preType))
                        {
                            std::vector<bool> typToDel, optToDel, parToDel;
                            std::vector<bool> typWasConst, optWasConst, parWasConst;
                            std::vector<std::string> oldTypTypes, oldOptTypes, oldParTypes;
                            int old_no_types = 0, old_no_options = 0, old_no_params = 0;
                            std::vector<std::string> oldTyps, oldOpts, oldPars;

                            bool varNameToDel = 1, varNameWasConst = 0;
                            std::string oldVarNameType, oldVarName;

                            params = inputVars[v].second;

                            //backs up variables already defined at this scope
                            typToDel = std::vector<bool>(types.size() + 1, 1);
                            optToDel = std::vector<bool>(options.size() + 1, 1);
                            parToDel = std::vector<bool>(params.size() + 1, 1);
                            typWasConst = std::vector<bool>(types.size() + 1, 0);
                            optWasConst = std::vector<bool>(options.size() + 1, 0);
                            parWasConst = std::vector<bool>(params.size() + 1, 0);
                            oldTypTypes = std::vector<std::string>(types.size() + 1, "");
                            oldOptTypes = std::vector<std::string>(options.size() + 1, "");
                            oldParTypes = std::vector<std::string>(params.size() + 1, "");
                            oldTyps = std::vector<std::string>(types.size() + 1, "");
                            oldOpts = std::vector<std::string>(options.size() + 1, "");
                            oldPars = std::vector<std::string>(params.size() + 1, "");

                            if(vars.layers[layer].typeOf.count("types.size"))
                            {
                                typToDel[types.size()] = 0;
                                if(vars.layers[layer].constants.count("types.size"))
                                    typWasConst[types.size()] = 1;
                                oldTypTypes[types.size()] = vars.layers[layer].typeOf["types.size"];
                                if(oldTypTypes[types.size()] == "std::int")
                                    old_no_types = vars.layers[layer].ints["types.size"];
                            }

                            for(size_t t=0; t<types.size(); t++)
                            {
                                if(vars.layers[layer].typeOf.count("types[" + std::to_string(t) + "]"))
                                {
                                    typToDel[t] = 0;
                                    if(vars.layers[layer].constants.count("types[" + std::to_string(t) + "]"))
                                        typWasConst[t] = 1;
                                    oldTypTypes[t] = vars.layers[layer].typeOf["types[" + std::to_string(t) + "]"];
                                    if(oldTypTypes[t] == "string")
                                        oldTyps[t] = vars.layers[layer].strings["types[" + std::to_string(t) + "]"];
                                }
                            }


                            if(vars.layers[layer].typeOf.count("options.size"))
                            {
                                optToDel[options.size()] = 0;
                                if(vars.layers[layer].constants.count("options.size"))
                                    optWasConst[options.size()] = 1;
                                oldOptTypes[options.size()] = vars.layers[layer].typeOf["options.size"];
                                if(oldOptTypes[options.size()] == "std::int")
                                    old_no_options = vars.layers[layer].ints["options.size"];
                            }

                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(vars.layers[layer].typeOf.count("options[" + std::to_string(o) + "]"))
                                {
                                    optToDel[o] = 0;
                                    if(vars.layers[layer].constants.count("options[" + std::to_string(o) + "]"))
                                        optWasConst[o] = 1;
                                    oldOptTypes[o] = vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"];
                                    if(oldOptTypes[o] == "string")
                                        oldOpts[o] = vars.layers[layer].strings["options[" + std::to_string(o) + "]"];
                                }
                            }

                            if(vars.layers[layer].typeOf.count("params.name"))
                            {
                                varNameToDel = 0;
                                if(vars.layers[layer].constants.count("params.name"))
                                    varNameWasConst = 1;
                                oldVarNameType = vars.layers[layer].typeOf["params.name"];
                                if(oldVarNameType == "string")
                                    oldVarName = vars.layers[layer].strings["params.name"];
                            }

                            if(vars.layers[layer].typeOf.count("params.size"))
                            {
                                parToDel[params.size()] = 0;
                                if(vars.layers[layer].constants.count("params.size"))
                                    parWasConst[params.size()] = 1;
                                oldParTypes[params.size()] = vars.layers[layer].typeOf["params.size"];
                                if(oldParTypes[params.size()] == "std::int")
                                    old_no_params = vars.layers[layer].ints["params.size"];
                            }

                            for(size_t p=0; p<params.size(); p++)
                            {
                                if(vars.layers[layer].typeOf.count("params[" + std::to_string(p) + "]"))
                                {
                                    parToDel[p] = 0;
                                    if(vars.layers[layer].constants.count("params[" + std::to_string(p) + "]"))
                                        parWasConst[p] = 1;
                                    oldParTypes[p] = vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"];
                                    if(oldParTypes[p] == "string")
                                        oldPars[p] = vars.layers[layer].strings["params[" + std::to_string(p) + "]"];
                                }
                            }

                            //defines types/params/options
                            vars.layers[layer].typeOf[inputVars[v].first] = varType;

                            vars.layers[layer].typeOf["types.size"] = "std::int";
                            vars.layers[layer].ints["types.size"] = types.size();
                            vars.layers[layer].constants.insert("types.size");

                            for(size_t t=0; t<types.size(); t++)
                            {
                                vars.layers[layer].typeOf["types[" + std::to_string(t) + "]"] = "string";
                                vars.layers[layer].strings["types[" + std::to_string(t) + "]"] = types[t];
                                vars.layers[layer].constants.insert("types[" + std::to_string(t) + "]");
                            }

                            vars.layers[layer].typeOf["params.name"] = "string";
                            vars.layers[layer].strings["params.name"] = inputVars[v].first;
                            vars.layers[layer].constants.insert("params.name");

                            vars.layers[layer].typeOf["params.size"] = "std::int";
                            vars.layers[layer].ints["params.size"] = params.size();
                            vars.layers[layer].constants.insert("params.size");

                            for(size_t p=0; p<params.size(); p++)
                            {
                                vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"] = "string";
                                vars.layers[layer].strings["params[" + std::to_string(p) + "]"] = params[p];
                                vars.layers[layer].constants.insert("params[" + std::to_string(p) + "]");
                            }

                            vars.layers[layer].typeOf["options.size"] = "std::int";
                            vars.layers[layer].ints["options.size"] = options.size();
                            vars.layers[layer].constants.insert("options.size");

                            for(size_t o=0; o<options.size(); o++)
                            {
                                vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"] = "string";
                                vars.layers[layer].strings["options[" + std::to_string(o) + "]"] = options[o];
                                vars.layers[layer].constants.insert("options[" + std::to_string(o) + "]");
                            }

                            //parses typedef function
                            std::string defOutput;
                            if(vars.nTypes.count(preType))
                            {
                                if(n_read_and_process_fast(1, vars.typeDefs[preType], vars.typeDefLineNo[preType]-1, vars.typeDefPath[preType], antiDepsOfReadPath, defOutput, eos) > 0)
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=(" << varType << "): failed here" << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                            else 
                            {
                                if(f_read_and_process_fast(1, vars.typeDefs[preType], vars.typeDefLineNo[preType]-1, vars.typeDefPath[preType], antiDepsOfReadPath, defOutput, eos) > 0)
                                {
                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << ":=(" << varType << "): failed here" << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }

                            //reverts existing variables otherwise deletes them
                            if(!typWasConst[types.size()])
                                vars.layers[layer].constants.erase("types.size");
                            if(oldTypTypes[types.size()] == "std::int")
                                vars.layers[layer].ints["types.size"] = old_no_types;
                            else
                            {
                                vars.layers[layer].ints.erase("types.size");
                                if(oldTypTypes[types.size()].size() == 0)
                                    vars.layers[layer].typeOf.erase("types.size");
                                else
                                    vars.layers[layer].typeOf["types.size"] = oldTypTypes[types.size()];
                            }

                            for(size_t t=0; t<types.size(); t++)
                            {
                                if(!typWasConst[t])
                                    vars.layers[layer].constants.erase("types[" + std::to_string(t) + "]");
                                if(oldTypTypes[t] == "string")
                                    vars.layers[layer].strings["types[" + std::to_string(t) + "]"] = oldTyps[t];
                                else
                                {
                                    vars.layers[layer].strings.erase("types[" + std::to_string(t) + "]");
                                    if(oldTypTypes[t].size() == 0)
                                        vars.layers[layer].typeOf.erase("types[" + std::to_string(t) + "]");
                                    else
                                        vars.layers[layer].typeOf["types[" + std::to_string(t) + "]"] = oldTypTypes[t];
                                }
                            }


                            if(!optWasConst[options.size()])
                                vars.layers[layer].constants.erase("options.size");
                            if(oldOptTypes[options.size()] == "std::int")
                                vars.layers[layer].ints["options.size"] = old_no_options;
                            else
                            {
                                vars.layers[layer].ints.erase("options.size");
                                if(oldOptTypes[options.size()].size() == 0)
                                    vars.layers[layer].typeOf.erase("options.size");
                                else
                                    vars.layers[layer].typeOf["options.size"] = oldOptTypes[options.size()];
                            }

                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(!optWasConst[o])
                                    vars.layers[layer].constants.erase("options[" + std::to_string(o) + "]");
                                if(oldOptTypes[o] == "string")
                                    vars.layers[layer].strings["options[" + std::to_string(o) + "]"] = oldOpts[o];
                                else
                                {
                                    vars.layers[layer].strings.erase("options[" + std::to_string(o) + "]");
                                    if(oldOptTypes[o].size() == 0)
                                        vars.layers[layer].typeOf.erase("options[" + std::to_string(o) + "]");
                                    else
                                        vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"] = oldOptTypes[o];
                                }
                            }

                            if(!varNameWasConst)
                                vars.layers[layer].constants.erase("params.name");
                            if(oldVarNameType == "string")
                                vars.layers[layer].strings["params.name"] = oldVarName;
                            else
                            {
                                vars.layers[layer].strings.erase("params.name");
                                if(varNameToDel)
                                    vars.layers[layer].typeOf.erase("params.name");
                                else
                                    vars.layers[layer].typeOf["params.name"] = oldVarNameType;
                            }

                            if(!parWasConst[params.size()])
                                vars.layers[layer].constants.erase("params.size");
                            if(oldParTypes[params.size()] == "std::int")
                                vars.layers[layer].ints["params.size"] = old_no_params;
                            else
                            {
                                vars.layers[layer].ints.erase("params.size");
                                if(oldParTypes[params.size()].size() == 0)
                                    vars.layers[layer].typeOf.erase("params.size");
                                else
                                    vars.layers[layer].typeOf["params.size"] = oldParTypes[params.size()];
                            }

                            for(size_t p=0; p<params.size(); p++)
                            {
                                if(!parWasConst[p])
                                    vars.layers[layer].constants.erase("params[" + std::to_string(p) + "]");
                                if(oldParTypes[p] == "string")
                                    vars.layers[layer].strings["params[" + std::to_string(p) + "]"] = oldPars[p];
                                else
                                {
                                    vars.layers[layer].strings.erase("params[" + std::to_string(p) + "]");
                                    if(oldParTypes[p].size() == 0)
                                        vars.layers[layer].typeOf.erase("params[" + std::to_string(p) + "]");
                                    else
                                        vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"] = oldParTypes[p];
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << ":=: do not recognise the type " << quote(varType) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            //sets whether variables are constants/privates
            if(constants || privates)// || custScope)
            {
                for(size_t v=0; v<inputVars.size(); v++)
                {
                    if(constants)
                        vars.layers[layer].constants.insert(inputVars[v].first);
                    if(privates)
                    {
                        vars.layers[layer].privates.insert(inputVars[v].first);
                        //vars.layers[layer].scopeOf[inputVars[v].first] = inputVars[v].first.substr(0, inputVars[v].first.find_last_of('.')+1);
                    }
                    /*else if(custScope)
                    {
                        vars.layers[layer].privates.insert(inputVars[v].first);
                        //vars.layers[layer].scopeOf[inputVars[v].first] = scopeOf;
                    }*/
                }

                if(varType == "fstream" || varType == "ifstream" || varType == "ofstream")
                {
                    for(size_t v=0; v<inputVars.size(); v++)
                    {
                        if(constants)
                        {
                            vars.layers[layer].constants.insert(inputVars[v].first + ".close");
                            vars.layers[layer].constants.insert(inputVars[v].first + ".open");
                        }
                        else if(privates)
                        {
                            vars.layers[layer].privates.insert(inputVars[v].first + ".close");
                            vars.layers[layer].privates.insert(inputVars[v].first + ".open");
                            //vars.layers[layer].scopeOf[inputVars[v].first + ".close"] = inputVars[v].first.substr(0, inputVars[v].first.find_last_of('.')+1);
                        }
                        /*else if(custScope)
                        {
                            vars.layers[layer].privates.insert(inputVars[v].first + ".close");
                            //vars.layers[layer].scopeOf[inputVars[v].first + ".close"] = scopeOf;
                        }*/
                    }
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == 'g')
    {
        if(funcName == "getline")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "getline: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos spos, vpos;
            bool fromConsole = 1;

            if(params[0] != "console")
            {
                fromConsole = 0;
                vars.find(params[0], spos);

                if(spos.type != "fstream" && spos.type != "ifstream" && spos.type != "sstream" && spos.type != "isstream")
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "getline: first parameter " << params[0] << " should be console or an input stream variable" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            std::string txt;
            bool result = 1, addResult = 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "!r")
                        addResult = 0;
                }
            }

            if(params.size() == 1)
            {
                if(fromConsole)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    if(getline(std::cin, txt))
                        result = 1;
                    else
                        result = 0;
                    if(!consoleLocked)
                        os_mtx->unlock();
                }
                else
                    result = getline_from_stream(spos, txt);

                std::istringstream iss(txt);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + indentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            for(size_t p=1; p<params.size(); p++)
            {
                if(fromConsole)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    if(getline(std::cin, txt))
                        result = 1;
                    else
                        result = 0;
                    if(!consoleLocked)
                        os_mtx->unlock();
                }
                else
                    result = getline_from_stream(spos, txt);

                if(!result)
                {
                    if(addResult)
                    {
                        outStr += "0";
                        if(indent)
                            indentAmount += " ";
                    }
                    break;
                }


                if(params[p] == "ofile")
                {
                    addResult = 0;
                    std::istringstream iss(txt);
                    std::string str, oldLine;
                    int fileLineNo = 0;

                    while(!iss.eof())
                    {
                        getline(iss, str);
                        if(0 < fileLineNo++)
                            outStr += "\n" + indentAmount;
                        outStr += str;
                        oldLine = str;
                    }
                    if(indent)
                        indentAmount += into_whitespace(oldLine);
                }
                else if(vars.find(params[p], vpos) && vpos.type == "string")
                {
                    result = set_var_from_str(vpos, txt, readPath, lineNo, "getline()", eos);

                    if(!result)
                    {
                        if(addResult)
                        {
                            outStr += "0";
                            if(indent)
                                indentAmount += " ";
                        }
                        break;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "getline: parameter " << params[p] << " is not a defined string variable" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                result = 1;
            }

            if(result && addResult)
            {
                outStr += std::to_string(result);
                if(indent)
                    indentAmount += into_whitespace(std::to_string(result));
            }

            return 0;
        }
        else if(funcName == "getenv")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "getenv: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            params[0] = get_env(params[0].c_str());

            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
    }
    else if(funcName[0] == 'w')
    {
        if(funcName == "write")
        {
            if(params.size() < 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "write: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool readBlock = 0, parseBlock = 1;
            int bLineNo;
            std::string txt;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        readBlock = 1;
                    else if(options[o] == "!pb")
                        parseBlock = 0;
                }
            }

            if(readBlock)
            {
                if(read_block(txt, linePos, inStr, readPath, lineNo, bLineNo, "write{block}", eos))
                   return 1;

                if(parseBlock && parse_replace(lang, txt, "write block", readPath, antiDepsOfReadPath, lineNo, "write{block}", sLineNo, eos))
                    return 1;

                txt += "\r\n";
            }
            else if(params.size() == 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "write: expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;
            std::string gIndentAmount;
            for(size_t p=1; p<params.size(); p++)
            {
                if(params[p] == "endl")
                    txt += "\r\n";
                else if(vars.find(params[p], vpos))
                {
                    gIndentAmount = "";
                    if(!vars.add_str_from_var(vpos, txt, 1, indent, gIndentAmount))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "write: cannot get string of var type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                    txt += params[p];
            }

            if(params[0] == "ofile")
            {
                std::istringstream iss(txt);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + indentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            else if(params[0] == "console")
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lolcat)
                    rnbwcout(txt);
                else
                    eos << txt;
                if(!consoleLocked)
                    os_mtx->unlock();
            }
            else
            {
                VPos vpos;
                if(vars.find(params[0], vpos))
                {
                    if(vpos.type == "fstream")
                        vars.layers[vpos.layer].fstreams[params[0] ] << txt;
                    else if(vpos.type == "ofstream")
                    {
                        vars.layers[vpos.layer].ofstreams[params[0] ] << txt;
                    }
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "write is not defined for parameter " << quote(params[0]) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;

                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "write: no variable named " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "while")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool first = 1, addNewLines = 1, addIndent = 1, addScope = 1, addOut = addOutput;
            char wLang = lang;
            std::string block, parsedCondition, eob = "\n", wBaseIndentAmount = indentAmount;
            int bLineNo;

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, "while(" + params[0] + ")", eos))
                return 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "!o")
                        addOut = 0;
                    else if(options[o] == "!\n")
                        addNewLines = 0;
                    else if(options[o] == "\n")
                        eob = "\n\n";
                    else if(options[o].substr(0, 4) == "eob=")
                    {
                        eob = unquote(options[o].substr(4, options[o].size()-4));
                        if(eob[eob.size()-1] != '\n')
                            addIndent = 0;
                    }
                    else if(options[o] == "!s")
                        addScope = 0;
                    else if(options[o] == "n++")
                        wLang = 'n';
                    else if(options[o] == "f++")
                        wLang = 'f';
                }
            }

            bool boolCond = 0, exprtkCond = 0, varCond = 0, result;
            Expr condExpr;
            VPos vpos;

            if(get_bool(result, params[0]))
                boolCond = 1;
            else
            {
                condExpr.register_symbol_table(symbol_table);
                if(params[0] != "" && condExpr.set_expr(params[0]))
                    exprtkCond = 1;
                else if(replaceVars && vars.find(params[0], vpos))
                        varCond = 1;
            }

            while(1)
            {
                if(boolCond)
                {
                    if(!result)
                        break;
                }
                else if(exprtkCond)
                {
                    if(!condExpr.evaluate())
                        break;
                }
                else if(varCond)
                {
                    if(!vars.get_bool_from_var(vpos, result))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot get bool from variable ";
                        eos << quote(vpos.name) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(!result)
                        break;
                }
                else
                {
                    parsedCondition = params[0];

                    if(parse_replace('f', parsedCondition, "while condition", readPath, antiDepsOfReadPath, conditionLineNo, "while(" + params[0] + ")", sLineNo, eos))
                        return 1;

                    if(replaceVars && replace_var(parsedCondition, readPath, conditionLineNo, "while", eos))
                        return 1;

                    if(!get_bool(result, parsedCondition))
                    {
                        if(parsedCondition != "" && condExpr.set_expr(parsedCondition))
                            result = condExpr.evaluate();
                        else
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err(eos, readPath, conditionLineNo) << funcName << ": cannot convert " << quote(parsedCondition) << " to bool" << std::endl;
                            start_err(eos, readPath, sLineNo) << funcName << ": possible errors from exprtk:" << std::endl;
                            print_exprtk_parser_errs(eos, condExpr.parser, condExpr.expr_str, readPath, sLineNo);
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(!result)
                        break;
                }

                if(first)
                    first = 0;
                else
                {
                    if(addNewLines && addOut)
                    {
                        outStr += eob;
                        if(addIndent)
                        {
                            indentAmount = wBaseIndentAmount;
                            outStr += indentAmount;
                        }
                        else
                            indentAmount += into_whitespace(eob);
                    }
                }

                if(addScope)
                {
                    std::string cScope = vars.layers[vars.layers.size()-1].scope;
                    vars.add_layer(cScope);
                    //vars.add_layer(vars.layers[vars.layers.size()-1].scope); //this is broken on windows!
                }

                int ret_val = 1;
                if(wLang == 'n')
                    ret_val = n_read_and_process_fast(addOut, addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);
                else if(wLang == 'f')
                    ret_val = f_read_and_process_fast(addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);

                if(addScope)
                    vars.layers.pop_back();

                if(ret_val == NSM_BREAK)
                    break;
                else if(ret_val)
                    return ret_val;
            }

            if(!addOut)
            {
                if(linePos < inStr.size() && inStr[linePos] == '!')
                    ++linePos;
                else
                {
                    //skips to next non-whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                    {
                        if(inStr[linePos] == '@')
                            linePos = inStr.find("\n", linePos);
                        else
                        {
                            if(inStr[linePos] == '\n')
                                ++lineNo;
                            ++linePos;
                        }

                        if(linePos >= inStr.size())
                            break;
                    }
                }
            }

            return 0;
        }
        else if(funcName == "warning")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "warning: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(!consoleLocked)
                os_mtx->lock();
            start_warn(eos, readPath, lineNo) << params[0] << std::endl;
            if(!consoleLocked)
                os_mtx->unlock();

            return 0;
        }
    }
    else if(funcName[0] == 'r')
    {
        if(funcName == "read")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "read: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos spos, vpos;
            bool fromConsole = 1;

            if(params[0] != "console")
            {
                fromConsole = 0;
                vars.find(params[0], spos);

                if(spos.type != "fstream" && spos.type != "ifstream" && spos.type != "sstream" && spos.type != "isstream")
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "read: first parameter " << params[0] << " should be console or an input stream variable" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            std::string txt;
            bool result = 1, addResult = 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "!r")
                        addResult = 0;
                }
            }

            if(params.size() == 1)
            {
                if(fromConsole)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    std::cin >> txt;
                    if(!consoleLocked)
                        os_mtx->unlock();
                }
                else
                    result = read_str_from_stream(spos, txt);

                std::istringstream iss(txt);
                std::string str, oldLine;
                int fileLineNo = 0;

                while(!iss.eof())
                {
                    getline(iss, str);
                    if(0 < fileLineNo++)
                        outStr += "\n" + indentAmount;
                    outStr += str;
                    oldLine = str;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);
            }
            for(size_t p=1; p<params.size(); p++)
            {
                if(fromConsole)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    std::cin >> txt;
                    if(!consoleLocked)
                        os_mtx->unlock();
                }
                else
                    result = read_str_from_stream(spos, txt);

                if(!result)
                {
                    if(addResult)
                    {
                        outStr += "0";
                        if(indent)
                            indentAmount += " ";
                    }
                    break;
                }


                if(params[p] == "ofile")
                {
                    addResult = 0;
                    std::istringstream iss(txt);
                    std::string str, oldLine;
                    int fileLineNo = 0;

                    while(!iss.eof())
                    {
                        getline(iss, str);
                        if(0 < fileLineNo++)
                            outStr += "\n" + indentAmount;
                        outStr += str;
                        oldLine = str;
                    }
                    if(indent)
                        indentAmount += into_whitespace(oldLine);
                }
                else if(vars.find(params[p], vpos))
                {
                    result = set_var_from_str(vpos, txt, readPath, lineNo, "read", eos);

                    if(!result)
                    {
                        if(addResult)
                        {
                            outStr += "0";
                            if(indent)
                                indentAmount += " ";
                        }
                        break;
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "read: parameter " << quote(params[p]) << " is not a defined variable" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                result = 1;
            }

            if(result && addResult)
            {
                outStr += std::to_string(result);
                if(indent)
                    indentAmount += into_whitespace(std::to_string(result));
            }

            return 0;
        }
        else if(funcName == "rmv")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() < 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "rmv: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool changePermissions = 0, 
                 prompt = 0,
                 verbose = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "f")
                        changePermissions = 1;
                    else if(options[o] == "i")
                        prompt = 1;
                    else if(options[o] == "v")
                        verbose = 1;
                }
            }

            Path rmPath;
            std::string rmPathStr;
            char promptInput = 'n';
            
            for(size_t p=0; p<params.size(); ++p)
            {
                if(params[p].size() && (params[p][params[p].size()-1] == '/' || params[p][params[p].size()-1] == '\\'))
                {
                    int pos=params[p].size()-2; 
                    while(pos>=0 && (params[p][pos] == '/' || params[p][pos] == '\\'))
                        --pos;
                    params[p] = params[p].substr(0, pos+1);
                }

                rmPathStr = replace_home_dir(params[p]);
                rmPath.set_file_path_from(rmPathStr);

                if(!path_exists(rmPathStr))
                {
                    if(rmPathStr.find_first_of('*') != std::string::npos)
                    {
                        std::string parsedTxt, toParse = "@lst{!c, 1, P}(" + rmPathStr + ")";
                        int parseLineNo = 0;

                        if(n_read_and_process_fast(indent, toParse, parseLineNo, readPath, antiDepsOfReadPath, parsedTxt, eos))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": failed to list " << quote(rmPathStr) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::istringstream iss(parsedTxt);
                        while(getline(iss, rmPathStr))
                            params.push_back(rmPathStr);

                        continue;
                    }
                    else
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot remove ";
                        eos << quote(params[p]) << " as path does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else
                {
                    struct stat  oldDirPerms;
                    if(changePermissions)
                    {
                        chmod(rmPathStr.c_str(), 0666);
                        stat(rmPath.dir.c_str(), &oldDirPerms);
                        chmod(rmPath.dir.c_str(), S_IRWXU);
                    }

                    if((prompt && promptInput != 'a') || verbose)
                    {
                        std::cout << "rmv " << quote(params[p]);

                        if(prompt && promptInput != 'a')
                        {
                            std::cout << "? " << std::flush;
                            promptInput = nsm_getch();
                            if(promptInput == 'Y' || promptInput == '1')
                                promptInput = 'y';
                            else if(promptInput == 'A')
                                promptInput = 'a';
                        }
                        
                        std::cout << std::endl;
                    }

                    if(!prompt || promptInput == 'y' || promptInput == 'a')
                    {
                        if(dir_exists(rmPathStr))
                        {
                            if(!can_write(rmPath.str()) || delDir(rmPathStr, sLineNo, readPath, eos, consoleLocked, os_mtx))
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to remove directory ";
                                eos << quote(params[p]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                        else if(!can_write(rmPath.str()) || remove_file(rmPath))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to remove file ";
                            eos << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(changePermissions)
                        chmod(rmPath.dir.c_str(), oldDirPerms.st_mode);
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "return")
        {
            if(params.size() < 1)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "return: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(params.size() == 1)
            {
                outStr = params[0];
                if(indent)
                    indentAmount = into_whitespace(params[0]);
            }

            return NSM_RET;
        }
        else if(funcName == "replace_all")
        {
            if(params.size() != 3)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "replace_all: expected 3 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string str;

            //if(replaceVars && replace_var(params[0], readPath, sLineNo, "replace_all", eos))
              //  return 1;

            if(params[0] == "ofile")
                parsedText = findAndReplaceAll(parsedText, params[1], params[2]);
            else
            {
                params[0] = findAndReplaceAll(params[0], params[1], params[2]);

                outStr += params[0];
                if(indent)
                    indentAmount += into_whitespace(params[0]);
            }

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
            else if(funcName == "rm")
                funcName = "del";
        #endif
        else if(funcName == "refresh_completions")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "refresh_completions: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return refresh_completions();
        }
    }
    else if(funcName[0] == 'b')
    {
        if(funcName == "break")
        {
            if(params.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "break: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return NSM_BREAK;
        }
        else if(funcName.substr(0, 5) == "blank")
        {
            return 0;
        }
    }
    else if(funcName[0] == 'f')
    {
        if(funcName == "f++")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "f++: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int bLineNo;
            bool addOut = addOutput;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                }
            }

            params.push_back("");
            if(read_block(params[0], linePos, inStr, readPath, lineNo, bLineNo, "f++{block}", eos))
                return 1;

            std::string resultStr;
            int result = f_read_and_process_fast(addOut, params[0], bLineNo-1, readPath, antiDepsOfReadPath, resultStr, eos);

            if(result)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed here" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(addOut)
            {
                std::string resultStr = std::to_string(result);
                outStr += resultStr;
                if(indent)
                    indentAmount += into_whitespace(resultStr);
            }

            return result;
        }
        else if(funcName == "for")
        {
            if(params.size() != 3 && params.size() != 4)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "for: expected 3-4 parameters separated by ';', got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool first = 1, addNewLines = 1, addIndent = 1, addScope = 1, addOut = addOutput;
            char fLang = lang;
            std::string block, parsedCondition, eob = "\n", wBaseIndentAmount = indentAmount;
            int bLineNo;

            std::string forStr;

            if(params.size() == 3)
                forStr = "for(" + params[0] + "; " + params[1] + "; " + params[2] + ")";
            else
                forStr = "for(" + params[0] + "; " + params[1] + "; " + params[2] + "; " + params[3] + ")";

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, forStr, eos))
                return 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "!o")
                        addOut = 0;
                    else if(options[o] == "!\n")
                        addNewLines = 0;
                    else if(options[o] == "\n")
                        eob = "\n\n";
                    else if(options[o].substr(0, 4) == "eob=")
                    {
                        eob = unquote(options[o].substr(4, options[o].size()-4));
                        if(eob[eob.size()-1] != '\n')
                            addIndent = 0;
                    }
                    else if(options[o] == "!s")
                        addScope = 0;
                    else if(options[o] == "n++")
                        fLang = 'n';
                    else if(options[o] == "f++")
                        fLang = 'f';
                }
            }

            //adds layer of scope
            if(addScope)
            {
                std::string cScope = vars.layers[vars.layers.size()-1].scope;
                vars.add_layer(cScope);
                //vars.add_layer(vars.layers[vars.layers.size()-1].scope); //this is broken on windows!
            }

            bool boolCond = 0, exprtkCond = 0, varCond = 0, result;
            bool exprtkInc = 0;
            Expr preExpr, condExpr, incExpr;
            VPos vpos;

            //runs pre-for-loop code
            if(params[0] != "")
            {
                //check if it compiles as exprtk script
                preExpr.register_symbol_table(symbol_table);
                if(preExpr.set_expr(params[0]))
                    preExpr.evaluate();
                else
                {
                    //parsedCondition = params[0];

                    //if(parse_replace('f', parsedCondition, "pre-for-loop code", readPath, antiDepsOfReadPath, conditionLineNo, forStr, sLineNo, eos))
                      //  return 1;

                    if(f_read_and_process_fast(0, params[0], conditionLineNo-1, readPath, antiDepsOfReadPath, parsedCondition, eos))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": failed to parse pre-loop code" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
            }

            if(params[1] == "")
                params[1] = "1";

            //determines condition type
            if(get_bool(result, params[1]))
                boolCond = 1;
            else
            {
                condExpr.register_symbol_table(symbol_table);
                if(params[1] != "" && condExpr.set_expr(params[1]))
                    exprtkCond = 1;
                else if(replaceVars && vars.find(params[1], vpos))
                    varCond = 1;
            }

            //determines post-for-block/increment code type
            bool hasIncrements = 0;

            for(size_t i=1; i<params[2].size(); ++i)
            {
                if(params[2][i] == '+' && params[2][i-1] == '+')
                {
                    hasIncrements = 1;
                    break;
                }
                else if(params[2][i] == '-' && params[2][i-1] == '-')
                {
                    hasIncrements = 1;
                    break;
                }
            }
            if(!hasIncrements)
            {
                incExpr.register_symbol_table(symbol_table); 
                if(params[2] != "" && incExpr.set_expr(params[2]))
                    exprtkInc = 1;
            }

            while(1)
            {
                if(boolCond)
                {
                    if(!result)
                        break;
                }
                else if(exprtkCond)
                {
                    if(!condExpr.evaluate())
                        break;
                }
                else if(varCond)
                {
                    if(!vars.get_bool_from_var(vpos, result))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot get bool from variable ";
                        eos << quote(vpos.name) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(!result)
                        break;
                }
                else
                {
                    parsedCondition = params[1];

                    if(parse_replace('f', parsedCondition, "for condition", readPath, antiDepsOfReadPath, conditionLineNo, forStr, sLineNo, eos))
                        return 1;

                    if(replaceVars && replace_var(parsedCondition, readPath, conditionLineNo, "for", eos))
                        return 1;

                    if(!get_bool(result, parsedCondition))//, readPath, conditionLineNo, "for", eos))
                    {
                        if(parsedCondition != "" && condExpr.set_expr(parsedCondition))
                            result = condExpr.evaluate();
                        else
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err(eos, readPath, conditionLineNo) << funcName << ": cannot convert " << quote(parsedCondition) << " to bool" << std::endl;
                            start_err(eos, readPath, sLineNo) << funcName << ": possible errors from ExprTk:" << std::endl;
                            print_exprtk_parser_errs(eos, condExpr.parser, condExpr.expr_str, readPath, sLineNo);
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(!result)
                        break;
                }

                if(first)
                    first = 0;
                else
                {
                    if(addNewLines && addOut)
                    {
                        outStr += eob;
                        if(addIndent)
                        {
                            indentAmount = wBaseIndentAmount;
                            outStr += indentAmount;
                        }
                        else
                            indentAmount += into_whitespace(eob);
                    }
                }

                if(addScope)
                {
                    std::string cScope = vars.layers[vars.layers.size()-1].scope;
                    vars.add_layer(cScope);
                    //vars.add_layer(vars.layers[vars.layers.size()-1].scope); //this is broken on windows!
                }

                int ret_val = 1;
                if(fLang == 'n')
                    ret_val = n_read_and_process_fast(addOut, addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);
                else if(fLang == 'f')
                    ret_val = f_read_and_process_fast(addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);

                if(addScope)
                    vars.layers.pop_back();

                if(ret_val == NSM_BREAK)
                    break;
                else if(ret_val)
                    return ret_val;

                //runs post-for-block code
                if(exprtkInc)
                    incExpr.evaluate();
                else
                {
                    parsedCondition = params[2];

                    if(parse_replace(0, 'f', parsedCondition, "post-for-block code", readPath, antiDepsOfReadPath, conditionLineNo, forStr, sLineNo, eos))
                        return 1;
                }
            }

            if(params.size() == 4)
            {
                //runs post-for-loop code
                //could check here whether post-for-loop code compiles as an exprtk script
                parsedCondition = params[3];

                if(parse_replace('f', parsedCondition, "post-for-loop code", readPath, antiDepsOfReadPath, conditionLineNo, forStr, sLineNo, eos))
                    return 1;
            }


            if(addScope)
                vars.layers.pop_back();

            if(!addOut)
            {
                if(linePos < inStr.size() && inStr[linePos] == '!')
                    ++linePos;
                else
                {
                    //skips to next non-whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                    {
                        if(inStr[linePos] == '@')
                            linePos = inStr.find("\n", linePos);
                        else
                        {
                            if(inStr[linePos] == '\n')
                                ++lineNo;
                            ++linePos;
                        }

                        if(linePos >= inStr.size())
                            break;
                    }
                }
            }

            return 0;
        }
        else if(funcName == "function")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "function: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string block;
            int bLineNo;
            char fnLang = lang;
            std::string fnType = "function";
            bool parseBlock = 0;
            size_t layer = vars.layers.size()-1;
            bool addOut, isConst = 1, isPrivate = 0, unscopedFn = 0;
            std::unordered_set<std::string> inScopes;

            if(lang == 'n')
                addOut = 1;
            else
                addOut = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "lambda")
                        fnType = "lambda";
                    else if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                    else if(options[o] == "f++")
                        fnLang = 'f';
                    else if(options[o] == "n++")
                        fnLang = 'n';
                    else if(options[o] == "!const")
                        isConst = 0;
                    else if(options[o] == "private")
                        isPrivate = 1;
                    else if(options[o].substr(0, 7) == "scope+=")
                        inScopes.insert(unquote(options[o].substr(7, options[o].size()-7)));
                    else if(options[o] == "!s")
                        unscopedFn = 1;
                    else if(options[o] == "s")
                        unscopedFn = 0;
                    else if(options[o] == "pb")
                        parseBlock = 1;
                    else if(options[o].substr(0, 6) == "layer=")
                    {
                        std::string str = unquote(options[o].substr(6, options[o].size()-6));

                        if(!isNonNegInt(str))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "function: specified layer " << quote(str) << " is not a non-negative integer" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        layer = std::atoi(str.c_str());
                        if(layer >= vars.layers.size())
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "function: specified layer " << quote(str) << " should be less than the number of layers" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                }
            }

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, fnType + "(" + params[0] + ")", eos))
                return 1;

            if(parseBlock && parse_replace(lang, block, fnType + " block", readPath, antiDepsOfReadPath, bLineNo, fnType + "()", sLineNo, eos))
                return 1;

            if(add_fn(params[0], fnLang, block, fnType, layer, isConst, isPrivate, unscopedFn, addOut, inScopes, readPath, bLineNo, std::string(1, fnLang) + "()", eos))
                return 1;

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "forget")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "forget: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool rmFromExprTk = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "exprtk")
                        rmFromExprTk = 1;
                }
            }

            VPos vpos;
            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.find(params[p], vpos))
                {
                    if(vars.layers[vpos.layer].privates.count(params[p]))
                    {
                        if(!vars.layers[vpos.layer].inScopes[params[p]].count(vars.layers[vars.layers.size()-1].scope))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "forget: attempted illegal removal of private variable " << quote(params[p]) << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    
                    if(vpos.type == "bool" || vpos.type == "int" || vpos.type == "double")
                    {
                        vars.layers[vpos.layer].doubles.erase(params[p]);
                        if(rmFromExprTk && symbol_table.symbol_exists(params[p]))
                            symbol_table.remove_variable(params[p]);
                    }
                    else if(vpos.type == "char" || vpos.type == "string")
                    {
                        vars.layers[vpos.layer].strings.erase(params[p]);
                        if(rmFromExprTk && symbol_table.symbol_exists(params[p]))
                            symbol_table.remove_stringvar(params[p]);
                    }
                    else if(vpos.type.substr(0, 5) == "std::")
                    {
                        if(vpos.type == "std::bool")
                            vars.layers[vpos.layer].bools.erase(params[p]);
                        if(vpos.type == "std::int")
                            vars.layers[vpos.layer].ints.erase(params[p]);
                        if(vpos.type == "std::double")
                            vars.layers[vpos.layer].doubles.erase(params[p]);
                        else if(vpos.type == "std::char")
                            vars.layers[vpos.layer].chars.erase(params[p]);
                        else if(vpos.type == "std::string")
                            vars.layers[vpos.layer].strings.erase(params[p]);
                        else if(vpos.type == "std::llint")
                            vars.layers[vpos.layer].llints.erase(params[p]);
                        else if(vpos.type == "std::vector<double>")
                        {
                            vars.layers[vpos.layer].doubVecs.erase(params[p]);
                            if(rmFromExprTk && symbol_table.symbol_exists(params[p]))
                                symbol_table.remove_vector(params[p]);
                        }
                    }
                    else if(vpos.type == "fstream")
                        vars.layers[vpos.layer].fstreams.erase(params[p]);
                    else if(vpos.type == "ifstream")
                        vars.layers[vpos.layer].ifstreams.erase(params[p]);
                    else if(vpos.type == "ofstream")
                        vars.layers[vpos.layer].ofstreams.erase(params[p]);
                    else if(vpos.type == "function")
                    {
                        vars.layers[vpos.layer].functions.erase(params[p]);
                        vars.layers[vpos.layer].paths.erase(params[p]);
                        vars.layers[vpos.layer].ints.erase(params[p]);
                    }

                    vars.layers[vpos.layer].typeOf.erase(params[p]);
                    vars.layers[vpos.layer].inScopes.erase(params[p]);
                    vars.layers[vpos.layer].scopeOf.erase(params[p]);
                    vars.layers[vpos.layer].constants.erase(params[p]);
                    vars.layers[vpos.layer].privates.erase(params[p]);
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "forget: no variable or function named " << params[p] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "faviconinclude")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "faviconinclude: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string faviconPathStr = params[0];
            Path faviconPath;
            faviconPath.set_file_path_from(faviconPathStr);

            //warns user if favicon file doesn't exist
            if(!file_exists(faviconPathStr.c_str()))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "faviconinclude: favicon file " << faviconPath << " does not exist" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            Path pathToFavicon(pathBetween(toBuild.outputPath.dir, faviconPath.dir), faviconPath.file);

            std::string faviconInclude = "<link rel=\"icon\" type=\"image/png\" href=\"" + pathToFavicon.str() + "\"";
            if(options.size())
                for(size_t o=0; o<options.size(); ++o)
                    faviconInclude += " " + options[o];
            faviconInclude += ">";

            outStr += faviconInclude;
            if(indent)
                indentAmount += into_whitespace(faviconInclude);

            return 0;
        }
    }
    else if(funcName[0] == 't')
    {
        if(funcName == "typeof")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "typeof: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                outStr += vars.layers[vpos.layer].typeOf[params[0]];
                if(indent)
                    indentAmount += into_whitespace(vars.layers[vpos.layer].typeOf[params[0]]);
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "typeof: " << params[0] << " is not defined" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
        #else
            else if(funcName == "type")
                funcName = "cat";
        #endif
    }
    else if(funcName[0] == 'e')
    {
        if(funcName == "exprtk")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "exprtk: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int bLineNo;
            bool blockOpt = 0, parseBlock = 0, addOut = addOutput, round = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "pb")
                        parseBlock = 1;
                    else if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                    else if(options[o] == "round")
                        round = 1;
                }
            }

            if(params.size() == 0)
            {
                blockOpt = 1;
                params.push_back("");
                if(read_block(params[0], linePos, inStr, readPath, lineNo, bLineNo, "exprtk{block}", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, params[0], "exprtk block", readPath, antiDepsOfReadPath, bLineNo, "exprtk{block}", sLineNo, eos))
                    return 1;
            }

            if(params.size() == 0)
                params.push_back("");

            std::string value;
            if(parseParams && !doNotParse)
            {
                if(!expr.set_expr(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "exprtk" << c_white << ": failed to compile expression" << std::endl;
                    if(blockOpt)
                        print_exprtk_parser_errs(eos, expr.parser, params[0], readPath, bLineNo);
                    else
                        print_exprtk_parser_errs(eos, expr.parser, params[0], readPath, sLineNo);
                    os_mtx->unlock();
                    return 1;
                }
                value = vars.double_to_string(expr.evaluate(), round);
            }
            else
            {
                if(!exprset.add_expr(params[0]))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, sLineNo) << c_green << "exprtk" << c_white << ": failed to compile expression" << std::endl;
                    if(blockOpt)
                        print_exprtk_parser_errs(eos, exprset.parser, params[0], readPath, bLineNo);
                    else
                        print_exprtk_parser_errs(eos, exprset.parser, params[0], readPath, sLineNo);
                    os_mtx->unlock();
                    return 1;
                }
                value = vars.double_to_string(exprset.evaluate(params[0]), round);
            }

            if(addOut)
            {
                outStr += value;
                if(indent)
                    indentAmount += into_whitespace(value);
            }

            return 0;
        }
        else if(funcName == "exprtk.add_package")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "exprtk.add_package: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            for(size_t p=0; p<params.size(); ++p)
            {
                if(params[p] == "basicio_package")
                    symbol_table.add_package(basicio_package);
                else if(params[p] == "fileio_package")
                    symbol_table.add_package(fileio_package);
                else if(params[p] == "vectorops_package")
                    symbol_table.add_package(vectorops_package);
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "exprtk.add_package: do not recognise package " << quote(params[p]) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
        else if(funcName == "ent")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "ent: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string ent = params[0];

            if(ent.size() == 1)
            {
                /*
                    see http://dev.w3.org/html5/html-author/charref for
                    more character references than you could ever need!
                */
                switch(ent[0])
                {
                    case '`':
                        outStr += "&grave;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '~':
                        outStr += "&tilde;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '!':
                        outStr += "&excl;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '@':
                        outStr += "&commat;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '#':
                        outStr += "&num;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case '$': //MUST HAVE MATHJAX HANDLE THIS WHEN \$
                        outStr += "&dollar;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '%':
                        outStr += "&percnt;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '^':
                        outStr += "&Hat;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case '&':
                        outStr += "&amp;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case '*':
                        outStr += "&ast;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case '?':
                        outStr += "&quest;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '<':
                        outStr += "&lt;";
                        if(indent)
                            indentAmount += std::string(4, ' ');
                        break;
                    case '>':
                        outStr += "&gt;";
                        if(indent)
                            indentAmount += std::string(4, ' ');
                        break;
                    case '(':
                        outStr += "&lpar;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case ')':
                        outStr += "&rpar;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '[':
                        outStr += "&lbrack;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case ']':
                        outStr += "&rbrack;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '{':
                        outStr += "&lbrace;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '}':
                        outStr += "&rbrace;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '-':
                        outStr += "&minus;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '_':
                        outStr += "&lowbar;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '=':
                        outStr += "&equals;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '+':
                        outStr += "&plus;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '|':
                        outStr += "&vert;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '\\':
                        outStr += "&bsol;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '/':
                        outStr += "&sol;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case ';':
                        outStr += "&semi;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case ':':
                        outStr += "&colon;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '\'':
                        outStr += "&apos;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '"':
                        outStr += "&quot;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case ',':
                        outStr += "&comma;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '.':
                        outStr += "&period;";
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    default:
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "ent: do not currently have an entity value for " << quote(ent) << std::endl;
                        os_mtx->unlock();
                        return 1;
                }
            }
            else if(ent == "£")
            {
                outStr += "&pound;";
                if(indent)
                    indentAmount += std::string(7, ' ');
            }
            else if(ent == "¥")
            {
                outStr += "&yen;";
                if(indent)
                    indentAmount += std::string(5, ' ');
            }
            else if(ent == "€")
            {
                outStr += "&euro;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "§" || ent == "section")
            {
                outStr += "&sect;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "+-")
            {
                outStr += "&pm;";
                if(indent)
                    indentAmount += std::string(4, ' ');
            }
            else if(ent == "-+")
            {
                outStr += "&mp;";
                if(indent)
                    indentAmount += std::string(4, ' ');
            }
            else if(ent == "!=")
            {
                outStr += "&ne;";
                if(indent)
                    indentAmount += std::string(4, ' ');
            }
            else if(ent == "<=")
            {
                outStr += "&leq;";
                if(indent)
                    indentAmount += std::string(5, ' ');
            }
            else if(ent == ">=")
            {
                outStr += "&geq;";
                if(indent)
                    indentAmount += std::string(5, ' ');
            }
            else if(ent == "->")
            {
                outStr += "&rarr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "<-")
            {
                outStr += "&larr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "<->")
            {
                outStr += "&harr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "==>")
            {
                outStr += "&rArr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "<==")
            {
                outStr += "&lArr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "<==>")
            {
                outStr += "&hArr;";
                if(indent)
                    indentAmount += std::string(6, ' ');
            }
            else if(ent == "<=!=>")
            {
                outStr += "&nhArr;";
                if(indent)
                    indentAmount += std::string(7, ' ');
            }
            else if(ent == "...")
            {
                outStr += "&hellip;";
                if(indent)
                    indentAmount += std::string(8, ' ');
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "ent: do not currently have an entity value for " << quote(ent) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "error")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "error: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << params[0] << std::endl;
            os_mtx->unlock();

            return 1;
        }
        else if(funcName == "exit")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "exit: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return NSM_EXIT; //check this??
        }
    }
    else if(funcName[0] == 'd')
    {
        if(funcName == "dep")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "dep: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string depPathStr;
            Path depPath;

            for(size_t p=0; p<params.size(); ++p)
            {
                depPathStr = params[p];
                depPath.set_file_path_from(depPathStr);
                depFiles.insert(depPath);

                if(depPath == toBuild.contentPath)
                    contentAdded = 1;

                if(!path_exists(depPathStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "dep(" << quote(depPathStr) << ") failed as dependency does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        else if(funcName == "do-while")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "do-while: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool first = 1, addNewLines = 1, addIndent = 1, addScope = 1, addOut = addOutput;
            char dwLang = lang;
            std::string block, parsedCondition, eob = "\n", wBaseIndentAmount = indentAmount;
            int bLineNo;

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, "do-while(" + params[0] + ")", eos))
                return 1;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "!o")
                        addOut = 0;
                    else if(options[o] == "!\n")
                        addNewLines = 0;
                    else if(options[o] == "\n")
                        eob = "\n\n";
                    else if(options[o].substr(0, 4) == "eob=")
                    {
                        eob = unquote(options[o].substr(4, options[o].size()-4));
                        if(eob[eob.size()-1] != '\n')
                            addIndent = 0;
                    }
                    else if(options[o] == "!s")
                        addScope = 0;
                    else if(options[o] == "n++")
                        dwLang = 'n';
                    else if(options[o] == "f++")
                        dwLang = 'f';
                }
            }

            bool boolCond = 0, exprtkCond = 0, varCond = 0, result;
            Expr condExpr;
            VPos vpos;

            if(get_bool(result, params[0]))
                boolCond = 1;
            else
            {
                condExpr.register_symbol_table(symbol_table);
                if(params[0] != "" && condExpr.set_expr(params[0]))
                    exprtkCond = 1;
                else if(replaceVars && vars.find(params[0], vpos))
                        varCond = 1;
            }

            while(1)
            {
                if(first)
                    first = 0;
                else
                {
                    if(addNewLines && addOut)
                    {
                        outStr += eob;
                        if(addIndent)
                        {
                            indentAmount = wBaseIndentAmount;
                            outStr += indentAmount;
                        }
                        else
                            indentAmount += into_whitespace(eob);
                    }
                }

                if(addScope)
                {
                    std::string cScope = vars.layers[vars.layers.size()-1].scope;
                    vars.add_layer(cScope);
                    //vars.add_layer(vars.layers[vars.layers.size()-1].scope); //this is broken on windows!
                }

                int ret_val = 1;
                if(dwLang == 'n')
                    ret_val = n_read_and_process_fast(addOut, addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);
                else if(dwLang == 'f')
                    ret_val = f_read_and_process_fast(addOut, block, bLineNo-1, readPath, antiDepsOfReadPath, outStr, eos);

                if(addScope)
                    vars.layers.pop_back();

                if(ret_val == NSM_BREAK)
                    break;
                else if(ret_val)
                    return ret_val;

                if(boolCond)
                {
                    if(!result)
                        break;
                }
                else if(exprtkCond)
                {
                    if(!condExpr.evaluate())
                        break;
                }
                else if(varCond)
                {
                    if(!vars.get_bool_from_var(vpos, result))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot get bool from variable ";
                        eos << quote(vpos.name) << " of type " << quote(vpos.type) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(!result)
                        break;
                }
                else
                {
                    parsedCondition = params[0];

                    if(parse_replace('f', parsedCondition, "do-while condition", readPath, antiDepsOfReadPath, conditionLineNo, "do-while(" + params[0] + ")", sLineNo, eos))
                        return 1;

                    if(replaceVars && replace_var(parsedCondition, readPath, conditionLineNo, "do-while", eos))
                        return 1;

                    if(!get_bool(result, parsedCondition))
                    {
                        if(parsedCondition != "" && condExpr.set_expr(parsedCondition))
                            result = condExpr.evaluate();
                        else
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err(eos, readPath, conditionLineNo) << funcName << ": cannot convert " << quote(parsedCondition) << " to bool" << std::endl;
                            start_err(eos, readPath, sLineNo) << funcName << ": possible errors from exprtk:" << std::endl;
                            print_exprtk_parser_errs(eos, condExpr.parser, condExpr.expr_str, readPath, sLineNo);
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    if(!result)
                        break;
                }
            }

            if(!addOut)
            {
                if(linePos < inStr.size() && inStr[linePos] == '!')
                    ++linePos;
                else
                {
                    //skips to next non-whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                    {
                        if(inStr[linePos] == '@')
                            linePos = inStr.find("\n", linePos);
                        else
                        {
                            if(inStr[linePos] == '\n')
                                ++lineNo;
                            ++linePos;
                        }

                        if(linePos >= inStr.size())
                            break;
                    }
                }
            }

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
        #else
            else if(funcName == "dir")
                funcName = "ls";
            else if(funcName == "del")
                funcName = "rm";
        #endif
    }
    else if(funcName[0] == 's')
    {
        if(funcName == "size")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "size: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_var(params[0], readPath, sLineNo, "size", eos))
                return 1;

            params[0] = std::to_string(params[0].size());
            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "substr")
        {
            if(params.size() != 3)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "substr: expected 3 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(replaceVars && replace_var(params[0], readPath, sLineNo, "substr", eos))
                return 1;

            size_t pos = std::atoi(params[1].c_str()),
                length = std::atoi(params[2].c_str());
            std::string str, oldLine;

            if(!isNonNegInt(params[1]) || pos >= params[0].size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "substr: second parameter " << params[1] << " should be a non-negative integer less than the size of the first parameter" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(!isNonNegInt(params[2]))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "substr: third parameter " << params[2] << " should be a non-negative integer" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            str += params[0].substr(pos, length);

            std::istringstream iss(str);
            int fileLineNo = 0;

            while(!iss.eof())
            {
                getline(iss, str);
                if(0 < fileLineNo++)
                    outStr += "\n" + indentAmount;
                outStr += str;
                oldLine = str;
            }
            if(indent)
                indentAmount += into_whitespace(oldLine);

            return 0;
        }
        else if(funcName == "struct")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "struct: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string block;
            char structLang = lang;
            bool parseBlock = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "f++")
                        structLang = 'f';
                    else if(options[o] == "n++")
                        structLang = 'n';
                    else if(options[o] == "pb")
                        parseBlock = 1;
                }
            }

            //checks whether name exists at current scope
            VPos vpos;
            if(vars.find(params[0], vpos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "struct: cannot have struct and variable/function named " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(vars.typeDefs.count(params[0]))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "struct: redeclaration of struct name " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else
            {
                if(read_block(block, linePos, inStr, readPath, lineNo, vars.typeDefLineNo[params[0]], "struct(" + params[0] + ")", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, block, "struct block", readPath, antiDepsOfReadPath, vars.typeDefLineNo[params[0]], "struct()", sLineNo, eos))
                    return 1;

                vars.typeDefs[params[0]] = block;
                if(structLang == 'n')
                    vars.nTypes.insert(params[0]);
                vars.typeDefPath[params[0]] = readPath;
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        if(funcName == "std::vector.push_back")
        {
            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "std::vector.push_back: expected 2+ parameters (name, value, ..., value), got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                if(vpos.type == "std::vector<double>")
                {
                    for(size_t v=1; v<params.size(); v++)
                    {
                        if(!isDouble(params[v]))
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << "std::vector.push_back: value " << quote(params[v]) << " given to push back is not a double" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        vars.layers[vpos.layer].doubVecs[params[0]].push_back(std::strtod(params[v].c_str(), NULL));
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "std::vector.push_back: do not recognise the type " << quote(vpos.type) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "std::vector.push_back: no variables named " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "script")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "script: no path provided" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool ifExists = 0,
                 attachContentPath = 0,
                 makeBackup = 1;
            bool blockOpt = 0, parseBlock = 0;
            int bLineNo;
            bool noReturnValue = 0;
            int console = 0, inject = 0, injectRaw = 0, noOutput = 0;
            int c = sys_counter++;
            std::string output_filename = ".@scriptoutput" + std::to_string(c);

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "b" || options[o] == "block")
                        blockOpt = 1;
                    else if(options[o] == "pb")
                        parseBlock = 1;
                    if(options[o] == "!bs")
                        makeBackup = 0;
                    else if(options[o] == "if-exists")
                        ifExists = 1;
                    else if(options[o] == "console")
                        console = 1;
                    else if(options[o] == "inject")
                        inject = 1;
                    else if(options[o] == "raw")
                        injectRaw = 1;
                    else if(options[o] == "!o")
                        noOutput = 1;
                    else if(options[o] == "!ret")
                        noReturnValue = 1;
                    else if(options[o] == "content")
                        attachContentPath = 1;
                }
            }

            if(blockOpt)
            {
                std::string block;
                if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, "script{block}", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, block, "script block", readPath, antiDepsOfReadPath, bLineNo, "script{block}", sLineNo, eos))
                    return 1;

                Path scriptPath;
                scriptPath.set_file_path_from(params[0]);
                scriptPath.ensureDirExists();
                std::ofstream ofs(params[0]);
                ofs << block << std::endl;
                ofs.close();
                if(chmod(params[0].c_str(), 0777))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "script: failed to set executable permissions for " << scriptPath << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(params.size() > 1)
            {
                params[1] = quote(params[1]);
                for(size_t i=2; i<params.size(); i++)
                    params[1] += " " + quote(params[i]);
            }
            else if(params.size() == 1)
                params.push_back("");

            size_t pos = params[0].substr(1, params[0].size()-1).find_first_of('.');
            std::string cScriptExt = "";
            if(pos != std::string::npos)
                cScriptExt = params[0].substr(pos+1, params[0].size()-pos-1);
            std::string execPath = "./.script" + std::to_string(c) + cScriptExt;
            std::string exec_str;

            if(console + inject + injectRaw + noOutput > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "script(" << quote(execPath) << "): console and raw options are incompatible with each other" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(!(console + inject + injectRaw + noOutput))
            {
                if(lang == 'n')
                    inject = 1;
                else
                    console = 1;
            }


            if(attachContentPath)
            {
                contentAdded = 1;
                params[1] = quote(toBuild.contentPath.str()) + " " + params[1];
            }

            #if defined _WIN32 || defined _WIN64
                if(unquote(execPath).substr(0, 2) == "./")
                    execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
            #else  //*nix
            #endif

            Path scriptPath;
            scriptPath.set_file_path_from(params[0]);
            if(scriptPath == toBuild.contentPath)
                contentAdded = 1;
            depFiles.insert(scriptPath);

            if(file_exists(params[0]))
            {
                //copies script to backup location
                if(makeBackup)
                {
                    if(cpFile(params[0], params[0] + ".backup", sLineNo, readPath, eos, consoleLocked, os_mtx))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script: failed to copy " << quote(params[0]) << " to " << quote(params[0] + ".backup") << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                //moves script to main directory
                //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
                //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                int mcount = 0;
                while(rename(params[0].c_str(), execPath.c_str()))
                {
                    if(++mcount == 100)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_warn(eos, readPath, lineNo) << "script: have tried to move " << quote(params[0]) << " to " << quote(execPath) << " 100 times already, may need to abort" << std::endl;
                        start_warn(eos) << "you may need to move " << quote(execPath) << " back to " << quote(params[0]) << std::endl;
                        if(!consoleLocked)
                            os_mtx->unlock();
                    }
                }

                int result;
                if(file_exists("/.flatpak-info"))
                    exec_str = "flatpak-spawn --host bash -c " + quote(execPath + " " + params[1]);
                else
                    exec_str = execPath + " " + params[1];

                if(console)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                }
                else
                    exec_str += " > " + output_filename;

                result = system(exec_str.c_str());

                if(console && !consoleLocked)
                    os_mtx->unlock();

                //moves script back to original location
                //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                mcount = 0;
                while(rename(execPath.c_str(), params[0].c_str()))
                {
                    if(++mcount == 100)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_warn(eos, readPath, lineNo) << "script: have tried to move " << execPath << " to " << params[0] << " 100 times already, may need to abort" << std::endl;
                        start_warn(eos) << "you may need to move " << quote(execPath) << " back to " << quote(params[0]) << std::endl;
                        if(!consoleLocked)
                            os_mtx->unlock();
                    }
                }

                //deletes backup copy
                if(makeBackup)
                    remove_file(Path("", params[0] + ".backup"));

                if(console)
                {
                    if(result)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script{console}(" << quote(params[0]) << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(mode != MODE_INTERP && !noReturnValue) //check this
                    {
                        std::string resultStr = std::to_string(result);
                        outStr += resultStr;
                        if(indent)
                            outStr += into_whitespace(resultStr);
                    }
                }
                else if(noOutput)
                {
                    if(result)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script{!o}(" << quote(params[0]) << ") failed, ";
                        eos << "see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    if(!noReturnValue)
                    {
                        std::string resultStr = std::to_string(result);
                        outStr += resultStr;
                        if(indent)
                            outStr += into_whitespace(resultStr);
                    }

                    remove_file(Path("./", output_filename));
                }
                else if(injectRaw)
                {
                    if(result)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script{raw}(" << quote(params[0]) << ") failed, ";
                        eos << "see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);
                    std::string fileLine, oldLine;
                    int fileLineNo = 0;

                    while(!ifs.eof())
                    {
                        getline(ifs, fileLine);
                        if(0 < fileLineNo++)
                            outStr += "\n" + indentAmount;
                        oldLine = fileLine;
                        outStr += fileLine;
                    }
                    if(indent)
                        indentAmount += into_whitespace(oldLine);

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else //inject
                {
                    if(result)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script{inject}(" << quote(params[0]) << ") failed, ";
                        eos << "see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::string fileStr = string_from_file(output_filename);

                    //indent amount updated inside read_and_process
                    if(n_read_and_process(1, fileStr, 0, Path("", scriptPath.str() + " output - " + output_filename), antiDepsOfReadPath, outStr, eos) > 0)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "script: failed to process output of script " << quote(params[0] + " " + params[1]) << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    remove_file(Path("./", output_filename));
                }
            }
            else if(ifExists)
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "script(" << quote(params[0]) << ") failed as script " << quote(params[0]) << " does not exist" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "system" || funcName == "sys")
        {
            if(params.size() > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "system: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int console = 0, inject = 0, injectRaw = 0, noOutput = 0;
            int bLineNo;
            bool parseBlock = 0;
            bool attachContentPath = 0;
            bool noReturnValue = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "pb")
                        parseBlock = 1;
                    else if(options[o] == "console")
                        console = 1;
                    else if(options[o] == "inject")
                        inject = 1;
                    else if(options[o] == "raw")
                        injectRaw = 1;
                    else if(options[o] == "!o")
                        noOutput = 1;
                    else if(options[o] == "!ret")
                        noReturnValue = 1;
                    else if(options[o] == "content")
                        attachContentPath = 1;
                }
            }

            if(params.size() == 0)
            {
                params.push_back("");
                if(read_block(params[0], linePos, inStr, readPath, lineNo, bLineNo, "system{block}", eos))
                    return 1;

                if(parseBlock && parse_replace(lang, params[0], "system block", readPath, antiDepsOfReadPath, bLineNo, "system{block}", sLineNo, eos))
                    return 1;
            }

            std::string sys_call = params[0];
            std::string exec_str;
            std::string output_filename = ".@systemoutput" + std::to_string(sys_counter++);

            #if defined _WIN32 || defined _WIN64
                if(unquote(sys_call).substr(0, 2) == "./")
                    sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
            #else  //*nix
            #endif

            if(attachContentPath)
            {
                contentAdded = 1;
                sys_call += " " + quote(toBuild.contentPath.str());
            }

            if(console + inject + injectRaw + noOutput > 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "system(" << quote(sys_call) << "): console, inject, raw and !o options are incompatible with each other" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(!(console + inject + injectRaw + noOutput))
            {
                if(lang == 'n')
                    inject = 1;
                else
                    console = 1;
            }

            //checks whether we're running from flatpak
            int result;
            if(file_exists("/.flatpak-info"))
                exec_str = "flatpak-spawn --host bash -c " + quote(sys_call);
            else
                exec_str = sys_call;

            if(console)
            {
                if(!consoleLocked)
                    os_mtx->lock();

                if(lolcat)
                    exec_str += " | " + lolcatCmd;
            }
            else
                exec_str += " > " + output_filename;

            result = system(exec_str.c_str());

            if(console && !consoleLocked)
                os_mtx->unlock();

            if(console)
            {
                if(result)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    if(mode == MODE_INTERP || mode == MODE_SHELL)
                        std::cout << "\a" << std::flush;
                    else
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "system{console}(" << quote(sys_call) << ") failed" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(mode != MODE_INTERP && !noReturnValue) //check this
                {
                    std::string resultStr = std::to_string(result);
                    outStr += resultStr;
                    if(indent)
                        outStr += into_whitespace(resultStr);
                }
            }
            else if(noOutput)
            {
                if(result)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "system{!o}(" << quote(sys_call) << ") failed, ";
                    eos << "see " << quote(output_filename) << " for pre-error system output" << std::endl;
                    os_mtx->unlock();
                    //remove_file(Path("./", output_filename));
                    return 1;
                }

                if(!noReturnValue)
                {
                    std::string resultStr = std::to_string(result);
                    outStr += resultStr;
                    if(indent)
                        outStr += into_whitespace(resultStr);
                }

                remove_file(Path("./", output_filename));
            }
            else if(injectRaw)
            {
                if(result)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "system{raw}(" << quote(sys_call) << ") failed, ";
                    eos << "see " << quote(output_filename) << " for pre-error system output" << std::endl;
                    os_mtx->unlock();
                    //remove_file(Path("./", output_filename));
                    return 1;
                }

                std::ifstream ifs(output_filename);
                std::string fileLine, oldLine;
                int fileLineNo = 0;

                while(!ifs.eof())
                {
                    getline(ifs, fileLine);
                    if(0 < fileLineNo++)
                        outStr += "\n" + indentAmount;
                    oldLine = fileLine;
                    outStr += fileLine;
                }
                if(indent)
                    indentAmount += into_whitespace(oldLine);

                ifs.close();

                remove_file(Path("./", output_filename));
            }
            else //inject
            {
                if(result)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "system{inject}(" << quote(sys_call) << ") failed, ";
                    eos << "see " << quote(output_filename) << " for pre-error system output" << std::endl;
                    os_mtx->unlock();
                    //remove_file(Path("./", output_filename));
                    return 1;
                }

                std::string fileStr = string_from_file(output_filename);

                //indent amount updated inside read_and_process
                if(n_read_and_process(1, fileStr, 0, Path("", sys_call + " - " + output_filename), antiDepsOfReadPath, outStr, eos) > 0)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "system: failed to process output of system call " << quote(sys_call) << std::endl;
                    os_mtx->unlock();
                    //remove_file(Path("./", output_filename));
                    return 1;
                }

                remove_file(Path("./", output_filename));
            }

            return 0;
        }
        else if(funcName == "scope")
        {
            if(params.size() && params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "scope: expected 0-1 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool inScopes = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "in")
                        inScopes = 1;
                }
            }

            if(params.size() == 0)
            {
                outStr += vars.layers[vars.layers.size()-1].scope;
                if(indent)
                    indentAmount += into_whitespace(vars.layers[vars.layers.size()-1].scope);
            }
            else if(params.size() == 1)
            {
                VPos vpos;

                if(vars.find(params[0], vpos))
                {
                    if(inScopes)
                    {
                        std::string inScopesStr = "{";
                        if(vars.layers[vpos.layer].inScopes[params[0]].size())
                        {
                            auto it = vars.layers[vpos.layer].inScopes[params[0]].begin();
                            inScopesStr += *it++;
                            while(it != vars.layers[vpos.layer].inScopes[params[0]].end())
                                inScopesStr += "; " + *it++;
                        }
                        inScopesStr += "}";
                        outStr += inScopesStr;
                        if(indent)
                            indentAmount += into_whitespace(inScopesStr);
                    }
                    else
                    {
                        outStr += vars.layers[vpos.layer].scopeOf[params[0]];
                        if(indent)
                            indentAmount += into_whitespace(vars.layers[vpos.layer].scopeOf[params[0]]);
                    }
                }
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "scope: no variables/functions named " << params[0] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            return 0;
        }
    }
    else if(funcName[0] == 'o')
    {
        if(funcName == "open")
        {
            if(params.size() != 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "open: expected 2 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            VPos vpos;

            if(vars.find(params[0], vpos))
            {
                if(vpos.type == "fstream")
                    vars.layers[vpos.layer].fstreams[params[0]].open(params[1]);
                else if(vpos.type == "ifstream") 
                {
                    if(dir_exists(params[1]) || !file_exists(params[1]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "cannot open " << quote(params[1]) << " as file does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    vars.layers[vpos.layer].ifstreams[params[0]].open(params[1]);
                }
                else if(vpos.type == "ofstream")
                    vars.layers[vpos.layer].ofstreams[params[0]].open(params[1]);
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << "open is not defined for variable " << quote(params[0]) << " of type " << quote(vpos.type) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "open: no variable named " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
    }
    else if(funcName[0] == 'q')
    {
        if(funcName == "quote")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "quote: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string str;

            if(replaceVars && replace_var(params[0], readPath, sLineNo, "quote", eos))
                return 1;

            params[0] = quote(params[0]);
            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
        else if(funcName == "quit")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "quit: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return NSM_QUIT; //check this??
        }
    }
    else if(funcName[0] == 'u')
    {
        if(funcName == "unquote")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "unquote: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string str;

            if(replaceVars && replace_var(params[0], readPath, sLineNo, "unquote", eos))
                return 1;

            params[0] = unquote(params[0]);
            outStr += params[0];
            if(indent)
                indentAmount += into_whitespace(params[0]);

            return 0;
        }
    }
    else if(funcName[0] == 'm')
    {
        if(funcName == "mve")
        {
            if(params.size() < 2)
            {
                if(read_sh_params(params, ' ', linePos, inStr, readPath, lineNo, funcName, eos))
                    return 1;
            }

            if(params.size() < 2)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 2+ parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            bool backupFiles = 0,
                 forceFile = 0,
                 ifNew = 0,
                 overwrite = 1,
                 changePermissions = 0, 
                 prompt = 0,
                 verbose = 0;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); ++o)
                {
                    if(options[o] == "b")
                        backupFiles = 1;
                    else if(options[o] == "f")
                        changePermissions = 1;
                    else if(options[o] == "i")
                        prompt = 1;
                    else if(options[o] == "n")
                        overwrite = 0;
                    else if(options[o] == "T")
                        forceFile = 1;
                    else if(options[o] == "u")
                        ifNew = 1;
                    else if(options[o] == "v")
                        verbose = 1;
                }
            }

            size_t t = params.size()-1;
            Path source, target;
            std::string sourceStr, 
                        targetStr = replace_home_dir(params[t]),
                        targetParam = params[t];
            char promptInput = 'n';

            params.pop_back();

            if(params.size() == 1 && 
                (forceFile || !dir_exists(targetStr)) && 
                params[0].find_first_of('*') == std::string::npos)
            {
                if(params[0].size() && (params[0][params[0].size()-1] == '/' || params[0][params[0].size()-1] == '\\'))
                {
                    int pos=params[0].size()-2;
                    while(pos>=0 && (params[0][pos] == '/' || params[0][pos] == '\\'))
                        --pos;
                    params[0] = params[0].substr(0, pos+1);
                }

                sourceStr = replace_home_dir(params[0]);
                source.set_file_path_from(sourceStr);
                target.set_file_path_from(targetStr);

                if(!path_exists(sourceStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot move ";
                    eos << quote(params[0]) << " as path does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(overwrite || !file_exists(targetStr))
                {
                    if(!ifNew || source.modified_after(target))
                    {
                        if(changePermissions)
                        {
                            if(file_exists(targetStr))
                                chmod(targetStr.c_str(), 0666);
                            else if(dir_exists(targetStr))
                                chmod(targetStr.c_str(), S_IRWXU);
                        }

                        if((prompt && promptInput != 'a') || verbose)
                        {
                            eos << quote(params[0]) << " -> " << quote(params[1]) << std::endl;

                            if(prompt && promptInput != 'a')
                            {
                                eos << "? " << std::flush;
                                promptInput = nsm_getch();
                                if(promptInput == 'Y' || promptInput == '1')
                                    promptInput = 'y';
                                else if(promptInput == 'A')
                                    promptInput = 'a';
                            }
                            
                            eos << std::endl;
                        }

                        if(!prompt || promptInput == 'y' || promptInput == 'a')
                        {
                            if(backupFiles && path_exists(targetStr))
                                rename(targetStr.c_str(), (targetStr + "~").c_str());

                            if(!can_write(targetStr) || rename(sourceStr.c_str(), targetStr.c_str()))
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to move ";
                                eos << quote(params[0]) << " to " << quote(params[1]) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                    }
                }
            }
            else
            {
                if(!dir_exists(targetStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot move files to ";
                    eos << quote(targetStr) << " as directory does not exist" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                char endChar = targetStr[targetStr.size()-1];
                if(endChar != '/' && endChar != '\\')
                    targetStr += "/";
                target.set_file_path_from(targetStr);

                struct stat  oldDirPerms;
                if(changePermissions)
                {
                    stat(targetStr.c_str(), &oldDirPerms);
                    chmod(target.dir.c_str(), S_IRWXU);
                }
                else if(!can_write(targetStr))
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot write to " << quote(target.dir) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                std::vector<Path> sources;
                for(size_t p=0; p<params.size(); ++p)
                {
                    if(params[p].size() && (params[p][params[p].size()-1] == '/' || params[p][params[p].size()-1] == '\\'))
                    {
                        int pos=params[p].size()-2; 
                        while(pos>=0 && (params[p][pos] == '/' || params[p][pos] == '\\'))
                            --pos;
                        params[p] = params[p].substr(0, pos+1);
                    }

                    sourceStr = replace_home_dir(params[p]);
                    source.set_file_path_from(sourceStr);
                    sources.push_back(source);

                    if(!path_exists(sourceStr))
                    {
                        if(sourceStr.find_first_of('*') != std::string::npos)
                        {
                            std::string parsedTxt, toParse = "@lst{!c, 1, P}(" + sourceStr + ")";
                            int parseLineNo = 0;

                            if(n_read_and_process_fast(indent, toParse, parseLineNo, readPath, antiDepsOfReadPath, parsedTxt, eos))
                            {
                                if(!consoleLocked)
                                    os_mtx->lock();
                                start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": failed to list " << quote(sourceStr) << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            std::istringstream iss(parsedTxt);
                            while(getline(iss, sourceStr))
                                params.push_back(sourceStr);

                            continue;
                        }
                        else
                        {
                            if(!consoleLocked)
                                os_mtx->lock();
                            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": cannot move ";
                            eos << quote(params[p]) << " as path does not exist" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    target.file = source.file;
                    targetStr = target.str();

                    if(overwrite || !file_exists(targetStr))
                    {
                        if(!ifNew || source.modified_after(target))
                        {
                            if(changePermissions && file_exists(targetStr))
                                chmod(targetStr.c_str(), 0666);

                            if((prompt && promptInput != 'a') || verbose)
                            {
                                if(targetParam.size())
                                {
                                    endChar = targetParam[targetParam.size()-1];
                                    if(endChar != '/' && endChar != '\\')
                                        targetParam += '/';
                                }

                                eos << quote(params[p]) << " -> " << quote(targetParam + source.file);

                                if(prompt && promptInput != 'a')
                                {
                                    eos << "? " << std::flush;
                                    promptInput = nsm_getch();
                                    if(promptInput == 'Y' || promptInput == '1')
                                        promptInput = 'y';
                                    else if(promptInput == 'A')
                                        promptInput = 'a';
                                }
                                
                                eos << std::endl;
                            }

                            if(!prompt || promptInput == 'y' || promptInput == 'a')
                            {
                                if(backupFiles && path_exists(targetStr))
                                    rename(targetStr.c_str(), (targetStr + "~").c_str());


                                if(!can_write(targetStr) || rename(sourceStr.c_str(), targetStr.c_str()))
                                {
                                    if(targetParam.size())
                                    {
                                        endChar = targetParam[targetParam.size()-1];
                                        if(endChar != '/' && endChar != '\\')
                                            targetParam += '/';
                                    }

                                    if(!consoleLocked)
                                        os_mtx->lock();
                                    start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed to move ";
                                    eos << quote(params[p]) << " to " << quote(targetParam + source.file) << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                        }
                    }
                }

                if(changePermissions)
                    chmod(target.dir.c_str(), oldDirPerms.st_mode);
            }

            if(linePos < inStr.size() && inStr[linePos] == '!')
                ++linePos;
            else
            {
                //skips to next non-whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    else
                    {
                        if(inStr[linePos] == '\n')
                            ++lineNo;
                        ++linePos;
                    }

                    if(linePos >= inStr.size())
                        break;
                }
            }

            return 0;
        }
        #if defined _WIN32 || defined _WIN64
            else if(funcName == "mv")
                funcName = "move";
        #else
            else if(funcName == "move")
                funcName = "mv";
        #endif
    }
    else if(funcName[0] == 'n')
    {
        if(funcName == "n++")
        {
            if(params.size() != 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "n++: expected 0 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int bLineNo;
            bool addOut = addOutput;

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "o")
                        addOut = 1;
                    else if(options[o] == "!o")
                        addOut = 0;
                }
            }

            params.push_back("");
            if(read_block(params[0], linePos, inStr, readPath, lineNo, bLineNo, "n++{block}", eos))
                return 1;

            std::string resultStr;
            int result = n_read_and_process_fast(1, addOut, params[0], bLineNo-1, readPath, antiDepsOfReadPath, resultStr, eos);

            if(result)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed here" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(addOut)
            {
                std::string resultStr = std::to_string(result);
                outStr += resultStr;
                if(indent)
                    indentAmount += into_whitespace(resultStr);
            }

            return result;
        }
        else if(funcName == "nsm_lang" && (mode == MODE_INTERP || mode == MODE_SHELL))
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int pos = params[0].find_first_of("nflxc", 0);

            if(pos >= 0)
            {
                if(params[0][pos] == 'n')
                    return LANG_NPP;
                else if(params[0][pos] == 'f')
                    return LANG_FPP;
                else if(params[0][pos] == 'l')
                    return LANG_LUA;
                else if(params[0][pos] == 'x' || params[0][pos] == 'e')
                    return LANG_EXPRTK;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": do not recognise the language " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(funcName == "nsm_mode" && (mode == MODE_INTERP || mode == MODE_SHELL))
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            int pos = params[0].find_first_of("si", 0);

            if(pos >= 0)
            {
                if(params[0][pos] == 's')
                    return MODE_SHELL;
                else if(params[0][pos] == 'i')
                    return MODE_INTERP;
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": do not recognise mode " << quote(params[0]) << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
    }
    else if(funcName[0] == 'j')
    {
        if(funcName == "join")
        {
            if(params.size() < 1 || params.size() > 4)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "join: expected 1-4 parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string output, separator = ", ";
            bool addAtStart = 0, addAtEnd = 0;
            VPos vpos;

            if(params.size() == 2)
                separator = params[1];

            if(options.size())
            {
                for(size_t o=0; o<options.size(); o++)
                {
                    if(options[o] == "<-")
                        addAtStart = 1;
                    else if(options[o] == "->")
                        addAtEnd = 1;
                    else if(options[o] == "<->")
                        addAtStart = addAtEnd = 1;
                }
            }

            if(vars.find(params[0] + ".size", vpos) && (vpos.type == "std::int" || vpos.type == "int"))
            {
                int sizeOf;
                if(vpos.type == "std::int")
                    sizeOf = vars.layers[vpos.layer].ints[params[0] + ".size"];
                else
                    sizeOf = vars.layers[vpos.layer].doubles[params[0] + ".size"];

                int spos = 0, epos = sizeOf-1;

                if(params.size() > 2)
                {
                    if(!isNonNegInt(params[2]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "join: ";
                        eos << "third parameter " << params[2] << " should be a non-negative integer less than the ";
                        eos << "size of struct specified in first parameter" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    spos = std::atoi(params[2].c_str());
                    if(spos >= sizeOf)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "join: ";
                        eos << "third parameter " << params[2] << " should be a non-negative integer less than the ";
                        eos << "size of struct specified in first parameter" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                if(params.size() > 3)
                {
                    if(!isNonNegInt(params[3]))
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "join: ";
                        eos << "fourth parameter " << params[3] << " should be a non-negative integer less than the ";
                        eos << "size of struct specified in first parameter" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    epos = std::atoi(params[3].c_str());
                    if(epos >= sizeOf)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << "join: ";
                        eos << "fourth parameter " << params[3] << " should be a non-negative integer less than the ";
                        eos << "size of struct specified in first parameter" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(spos <= epos)
                {
                    if(addAtStart)
                        output += separator + "$[" + params[0] + "[" + std::to_string(spos) + "]]";
                    else
                        output += "$[" + params[0] + "[" + std::to_string(spos) + "]]";
                }

                for(int i=spos+1; i<=epos; i++)
                    output += separator + "$[" + params[0] + "[" + std::to_string(i) + "]]";

                if(spos <= epos && addAtEnd)
                    output += separator;

                if(parse_replace(lang, output, "join output", readPath, antiDepsOfReadPath, lineNo, "join", sLineNo, eos))
                    return 1;

                outStr += output;
                if(indent)
                    indentAmount += into_whitespace(output);
            }
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "join: " << params[0] << ".size is not a defined integer" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            return 0;
        }
        else if(funcName == "jsinclude")
        {
            if(params.size() != 1)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "jsinclude: expected 1 parameter, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            std::string jsPathStr = params[0];
            Path jsPath;
            jsPath.set_file_path_from(jsPathStr);

            //warns user if js file doesn't exist
            if(!file_exists(jsPathStr.c_str()))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_warn(eos, readPath, lineNo) << "jsinclude: javascript file " << jsPath << " does not exist" << std::endl;
                if(!consoleLocked)
                    os_mtx->unlock();
            }

            Path pathToJSFile(pathBetween(toBuild.outputPath.dir, jsPath.dir), jsPath.file);

            std::string jsInclude="<script src=\"" + pathToJSFile.str() + "\"";
            if(options.size())
                for(size_t o=0; o<options.size(); ++o)
                    jsInclude += " " + options[o];
            jsInclude+="></script>";

            outStr += jsInclude;
            if(indent)
                indentAmount += into_whitespace(jsInclude);

            return 0;
        }
    }
    else if(funcName[0] == 'v')
    {
        if(funcName == "valid_type")
        {
            if(params.size() == 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "valid_type: expected parameters, got " << params.size() << std::endl;
                os_mtx->unlock();
                return 1;
            }

            for(size_t p=1; p<params.size(); p++)
                params[0] += ", " + params[p];

            outStr += std::to_string(valid_type(params[0], readPath, antiDepsOfReadPath, lineNo, "valid_type", sLineNo, eos));
            if(indent)
                indentAmount += " ";

            return 0;
        }
    }

    VPos vpos;
    if(vars.find_fn(funcName, vpos))
    {
        size_t layer = vars.layers.size()-1;

        if(vars.layers[vpos.layer].privates.count(funcName))
        {
            if(!vars.layers[vpos.layer].inScopes[funcName].count(vars.layers[layer].scope))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": attempted illegal access of private function" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        bool addScope = 1, addOut = 1;
        std::vector<bool> optToDel, parToDel;
        std::vector<bool> optWasConst, parWasConst;
        std::vector<std::string> oldOptTypes, oldParTypes;
        int old_no_options = 0, old_no_params = 0;
        std::vector<std::string> oldOpts, oldPars;

        std::string oldScope;

        if(vars.layers[vpos.layer].noOutput.count(funcName))
            addOut = 0;
        if(vars.layers[vpos.layer].unscopedFns.count(funcName))
            addScope = 0;

        if(options.size())
        {
            for(size_t o=0; o<options.size(); o++)
            {
                if(options[o] == "o")
                    addOut = 1;
                else if(options[o] == "!o")
                    addOut = 0;
                else if(options[o] == "s")
                    addScope = 1;
                else if(options[o] == "!s")
                    addScope = 0;
            }
        }

        //adds scope to variables
        if(addScope)
            //vars.add_layer(vars.layers[vpos.layer].scopeOf[funcName]);
            vars.add_layer(funcName.substr(0, funcName.find_last_of('.')+1));
        else //backs up variables already defined at this scope
        {
            /*oldScope = vars.layers[layer].scope;
            vars.layers[layer].scope = funcName.substr(0, funcName.find_last_of('.')+1);*/

            optToDel = std::vector<bool>(options.size() + 1, 1);
            parToDel = std::vector<bool>(params.size() + 1, 1);
            optWasConst = std::vector<bool>(options.size() + 1, 0);
            parWasConst = std::vector<bool>(params.size() + 1, 0);
            oldOptTypes = std::vector<std::string>(options.size() + 1, "");
            oldParTypes = std::vector<std::string>(params.size() + 1, "");
            oldOpts = std::vector<std::string>(options.size() + 1, "");
            oldPars = std::vector<std::string>(params.size() + 1, "");

            if(vars.layers[layer].typeOf.count("options.size"))
            {
                optToDel[options.size()] = 0;
                if(vars.layers[layer].constants.count("options.size"))
                    optWasConst[options.size()] = 1;
                oldOptTypes[options.size()] = vars.layers[layer].typeOf["options.size"];
                if(oldOptTypes[options.size()] == "std::int")
                    old_no_options = vars.layers[layer].ints["options.size"];
            }

            for(size_t o=0; o<options.size(); o++)
            {
                if(vars.layers[layer].typeOf.count("options[" + std::to_string(o) + "]"))
                {
                    optToDel[o] = 0;
                    if(vars.layers[layer].constants.count("options[" + std::to_string(o) + "]"))
                        optWasConst[o] = 1;
                    oldOptTypes[o] = vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"];
                    if(oldOptTypes[o] == "string")
                        oldOpts[o] = vars.layers[layer].strings["options[" + std::to_string(o) + "]"];
                }
            }

            if(vars.layers[layer].typeOf.count("params.size"))
            {
                parToDel[params.size()] = 0;
                if(vars.layers[layer].constants.count("params.size"))
                    parWasConst[params.size()] = 1;
                oldParTypes[params.size()] = vars.layers[layer].typeOf["params.size"];
                if(oldParTypes[params.size()] == "std::int")
                    old_no_params = vars.layers[layer].ints["params.size"];
            }

            for(size_t p=0; p<params.size(); p++)
            {
                if(vars.layers[layer].typeOf.count("params[" + std::to_string(p) + "]"))
                {
                    parToDel[p] = 0;
                    if(vars.layers[layer].constants.count("params[" + std::to_string(p) + "]"))
                        parWasConst[p] = 1;
                    oldParTypes[p] = vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"];
                    if(oldParTypes[p] == "string")
                        oldPars[p] = vars.layers[layer].strings["params[" + std::to_string(p) + "]"];
                }
            }
        }

        //defines params/options
        vars.layers[layer].typeOf["params.size"] = "std::int";
        vars.layers[layer].ints["params.size"] = params.size();
        vars.layers[layer].constants.insert("params.size");

        for(size_t p=0; p<params.size(); p++)
        {
            vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"] = "string";
            vars.layers[layer].strings["params[" + std::to_string(p) + "]"] = params[p];
            vars.layers[layer].constants.insert("params[" + std::to_string(p) + "]");
        }

        vars.layers[layer].typeOf["options.size"] = "std::int";
        vars.layers[layer].ints["options.size"] = options.size();
        vars.layers[layer].constants.insert("options.size");

        for(size_t o=0; o<options.size(); o++)
        {
            vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"] = "string";
            vars.layers[layer].strings["options[" + std::to_string(o) + "]"] = options[o];
            vars.layers[layer].constants.insert("options[" + std::to_string(o) + "]");
        }

        std::string fnOutput;
        if(lang == 'n')
        {
            if(n_read_and_process_fast(1, addOut, vars.layers[vpos.layer].functions[funcName], vars.layers[vpos.layer].ints[funcName]-1, vars.layers[vpos.layer].paths[funcName], antiDepsOfReadPath, fnOutput, eos) > 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed here" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else
        {
            if(f_read_and_process_fast(addOut, vars.layers[vpos.layer].functions[funcName], vars.layers[vpos.layer].ints[funcName]-1, vars.layers[vpos.layer].paths[funcName], antiDepsOfReadPath, fnOutput, eos) > 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": failed here" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        outStr += fnOutput;

        //removes added scope to variables
        if(addScope)
            vars.layers.pop_back();
        else //reverts existing variables otherwise deletes them
        {
            //vars.layers[layer].scope = oldScope;

            if(!optWasConst[options.size()])
                vars.layers[layer].constants.erase("options.size");
            if(oldOptTypes[options.size()] == "std::int")
                vars.layers[layer].ints["options.size"] = old_no_options;
            else
            {
                vars.layers[layer].ints.erase("options.size");
                if(oldOptTypes[options.size()].size() == 0)
                    vars.layers[layer].typeOf.erase("options.size");
                else
                    vars.layers[layer].typeOf["options.size"] = oldOptTypes[options.size()];
            }

            for(size_t o=0; o<options.size(); o++)
            {
                if(!optWasConst[o])
                    vars.layers[layer].constants.erase("options[" + std::to_string(o) + "]");
                if(oldOptTypes[o] == "string")
                    vars.layers[layer].strings["options[" + std::to_string(o) + "]"] = oldOpts[o];
                else
                {
                    vars.layers[layer].strings.erase("options[" + std::to_string(o) + "]");
                    if(oldOptTypes[o].size() == 0)
                        vars.layers[layer].typeOf.erase("options[" + std::to_string(o) + "]");
                    else
                        vars.layers[layer].typeOf["options[" + std::to_string(o) + "]"] = oldOptTypes[o];
                }
            }


            if(!parWasConst[params.size()])
                vars.layers[layer].constants.erase("params.size");
            if(oldParTypes[params.size()] == "std::int")
                vars.layers[layer].ints["params.size"] = old_no_params;
            else
            {
                vars.layers[layer].ints.erase("params.size");
                if(oldParTypes[params.size()].size() == 0)
                    vars.layers[layer].typeOf.erase("params.size");
                else
                    vars.layers[layer].typeOf["params.size"] = oldParTypes[params.size()];
            }

            for(size_t p=0; p<params.size(); p++)
            {
                if(!parWasConst[p])
                    vars.layers[layer].constants.erase("params[" + std::to_string(p) + "]");
                if(oldParTypes[p] == "string")
                    vars.layers[layer].strings["params[" + std::to_string(p) + "]"] = oldPars[p];
                else
                {
                    vars.layers[layer].strings.erase("params[" + std::to_string(p) + "]");
                    if(oldParTypes[p].size() == 0)
                        vars.layers[layer].typeOf.erase("params[" + std::to_string(p) + "]");
                    else
                        vars.layers[layer].typeOf["params[" + std::to_string(p) + "]"] = oldParTypes[p];
                }
            }
        }
    }
    else if(valid_type(funcName, readPath, antiDepsOfReadPath, lineNo, "valid_type(" + funcName + ")", sLineNo, eos))
    {
        if(params.size() < 1)
        {
            if(read_sh_params(params, ',', linePos, inStr, readPath, lineNo, funcName, eos))
                return 1;
        }

        if(!params.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << funcName << ": expected parameters, got " << params.size() << std::endl;
            os_mtx->unlock();
            return 1;
        }

        std::string toParse;
        if(lang == 'n')
            toParse = "@";
        toParse += ":=";
        if(options.size())
        {
            toParse += "{" + options[0];
            for(size_t o=1; o<options.size(); ++o)
                toParse += ", " + options[o];
            toParse += '}';
        }
        toParse += "(" + funcName;
        for(size_t p=0; p<params.size(); ++p)
            toParse += ", " + params[p];
        toParse += ')';

        if(lang == 'f')
        {
            if(f_read_and_process_fast(addOutput, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": definition failed" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else
        {
            if(n_read_and_process_fast(indent, toParse, lineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) <<  funcName << ": definition failed" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        if(linePos < inStr.size() && inStr[linePos] == '!')
            ++linePos;
        else
        {
            //skips to next non-whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find("\n", linePos);
                else
                {
                    if(inStr[linePos] == '\n')
                        ++lineNo;
                    ++linePos;
                }

                if(linePos >= inStr.size())
                    break;
            }
        }

        return 0;
    }
    else if(zeroParams)
    {
        if(lang == 'n')
        {
            linePos = sLinePos;
            if(addOutput)
            {
                outStr += "@";
                if(indent)
                    indentAmount += " ";
            }

            return 0;
        }
        else
        {
            if(addOutput)
            {
                if(inStr[sLinePos] == '"')
                    funcName = "\"" + funcName + "\"";
                else if(inStr[sLinePos] == '\'')
                    funcName = "'" + funcName + "'";
                outStr += funcName + optionsStr;
                if(indent)
                    indentAmount += into_whitespace(funcName + optionsStr);
                return 0;
            }
            else
            {
                std::string trash, toParse = "sys(" + funcName;

                if(options.size())
                {
                    for(size_t o=0; o<options.size(); ++o)
                        #if defined _WIN32 || defined _WIN64
                            toParse += " /" + options[o];
                        #else  //*nix
                            toParse += " -" + options[o];
                        #endif
                }
                else if(toParse[toParse.size()-1] == ';')
                    toParse = toParse.substr(0, toParse.size()-1);
                else
                {
                    for(;linePos < inStr.size() && inStr[linePos] != '\n'; ++linePos)
                    {
                        if(inStr[linePos] == ';')
                        {
                            ++linePos;
                            break;
                        }
                        toParse += inStr[linePos];
                    }
                }
                toParse += ")";

                if(f_read_and_process(0, toParse, sLineNo-1, readPath, antiDepsOfReadPath, trash, eos))
                {
                    if(mode != MODE_INTERP && mode != MODE_SHELL)
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << c_light_blue << funcName << c_white << ": function does not exist in this scope";
                        eos << " and failed as a system call" << std::endl;
                        os_mtx->unlock();
                    }
                    return 1;
                }

                return 0;
            }
        }
    }
    else if(lang == 'f')
    {
        if(addOutput)
        {
            if(inStr[sLinePos] == '"')
                funcName = "\"" + funcName + "\"";
            else if(inStr[sLinePos] == '\'')
                funcName = "'" + funcName + "'";
            outStr += funcName + optionsStr + paramsStr;
            if(indent)
                indentAmount += into_whitespace(funcName + optionsStr + paramsStr);
            return 0;
        }
        else
        {
            std::string trash;
            return try_system_call(funcName, options, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, trash);
        }
    }
    else
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << c_light_blue << funcName << "()" << c_white << ": function does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    return 0;
}

void Parser::get_line(const std::string& str, std::string& restOfLine, size_t& linePos)
{
    restOfLine = "";
    for(; linePos < str.size() && str[linePos] != '\n'; ++linePos)
        restOfLine += str[linePos];
}

int Parser::try_system_call(const std::string& funcName, 
                            const std::vector<std::string>& options,
                            const std::vector<std::string>& params,
                            const Path& readPath,
                            std::set<Path>& antiDepsOfReadPath,
                            const int& sLineNo,
                            const int& lineNo,
                            std::ostream& eos,
                            std::string& outStr)
{
    return try_system_call(0, funcName, options, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, outStr);
}

int Parser::try_system_call_console(const std::string& funcName, 
                            const std::vector<std::string>& options,
                            const std::vector<std::string>& params,
                            const Path& readPath,
                            std::set<Path>& antiDepsOfReadPath,
                            const int& sLineNo,
                            const int& lineNo,
                            std::ostream& eos,
                            std::string& outStr)
{
    return try_system_call(1, funcName, options, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, outStr);
}

int Parser::try_system_call_inject(const std::string& funcName, 
                            const std::vector<std::string>& options,
                            const std::vector<std::string>& params,
                            const Path& readPath,
                            std::set<Path>& antiDepsOfReadPath,
                            const int& sLineNo,
                            const int& lineNo,
                            std::ostream& eos,
                            std::string& outStr)
{
    return try_system_call(2, funcName, options, params, readPath, antiDepsOfReadPath, sLineNo, lineNo, eos, outStr);
}

int Parser::try_system_call(const int& whereTo,
                            const std::string& funcName, 
                            const std::vector<std::string>& options,
                            const std::vector<std::string>& params,
                            const Path& readPath,
                            std::set<Path>& antiDepsOfReadPath,
                            const int& sLineNo,
                            const int& lineNo,
                            std::ostream& eos,
                            std::string& outStr)
{
    std::string trash, toParse = "sys"; 
    if(whereTo == 1)
        toParse += "{console, !ret}";
    else if(whereTo == 2)
        toParse += "{inject, !ret}";
    toParse += "(" + funcName;
    for(size_t o=0; o<options.size(); ++o)
        toParse += " " + options[o];
    for(size_t p=0; p<params.size(); ++p)
        toParse += " " + params[p];
    toParse += ")";

    if(f_read_and_process(0, toParse, sLineNo-1, readPath, antiDepsOfReadPath, outStr, eos))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << c_light_blue << funcName << c_white << ": function does not exist in this scope";
        eos << " and failed as a system call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    return 0;
}

int Parser::parse_replace(const char& lang,
                  std::string& str,
                  const std::string& strType,
                  const Path& readPath,
                  std::set<Path>& antiDepsOfReadPath,
                  const int& lineNo,
                  const std::string& callType,
                  const int& callLineNo,
                  std::ostream& eos)
{
    return parse_replace(1, lang, str, strType, readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos);
}

int Parser::parse_replace(const bool& addOutput,
                  const char& lang,
                  std::string& str,
                  const std::string& strType,
                  const Path& readPath,
                  std::set<Path>& antiDepsOfReadPath,
                  const int& lineNo,
                  const std::string& callType,
                  const int& callLineNo,
                  std::ostream& eos)
{
    std::string iStr = str;
    str = "";

    std::string oldIndent = indentAmount;
    indentAmount = "";

    if(lang == 'n')
    {
        if(n_read_and_process_fast(addOutput, addOutput, iStr, lineNo-1, readPath, antiDepsOfReadPath, str, eos))
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, callLineNo) << callType << ": failed to parse " << quote(strType) << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }
    else if(lang == 'f')
    {
        if(f_read_and_process_fast(addOutput, iStr, lineNo-1, readPath, antiDepsOfReadPath, str, eos))
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, callLineNo) << callType << ": failed to parse " << quote(strType) << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }
    indentAmount = oldIndent;

    return 0;
}

int Parser::parse_replace(const char& lang,
                  std::vector<std::string>& strs,
                  const std::string& strType,
                  const Path& readPath,
                  std::set<Path>& antiDepsOfReadPath,
                  const int& lineNo,
                  const std::string& callType,
                  const int& callLineNo,
                  std::ostream& eos)
{
    std::string oldIndent = indentAmount;

    for(size_t s=0; s<strs.size(); s++)
    {
        std::string iStr = strs[s];
        strs[s] = "";

        if(lang == 'n')
        {
            indentAmount = "";

            if(n_read_and_process_fast(1, 1, iStr, lineNo-1, readPath, antiDepsOfReadPath, strs[s], eos) > 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, callLineNo) << callType << ": failed to parse " << quote(strType) << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(lang == 'f')
        {
            indentAmount = "";

            if(f_read_and_process_fast(1, iStr, lineNo-1, readPath, antiDepsOfReadPath, strs[s], eos) > 0)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, callLineNo) << callType << ": failed to parse " << quote(strType) << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
    }

    indentAmount = oldIndent;

    return 0;
}

int Parser::replace_var(std::string& str,
                         const Path& readPath,
                         const int& lineNo,
                         const std::string& callType,
                         std::ostream& eos)
{
    VPos vpos;
    if(vars.find(str, vpos))
    {
        if(vpos.type.substr(0, 5) == "std::")
        {
            if(vpos.type == "std::bool")
            {
                if(vars.layers[vpos.layer].bools[vpos.name])
                    str = "1";
                else
                    str = "0";
            }
            else if(vpos.type == "std::int")
                str = std::to_string(vars.layers[vpos.layer].ints[vpos.name]);
            else if(vpos.type == "std::double")
                str = vars.double_to_string(vars.layers[vpos.layer].doubles[vpos.name], 0);
            else if(vpos.type == "std::char")
                str = vars.layers[vpos.layer].chars[vpos.name];
            else if(vpos.type == "std::string")
                str = vars.layers[vpos.layer].strings[vpos.name];
            else if(vpos.type == "std::llint")
                str = std::to_string(vars.layers[vpos.layer].llints[vpos.name]);
        }
        else if(vpos.type == "bool")
        {
            if(vars.layers[vpos.layer].doubles[vpos.name])
                str = "1";
            else
                str = "0";
        }
        else if(vpos.type == "int")
            str = std::to_string((int)vars.layers[vpos.layer].doubles[vpos.name]);
        else if(vpos.type == "double")
            str = vars.double_to_string(vars.layers[vpos.layer].doubles[vpos.name], 0);
        else if(vpos.type == "char")
        {
            if(vars.layers[vpos.layer].strings[vpos.name].size())
                str = vars.layers[vpos.layer].strings[vpos.name][0];
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": char variable " << vpos.name << " is empty, likely caused using exprtk" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        else if(vpos.type == "string")
            str = vars.layers[vpos.layer].strings[vpos.name];
        else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": cannot replace variable named " << vpos.name << " of type " << vpos.type << " with a string" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    return 0;
}

int Parser::replace_vars(std::vector<std::string>& strs,
                         const int& spos,
                         const Path& readPath,
                         const int& lineNo,
                         const std::string& callType,
                         std::ostream& eos)
{
    VPos vpos;
    for(size_t s=spos; s<strs.size(); s++)
    {
        if(vars.find(strs[s], vpos))
        {
            if(vpos.type.substr(0, 5) == "std::")
            {
                if(vpos.type == "std::bool")
                {
                    if(vars.layers[vpos.layer].bools[strs[s] ])
                        strs[s] = "1";
                    else
                        strs[s] = "0";
                }
                else if(vpos.type == "std::int")
                    strs[s] = std::to_string(vars.layers[vpos.layer].ints[strs[s] ]);
                else if(vpos.type == "std::double")
                    strs[s] = vars.double_to_string(vars.layers[vpos.layer].doubles[strs[s] ], 0);
                else if(vpos.type == "std::char")
                    strs[s] = vars.layers[vpos.layer].chars[strs[s] ];
                else if(vpos.type == "std::string")
                    strs[s] = vars.layers[vpos.layer].strings[strs[s] ];
                else if(vpos.type == "std::llint")
                    strs[s] = std::to_string(vars.layers[vpos.layer].llints[strs[s] ]);
            }
            else if(vpos.type == "bool")
            {
                if(vars.layers[vpos.layer].doubles[strs[s] ])
                    strs[s] = "1";
                else
                    strs[s] = "0";
            }
            else if(vpos.type == "int")
                strs[s] = std::to_string((int)vars.layers[vpos.layer].doubles[strs[s] ]);
            else if(vpos.type == "double")
                strs[s] = vars.double_to_string(vars.layers[vpos.layer].doubles[strs[s] ], 0);
            else if(vpos.type == "char")
            {
                if(vars.layers[vpos.layer].strings[strs[s] ].size())
                    strs[s] = vars.layers[vpos.layer].strings[strs[s] ][0];
                else
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err(eos, readPath, lineNo) << callType << ": char variable " << strs[s] << " is empty, likely caused using exprtk" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            else if(vpos.type == "string")
                strs[s] = vars.layers[vpos.layer].strings[strs[s] ];
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": cannot replace variable named " << strs[s] << " of type " << vpos.type << " with a string" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
    }

    return 0;
}

int Parser::get_bool(bool& bVal, const std::string& str)
{
    bVal = 0;

    int typeInt = getTypeInt(str);

    if(typeInt == 0)
    {
        if(std::atoi(str.c_str()))
            bVal = 1;

        return 1;
    }
    else if(typeInt == 1)
    {
        if(std::strtod(str.c_str(), NULL))
            bVal = 1;

        return 1;
    }

    return 0;
}

int Parser::get_bool(bool& bVal,
                     const std::string& str,
                     const Path& readPath,
                     const int& lineNo,
                     const std::string& callType,
                     std::ostream& eos)
{
    if(!get_bool(bVal, str))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": cannot convert " << quote(str) << " to bool" << std::endl;
        os_mtx->unlock();
        return 0;
    }

    return 1;
}



int Parser::read_str_from_stream(const VPos& spos, std::string& str)
{
    if(spos.type == "fstream")
    {
        if(vars.layers[spos.layer].fstreams[spos.name] >> str)
            return 1;
    }
    else if(spos.type == "ifstream")
    {
        if(vars.layers[spos.layer].ifstreams[spos.name] >> str)
            return 1;
    }
    /*else if(spos.type == "sstream")
    {
        if(layers[spos.layer].sstreams[spos.name] >> str)
            return 1;
    }
    else if(spos.type == "isstream")
    {
        if(layers[spos.layer].isstreams[spos.name] >> str)
            return 1;
    }*/

    return 0;
}

int Parser::getline_from_stream(const VPos& spos, std::string& str)
{
    if(spos.type == "fstream")
    {
        if(getline(vars.layers[spos.layer].fstreams[spos.name], str))
            return 1;
    }
    else if(spos.type == "ifstream")
    {
        if(getline(vars.layers[spos.layer].ifstreams[spos.name], str))
            return 1;
    }
    /*else if(spos.type == "sstream")
    {
        if(getline(layers[spos.layer].sstreams[spos.name], str))
            return 1;
    }
    else if(spos.type == "isstream")
    {
        if(getline(layers[spos.layer].isstreams[spos.name], str))
            return 1;
    }*/

    return 0;
}

int Parser::set_var_from_str(const VPos& vpos,
                             const std::string& value,
                             const Path& readPath,
                             const int& lineNo,
                             const std::string& callType,
                             std::ostream& eos)
{
    if(vars.layers[vpos.layer].constants.count(vpos.name))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": attempted illegal change of constant variable " << quote(vpos.name) << std::endl;
        os_mtx->unlock();
        return 0;
    }
    if(vars.layers[vpos.layer].privates.count(vpos.name))
    {
        if(!vars.layers[vpos.layer].inScopes[vpos.name].count(vars.layers[vars.layers.size()-1].scope))
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": attempted illegal change of private variable " << quote(vpos.name) << std::endl;
            os_mtx->unlock();
            return 0;
        }
    }

    std::string varType = vars.layers[vpos.layer].typeOf[vpos.name];
    if(varType.substr(0, 5) == "std::")
    {
        if(varType == "std::bool")
        {
            if(isInt(value))
                vars.layers[vpos.layer].bools[vpos.name] = (bool)std::atoi(value.c_str());
            else if(isDouble(value))
                vars.layers[vpos.layer].bools[vpos.name] = (bool)std::strtod(value.c_str(), NULL);
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a boolean" << std::endl;
                start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
                os_mtx->unlock();
                return 0;
            }
        }
        else if(varType == "std::int")
        {
            if(isInt(value))
                vars.layers[vpos.layer].ints[vpos.name] = std::atoi(value.c_str());
            else if(isDouble(value))
                vars.layers[vpos.layer].ints[vpos.name] = (int)std::strtod(value.c_str(), NULL);
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not an integer" << std::endl;
                start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
                os_mtx->unlock();
                return 0;
            }
        }
        else if(varType == "std::double")
        {
            if(isDouble(value))
                vars.layers[vpos.layer].doubles[vpos.name] = std::strtod(value.c_str(), NULL);
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a double" << std::endl;
                start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
                os_mtx->unlock();
                return 0;
            }
        }
        else if(varType == "std::char")
        {
            if(value.size() == 1)
                vars.layers[vpos.layer].chars[vpos.name] = value[0];
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a character" << std::endl;
                start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
                os_mtx->unlock();
                return 0;
            }
        }
        else if(varType == "std::string")
            vars.layers[vpos.layer].strings[vpos.name] = value;
        else if(varType == "std::llint")
        {
            if(isInt(value))
                vars.layers[vpos.layer].llints[vpos.name] = std::atoll(value.c_str());
            else if(isDouble(value))
                vars.layers[vpos.layer].ints[vpos.name] = (long long int)std::strtod(value.c_str(), NULL);
            else
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not an integer" << std::endl;
                start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
                os_mtx->unlock();
                return 0;
            }
        }
    }
    else if(varType == "bool")
    {
        if(isInt(value))
            vars.layers[vpos.layer].doubles[vpos.name] = (bool)std::atoi(value.c_str());
        else if(isDouble(value))
            vars.layers[vpos.layer].doubles[vpos.name] = (bool)std::strtod(value.c_str(), NULL);
        else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a boolean" << std::endl;
            start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
            os_mtx->unlock();
            return 0;
        }
    }
    else if(varType == "int")
    {
        if(isInt(value))
            vars.layers[vpos.layer].doubles[vpos.name] = std::atoi(value.c_str());
        else if(isDouble(value))
            vars.layers[vpos.layer].doubles[vpos.name] = (int)std::strtod(value.c_str(), NULL);
        else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not an integer" << std::endl;
            start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
            os_mtx->unlock();
            return 0;
        }
    }
    else if(varType == "double")
    {
        if(isDouble(value))
            vars.layers[vpos.layer].doubles[vpos.name] = std::strtod(value.c_str(), NULL);
        else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a double" << std::endl;
            start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
            os_mtx->unlock();
            return 0;
        }
    }
    else if(varType == "char")
    {
        if(value.size() == 1)
            vars.layers[vpos.layer].strings[vpos.name] = value[0];
        else
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << callType << ": value given for " << quote(vpos.name) << " is not a character" << std::endl;
            start_err(eos, readPath, lineNo) << callType << ": value given = " << quote(value) << std::endl;
            os_mtx->unlock();
            return 0;
        }
    }
    else if(varType == "string")
        vars.layers[vpos.layer].strings[vpos.name] = value;
    else
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": cannot set variable of type " << quote(vpos.type) << " from " << quote(value) << std::endl;
        os_mtx->unlock();
        return 0;
    }

    return 1;
}


int Parser::add_fn(const std::string& fnName,
                   const char& fnLang,
                   const std::string& fnBlock,
                   const std::string& fnType,
                   const size_t& layer,
                   const bool& isConst,
                   const bool& isPrivate,
                   const bool& isUnscoped,
                   const bool& addOutput,
                   const std::unordered_set<std::string>& inScopes,
                   const Path& readPath,
                   const int& lineNo,
                   const std::string& callType,
                   std::ostream &eos)
{
    if(layer >= vars.layers.size()) //layer is a size_t which is always non-negative
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": variables does not have a layer " << layer << std::endl;
        os_mtx->unlock();
        return 1;
    }
    else if(vars.layers[layer].typeOf.count(fnName))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": redeclaration of variable/function name " << quote(fnName) << std::endl;
        os_mtx->unlock();
        return 1;
    }
    else if(vars.typeDefs.count(fnName))
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << callType << ": cannot have type and function named " << quote(fnName) << std::endl;
        os_mtx->unlock();
        return 1;
    }

    std::string scopeOf;
    int pos = find_last_of_special(fnName);

    if(pos)
        scopeOf = vars.layers[layer].scope + fnName.substr(0, pos) + ".";
    else
        scopeOf = vars.layers[layer].scope;
    vars.layers[layer].scopeOf[fnName] = scopeOf;
    vars.layers[layer].inScopes[fnName].insert(scopeOf);

    for(auto it=inScopes.begin(); it!=inScopes.end(); ++it)
        vars.layers[layer].inScopes[fnName].insert(*it);

    if(!addOutput)
        vars.layers[layer].noOutput.insert(fnName);
    if(isConst)
        vars.layers[layer].constants.insert(fnName);
    if(isPrivate)
    {
        vars.layers[layer].privates.insert(fnName);
    }
    if(isUnscoped)
        vars.layers[layer].unscopedFns.insert(fnName);

    if(fnLang == 'n')
        vars.layers[layer].nFns.insert(fnName);

    vars.layers[layer].functions[fnName] = fnBlock;
    vars.layers[layer].typeOf[fnName] = fnType;
    vars.layers[layer].scopeOf[fnName] = scopeOf;
    vars.layers[layer].paths[fnName] = readPath;
    vars.layers[layer].ints[fnName] = lineNo;

    return 0;
}

int Parser::read_func_name(std::string& funcName,
                          bool& parseFuncName,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          std::ostream& eos)
{
    size_t sLinePos = linePos;
    int sLineNo = lineNo;
    std::string extraLine;
    funcName = "";

    //normal @ or referencing variable
    if(linePos >= inStr.size() || inStr[linePos] == '\n')
        return 0;
    else if(inStr[linePos] == '$' && inStr[linePos+1] == '[')
    {
        ++linePos;
        funcName = "$";
        return 0;
    }

    //reads function name
    if(inStr[linePos] == '\'' || inStr[linePos] == '"' || inStr[linePos] == '`')
    {
        char closeChar = inStr[linePos];

        ++linePos;
        for(; inStr[linePos] != closeChar;)
        {
            if(inStr[linePos] == '@' || inStr[linePos] == '$' || inStr[linePos] == '(' || inStr[linePos] == '`')
                parseFuncName = 1;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
            {
                if(inStr[linePos+1] == 'n')
                {
                    funcName += '\n';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos+1] == 't')
                {
                    funcName += '\t';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos+1] == '\\')
                    linePos++;
                else if(inStr[linePos+1] == '\'')
                    linePos++;
                else if(inStr[linePos+1] == '"')
                    linePos++;
                else if(inStr[linePos+1] == '`')
                    linePos++;
            }
            funcName += inStr[linePos];

            ++linePos;

            while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find("\n", linePos);
                ++linePos;
                ++lineNo;

                //skips over hopefully leading whitespace
                while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                    ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << "no close " << closeChar << " for function name in " << c_light_blue << quote(funcName + "..") << c_white << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
        ++linePos;
    }
    else
    {
        int depth = 0;
        for(; linePos < inStr.size() &&
              inStr[linePos] != '(' &&
              inStr[linePos] != ')' &&
              (depth || (inStr[linePos] != '\n' &&
                        inStr[linePos] != ' ' &&
                        inStr[linePos] != '\\' && //do we want this?
                        inStr[linePos] != '@' &&
                        inStr[linePos] != '"' &&
                        inStr[linePos] != '\'' &&
                        inStr[linePos] != ']' &&
                        inStr[linePos] != '{' &&
                        inStr[linePos] != '}')); ++linePos)
        {
            if(inStr[linePos] == '[' || inStr[linePos] == '<')
                ++depth;
            else if(inStr[linePos] == ']' || inStr[linePos] == '>')
                --depth;

            if(linePos != sLinePos && (inStr[linePos] == '$' || inStr[linePos] == '`'))
                break;
            /*if(inStr[linePos] == '$' || inStr[linePos] == '`')
                parseFuncName = 1;
            else */if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
            {
                if(inStr[linePos+1] == 'n')
                {
                    funcName += '\n';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos+1] == 't')
                {
                    funcName += '\t';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos+1] == '\\')
                    linePos++;
                else if(inStr[linePos+1] == '\'')
                    linePos++;
                else if(inStr[linePos+1] == '"')
                    linePos++;
                else if(inStr[linePos+1] == '`')
                    linePos++;
            }
            funcName += inStr[linePos];
        }
        strip_trailing_whitespace(funcName);
    }

    //normal @
    //nothing to do
    /*if(linePos >= inStr.size() || (inStr[linePos] != '{' && inStr[linePos] != '(')
    {
    }*/

    return 0;
}

int Parser::read_def(std::string& varType,
                           std::vector<std::pair<std::string, std::vector<std::string> > >& vars,
                           size_t& linePos,
                           const std::string& inStr,
                           const Path& readPath,
                           int& lineNo,
                           const std::string& callType,
                           std::ostream& eos)
{
    int sLineNo = lineNo;
    std::string extraLine;
    varType = "";
    vars.clear();
    std::pair<std::string, std::vector<std::string> > newVar("", std::vector<std::string>());

    ++linePos;

    //skips to next non-whitespace
    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
    {
        if(inStr[linePos] == '@')
            linePos = inStr.find("\n", linePos);
        else
        {
            if(inStr[linePos] == '\n')
                ++lineNo;
            ++linePos;
        }

        if(linePos >= inStr.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << "no close bracket for " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    //throws error if no variable definition
    if(inStr[linePos] == ')')
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << "no variable definition inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads variable type
    if(inStr[linePos] == '\'')
    {
        ++linePos;
        for(; inStr[linePos] != '\n' && inStr[linePos] != '\''; ++linePos)
        {
            if(linePos >= inStr.size() || inStr[linePos] == '\n')
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing ' for variable type" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                linePos++;
            varType += inStr[linePos];
        }
        ++linePos;
    }
    else if(inStr[linePos] == '"')
    {
        ++linePos;
        for(; inStr[linePos] != '\n' && inStr[linePos] != '"'; ++linePos)
        {
            if(linePos >= inStr.size() || inStr[linePos] == '\n')
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing \" for variable type" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                linePos++;
            varType += inStr[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        for(; inStr[linePos] != '\n'; ++linePos)
        {
            if(inStr[linePos] == '(' || inStr[linePos] == '<')
                depth++;
            else if(inStr[linePos] == ')' || inStr[linePos] == '>')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }
            else if(depth == 1 && inStr[linePos] == ',')
                break;

            if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                linePos++;
            else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                linePos++;
            varType += inStr[linePos];
        }
        strip_trailing_whitespace(varType);
    }

    //skips to next non-whitespace
    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
    {
        if(inStr[linePos] == '@')
            linePos = inStr.find("\n", linePos);
        else
        {
            if(inStr[linePos] == '\n')
                ++lineNo;
            ++linePos;
        }

        if(linePos >= inStr.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    //throws error if no variable definition
    if(inStr[linePos] == ')')
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no variable definition with type " << quote(varType) << std::endl;
        os_mtx->unlock();
        return 1;
    }

    if(inStr[linePos] != ',')
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << " : variable definition has no comma between variable type and variables" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    while(inStr[linePos] == ',')
    {
        newVar.first = "";
        newVar.second.clear();

        linePos++;

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                if(inStr[linePos] == '\n')
                    ++lineNo;
                ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        //throws error if incomplete variable definition
        if(inStr[linePos] == ')')
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": incomplete variable definition" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //reads variable name
        if(inStr[linePos] == '\'')
        {
            ++linePos;
            for(; inStr[linePos] != '\n' && inStr[linePos] != '\''; ++linePos)
            {
                if(linePos >= inStr.size() || inStr[linePos] == '\n')
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing ' for variable name" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                    linePos++;
                newVar.first += inStr[linePos];
            }
            ++linePos;
        }
        else if(inStr[linePos] == '"')
        {
            ++linePos;
            for(; inStr[linePos] != '\n' && inStr[linePos] != '"'; ++linePos)
            {
                if(linePos >= inStr.size() || inStr[linePos] == '\n')
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing \" for variable name" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                    linePos++;
                newVar.first += inStr[linePos];
            }
            ++linePos;
        }
        else
        {
            for(; inStr[linePos] != '\n'; ++linePos)
            {
                if(inStr[linePos] == '(' || inStr[linePos] == ')' || inStr[linePos] == ',' || inStr[linePos] == '=')
                    break;

                if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                    linePos++;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                    linePos++;
                newVar.first += inStr[linePos];
            }
            strip_trailing_whitespace(newVar.first);
        }

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                if(inStr[linePos] == '\n')
                    ++lineNo;
                ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        if(linePos < inStr.size() && inStr[linePos] == '(') //reads variable parameters
        {
            if(read_params(newVar.second, linePos, inStr, readPath, lineNo, callType, eos))
                return 1;
        }
        else if(linePos < inStr.size() && inStr[linePos] == '=') //reads variable value
        {
            linePos++;
            newVar.second.push_back("");

            //skips to next non-whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find("\n", linePos);
                else
                {
                    if(inStr[linePos] == '\n')
                        ++lineNo;
                    ++linePos;
                }

                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            //throws error if no variable definition
            if(inStr[linePos] == ')')
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": incomplete variable definition" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            //reads the variable value
            if(inStr[linePos] == '\'')
            {
                ++linePos;
                for(; inStr[linePos] != '\n' && inStr[linePos] != '\''; ++linePos)
                {
                    if(linePos >= inStr.size() || inStr[linePos] == '\n')
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing ' for variable value" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                    {
                        newVar.second[0] += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                    {
                        newVar.second[0] += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                        linePos++;
                    newVar.second[0] += inStr[linePos];
                }
                ++linePos;
            }
            else if(inStr[linePos] == '"')
            {
                ++linePos;
                for(; inStr[linePos] != '\n' && inStr[linePos] != '"'; ++linePos)
                {
                    if(linePos >= inStr.size() || inStr[linePos] == '\n')
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no closing \" for variable value" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                    {
                        newVar.second[0] += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                    {
                        newVar.second[0] += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                        linePos++;
                    newVar.second[0] += inStr[linePos];
                }
                ++linePos;
            }
            else
            {
                int depth = 1;

                //reads variable value
                for(; inStr[linePos] != '\n'; ++linePos)
                {
                    if(inStr[linePos] == '(')
                        depth++;
                    else if(inStr[linePos] == ')')
                    {
                        if(depth == 1)
                            break;
                        else
                            depth--;
                    }
                    else if(depth == 1 && inStr[linePos] == ',')
                        break;

                    if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 'n')
                    {
                        newVar.second[0] += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == 't')
                    {
                        newVar.second[0] += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\\')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '\'')
                        linePos++;
                    else if(inStr[linePos] == '\\' && linePos+1 < inStr.size() && inStr[linePos+1] == '"')
                        linePos++;
                    newVar.second[0] += inStr[linePos];
                }
                strip_trailing_whitespace(newVar.second[0]);
            }

            //skips to next non-whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
            {
                if(inStr[linePos] == '@')
                    linePos = inStr.find("\n", linePos);
                else
                {
                    if(inStr[linePos] == '\n')
                        ++lineNo;
                    ++linePos;
                }

                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
        }

        vars.push_back(newVar);
    }

    //throws error if new line is between the variable definition and close bracket
    if(linePos >= inStr.size()) //probably don't need this
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": variable definition has no closing bracket ) or newline" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the variable definition is invalid
    if(inStr[linePos] != ')')
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": invalid variable definition" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_sh_params(std::vector<std::string>& params,
                          const char& separator,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    int sLineNo = lineNo;
    std::string param, extraLine;

    bool firstParam = 1;

    while(linePos < inStr.size())
    {
        param = "";

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' '  || 
                                         inStr[linePos] == '\t' ||
                                         (!firstParam && separator != ' ' && 
                                                         inStr[linePos] == '\n')))
        {
            if(inStr[linePos] == '\n')
                ++lineNo;
            ++linePos;
        }

        firstParam = 0;


        if(linePos >= inStr.size() || inStr[linePos] == '\n' || inStr[linePos] == ';')
        {
            if(!firstParam && separator != ' ')
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": missing parameter" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else
                return 0;
        }
        else if(inStr[linePos] == ';')
        {
            if(!firstParam && separator != ' ')
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": missing parameter" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else
            {
                ++linePos;
                return 0;
            }
        }

        //reads parameter
        if(inStr[linePos] == '\'' || inStr[linePos] == '"' || inStr[linePos] == '`')
        {
            char closeChar = inStr[linePos];

            if(closeChar == '`')
                param += closeChar;

            ++linePos;
            for(; inStr[linePos] != closeChar;)
            {
                if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
                {
                    if(inStr[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '\\')
                        linePos++;
                    else if(inStr[linePos+1] == '\'')
                        linePos++;
                    else if(inStr[linePos+1] == '"')
                        linePos++;
                    else if(inStr[linePos+1] == '`')
                        linePos++;
                }
                param += inStr[linePos];

                ++linePos;

                while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    ++linePos;
                    ++lineNo;

                    //skips over hopefully leading whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                        ++linePos;
                }

                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close " << closeChar << " for sh parameter " << quote(param) << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
            ++linePos;

            if(closeChar == '`')
                param += '`';
        }
        else
        {
            int depth = 0;
            for(; linePos < inStr.size() && 
                  (depth || (inStr[linePos] != ';' && 
                            inStr[linePos] != ')' &&
                            inStr[linePos] != ']' &&
                            inStr[linePos] != '}' &&
                            inStr[linePos] != separator && 
                            inStr[linePos] != '\n'));)
            {
                if(inStr[linePos] == '(' || inStr[linePos] == '[' || inStr[linePos] == '{')
                    ++depth;
                else if(inStr[linePos] == ')' || inStr[linePos] == ']' || inStr[linePos] == '}')
                    --depth;

                if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
                {
                    if(inStr[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '\\')
                        ++linePos;
                    else if(inStr[linePos+1] == '\'')
                        ++linePos;
                    else if(inStr[linePos+1] == '"')
                        ++linePos;
                    else if(inStr[linePos+1] == '`')
                        ++linePos;
                }
                param += inStr[linePos];
                ++linePos;
            }
        }

        params.push_back(param);

        if(linePos >= inStr.size() || 
           inStr[linePos] == '\n' || 
           inStr[linePos] == ')' || 
           inStr[linePos] == ']' || 
           inStr[linePos] == '}')
            return 0;
        else if(inStr[linePos] == ';')
        {
            ++linePos;
            return 0;
        }
        else if(inStr[linePos] == separator)
            ++linePos;
    }

    return 0;
}

int Parser::read_params(std::vector<std::string>& params,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    return read_params(params, ',', linePos, inStr, readPath, lineNo, callType, eos);
}

int Parser::read_params(std::vector<std::string>& params,
                          const char& separator,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    int sLineNo = lineNo;
    char closeBracket, openBracket;
    std::string param = "", extraLine;
    bool paramsAdded = 0;

    if(inStr[linePos] == '<')
    {
        openBracket = '<';
        closeBracket = '>';
    }
    else if(inStr[linePos] == '(')
    {
        openBracket = '(';
        closeBracket = ')';
    }
    else if(inStr[linePos] == '[')
    {
        openBracket = '[';
        closeBracket = ']';
    }
    else if(inStr[linePos] == '{')
    {
        openBracket = '{';
        closeBracket = '}';
    }
    else
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": could not determine open bracket type" << std::endl;
        eos << c_aqua << "note" << c_white << ": please report this on the Nift GitHub repo issues" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    do
    {
        param = "";

        ++linePos;

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                if(inStr[linePos] == '\n')
                    ++lineNo;
                ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        //no parameters
        if(linePos < inStr.size() && inStr[linePos] == closeBracket && !paramsAdded)
        {
            ++linePos;
            return 0;
        }
        paramsAdded = 1;

        //reads parameter
    
        int depth = 1;
        int noQuotedSections = 0;

        for(;;)
        {
            if(inStr[linePos] == '\'' || inStr[linePos] == '"' || inStr[linePos] == '`')
            {
                ++noQuotedSections;
                char closeChar = inStr[linePos];
                param += closeChar;

                //if(closeChar == '`')
                  //  param += closeChar;

                ++linePos;
                for(; inStr[linePos] != closeChar;)
                {
                    if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
                    {
                        if(inStr[linePos+1] == 'n')
                        {
                            param += '\n';
                            linePos+=2;
                            continue;
                        }
                        else if(inStr[linePos+1] == 't')
                        {
                            param += '\t';
                            linePos+=2;
                            continue;
                        }
                        else if(inStr[linePos+1] == '\\')
                            linePos++;
                        else if(inStr[linePos+1] == '\'')
                            linePos++;
                        else if(inStr[linePos+1] == '"')
                            linePos++;
                        else if(inStr[linePos+1] == '`')
                            linePos++;
                    }
                    param += inStr[linePos];

                    ++linePos;

                    while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
                    {
                        if(inStr[linePos] == '@')
                            linePos = inStr.find("\n", linePos);
                        ++linePos;
                        ++lineNo;

                        //skips over hopefully leading whitespace
                        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                            ++linePos;
                    }

                    if(linePos >= inStr.size())
                    {
                        if(!consoleLocked)
                            os_mtx->lock();
                        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close " << closeChar << " for option/parameter " << quote(param) << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                param += closeChar;
                ++linePos;

                //if(closeChar == '`')
                  //  param += '`';
            }
            else
            {
                if(inStr[linePos] == openBracket)
                    depth++;
                else if(inStr[linePos] == closeBracket)
                {
                    if(depth == 1)
                        break;
                    else
                        depth--;
                }
                else if(depth == 1 && inStr[linePos] == separator)
                    break;

                if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
                {
                    if(inStr[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '\\')
                        linePos++;
                    else if(inStr[linePos+1] == '\'')
                        linePos++;
                    else if(inStr[linePos+1] == '"')
                        linePos++;
                    else if(inStr[linePos+1] == '`')
                        linePos++;
                    else if(inStr[linePos+1] == separator)
                        linePos++;
                }
                param += inStr[linePos];

                ++linePos;

                while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    ++linePos;
                    ++lineNo;

                    //skips over hopefully leading whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                        ++linePos;
                }

                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket " << closeBracket << " for parameters" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
        }
        strip_trailing_whitespace(param);
        if(noQuotedSections == 1)
            param = unquote(param);

        if(separator != ';' && !param.size()) //throws error if missing parameter
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": option/parameter missing" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        params.push_back(param);

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                if(inStr[linePos] == '\n')
                    ++lineNo;
                ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }
    }while(inStr[linePos] == separator);

    //don't actually need this as it's checked at end of do while loop
    //skips to next non-whitespace
    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
    {
        if(inStr[linePos] == '@')
            linePos = inStr.find("\n", linePos);
        else
        {
            if(inStr[linePos] == '\n')
                ++lineNo;
            ++linePos;
        }

        if(linePos >= inStr.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    //throws error if there's no close bracket
    if(inStr[linePos] != closeBracket)
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close bracket " << closeBracket << " for options/parameters" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_paramsStr(std::string& paramsStr,
                          bool& parseParams,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    int sLineNo = lineNo;
    char closeBracket, openBracket;
    std::string param = "", extraLine;

    if(inStr[linePos] == '<')
    {
        openBracket = '<';
        closeBracket = '>';
    }
    else if(inStr[linePos] == '(')
    {
        openBracket = '(';
        closeBracket = ')';
    }
    else if(inStr[linePos] == '[')
    {
        openBracket = '[';
        closeBracket = ']';
    }
    else if(inStr[linePos] == '{')
    {
        openBracket = '{';
        closeBracket = '}';
    }
    else
    {
        if(!consoleLocked)
            os_mtx->lock();
        start_err(eos, readPath, lineNo) << "could not determine open bracket type with " << callType << " call" << std::endl;
        eos << c_aqua << "note" << c_white << ": please report this error on the Nift GitHub repo's issues" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    int sLinePos = linePos;
    //paramsStr += inStr[linePos];
    ++linePos;
    int depth = 1;

    for(;;)
    {
        if(inStr[linePos] == openBracket)
            depth++;
        else if(inStr[linePos] == closeBracket)
        {
            if(depth == 1)
                break;
            else
                depth--;
        }

        if(inStr[linePos] == '"' || inStr[linePos] == '\'' || inStr[linePos] == '`')
        {
            //paramsStr += inStr[linePos];
            char closeChar = inStr[linePos];
            ++linePos;

            for(;inStr[linePos] != closeChar;)
            {
                if(inStr[linePos] == '@' || 
                   inStr[linePos] == '$' || 
                   //inStr.substr(linePos, 2) == "++" ||
                   //inStr.substr(linePos, 2) == "--" ||
                   (inStr[linePos] == '+' &&  linePos+1 < inStr.size() && inStr[linePos+1] == '+') || 
                   (inStr[linePos] == '-' &&  linePos+1 < inStr.size() && inStr[linePos+1] == '-') || 
                   inStr[linePos] == '(' || 
                   inStr[linePos] == '{' || 
                   inStr[linePos] == '`')
                    parseParams = 1;
                else if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
                {
                    if(inStr[linePos+1] == 'n')
                    {
                        //paramsStr += "\\n";
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == 't')
                    {
                        //paramsStr += "\\t";
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '\\')
                    {
                        //paramsStr += "\\\\";
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '\'')
                    {
                        //paramsStr += "\\'";
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '"')
                    {
                        //paramsStr += "\\\"";
                        linePos+=2;
                        continue;
                    }
                    else if(inStr[linePos+1] == '`')
                    {
                        //paramsStr += "\\`";
                        linePos+=2;
                        continue;
                    }
                }
                //paramsStr += inStr[linePos];
                ++linePos;

                while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
                {
                    if(inStr[linePos] == '@')
                        linePos = inStr.find("\n", linePos);
                    ++linePos;
                    //++lineNo;

                    //skips over hopefully leading whitespace
                    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                        ++linePos;
                }

                if(linePos >= inStr.size())
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": uneven number of " << closeChar << " for options/parameter string" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }
        }

        if(inStr[linePos] == '@' || 
           inStr[linePos] == '$' || 
           //inStr.substr(linePos, 2) == "++" ||
           //inStr.substr(linePos, 2) == "--" ||
           (inStr[linePos] == '+' &&  linePos+1 < inStr.size() && inStr[linePos+1] == '+') || 
           (inStr[linePos] == '-' &&  linePos+1 < inStr.size() && inStr[linePos+1] == '-') || 
           inStr[linePos] == '(' || 
           inStr[linePos] == '{' || 
           inStr[linePos] == '`')
            parseParams = 1;
        else if(inStr[linePos] == '\\' && linePos+1 < inStr.size())
        {
            if(inStr[linePos+1] == 'n')
            {
                //paramsStr += "\\n";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == 't')
            {
                //paramsStr += "\\t";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == '\\')
            {
                //paramsStr += "\\\\";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == '\'')
            {
                //paramsStr += "\\'";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == '"')
            {
                //paramsStr += "\\\"";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == ',')
            {
                //paramsStr += "\\,";
                linePos+=2;
                continue;
            }
            else if(inStr[linePos+1] == '`')
            {
                //paramsStr += "\\`";
                linePos+=2;
                continue;
            }
        }
        //paramsStr += inStr[linePos];
        ++linePos;

        while(inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#"))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            ++linePos;
            //++lineNo;

            //skips over hopefully leading whitespace
            while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t'))
                ++linePos;
        }

        if(linePos >= inStr.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close " << closeBracket << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }
    //strip_trailing_whitespace(param);
    //paramsStr += inStr[linePos];
    ++linePos;
    paramsStr += inStr.substr(sLinePos, linePos-sLinePos);

    return 0;
}

int Parser::read_optionsStr(std::string& optionsStr,
                          bool& parseOptions,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    while(linePos < inStr.size() && inStr[linePos] == '{')
    {
        if(read_paramsStr(optionsStr, parseOptions, linePos, inStr, readPath, lineNo, callType, eos))
            return 1;
    }

    return 0;
}

int Parser::read_options(std::vector<std::string>& options,
                          size_t& linePos,
                          const std::string& inStr,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& eos)
{
    while(linePos < inStr.size() && inStr[linePos] == '{')
    {
        if(read_params(options, linePos, inStr, readPath, lineNo, callType, eos))
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err(eos, readPath, lineNo) << "failed to read options inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    return 0;
}

int Parser::read_block(std::string& block,
                       size_t& linePos,
                       const std::string& inStr,
                       const Path& readPath,
                       int& lineNo,
                       int& bLineNo,
                       const std::string& callType,
                       std::ostream& eos)
{
    int sLineNo = lineNo;
    std::string leadIndenting = "", extraLine;
    block = "";

    //skips to next non-whitespace
    while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
    {
        if(inStr[linePos] == '@')
            linePos = inStr.find("\n", linePos);
        else
        {
            if(inStr[linePos] == '\n')
                ++lineNo;
            ++linePos;
        }

        if(linePos >= inStr.size())
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no block" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    if(inStr[linePos] != '{')  //one-liner
    {
        bLineNo = lineNo;
        get_line(inStr, block, linePos);
    }
    else
    {
        ++linePos;
        int sLineNo = bLineNo = lineNo;
        int depth = 1;

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                if(inStr[linePos] == '\n')
                {
                    if(lineNo == sLineNo)
                        ++bLineNo;
                    else
                        block += "\n";
                    leadIndenting = "";
                    ++lineNo;
                    
                }
                else if(inStr[linePos] == ' ' || inStr[linePos] == '\t')
                    leadIndenting += inStr[linePos];
                ++linePos;
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no block" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        //bLineNo = lineNo;

        //reads the first line of the block
        for(; inStr[linePos] != '\n'; ++linePos)
        {
            if(inStr[linePos] == '{')
                ++depth;
            else if(inStr[linePos] == '}')
            {
                if(depth == 1)
                    break;
                --depth;
            }

            block += inStr[linePos];
        }

        //reads the rest of block lines
        while(inStr[linePos] == '\n')
        {
            ++linePos;
            ++lineNo;

            //checks lead indenting
            for(size_t i=0; i<leadIndenting.size() && linePos < inStr.size(); ++i, ++linePos)
            {
                if(inStr[linePos] == '\n')
                    break;
                else if(inStr[linePos] == '}' && depth == 1)
                    break;
                else if(inStr[linePos] != leadIndenting[i])
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": must use the same leading indenting as the first line of block" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
            }

            if(inStr[linePos] == '}' && depth == 1)
                break;

            block += '\n';

            //reads the line of the block
            for(; linePos < inStr.size() && inStr[linePos] != '\n'; ++linePos)
            {
                if(inStr[linePos] == '{')
                    ++depth;
                else if(inStr[linePos] == '}')
                {
                    if(depth == 1)
                        break;
                    --depth;
                }

                block += inStr[linePos];
            }

            if(linePos >= inStr.size())
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close } bracket" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        //throws error if no closing bracket
        if(inStr[linePos] != '}')
        {
            if(!consoleLocked)
                os_mtx->lock();
            start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": no close } bracket" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        ++linePos;
    }

    //not sure if this will cause problems anywhere? seems to 'fix' while loops
    //this should be added now above
    //block += "\n";

    return 0;
}

int Parser::read_else_blocks(std::vector<std::string>& conditions,
                             std::vector<int>& cLineNos,
                             std::vector<std::string>& blocks,
                             std::vector<int>& bLineNos,
                             std::string& whitespace,
                             size_t& linePos,
                             const std::string& inStr,
                             const Path& readPath,
                             int& lineNo,
                             const std::string& callType,
                             std::ostream& eos)
{
    std::string block, condition, extraLine;
    std::vector<std::string> params;
    int bLineNo;

    while(1)
    {
        whitespace = "";

        if(linePos < inStr.size() && inStr[linePos] == '!')
        {
            ++linePos;
            break;
        }

        //skips to next non-whitespace
        while(linePos < inStr.size() && (inStr[linePos] == ' ' || inStr[linePos] == '\t' || inStr[linePos] == '\n' || (inStr[linePos] == '@' && inStr.substr(linePos, 2) == "@#")))
        {
            if(inStr[linePos] == '@')
                linePos = inStr.find("\n", linePos);
            else
            {
                whitespace += inStr[linePos];
                if(inStr[linePos] == '\n')
                    ++lineNo;
                ++linePos;
            }

            if(linePos >= inStr.size())
                return 0;
        }

        if(inStr.substr(linePos, 4) == "else")
        {
            linePos += 4;

            if(inStr.substr(linePos, 3) == "-if")
            {
                linePos += 3;

                cLineNos.push_back(lineNo);
                params.clear();
                int sLineNo = lineNo;
                if(read_params(params, linePos, inStr, readPath, lineNo, callType, eos))
                    return 1;

                if(params.size() != 1)
                {
                    if(!consoleLocked)
                        os_mtx->lock();
                    start_err_ml(eos, readPath, sLineNo, lineNo) << ": else-if: expected 1 parameter, got " << params.size() << std::endl;
                    eos << params[0] << "---" << params[1] << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                conditions.push_back(params[0]);
            }

            if(read_block(block, linePos, inStr, readPath, lineNo, bLineNo, callType, eos))
                return 1;

            blocks.push_back(block);
            bLineNos.push_back(bLineNo);
        }
        else
            break;
    }
    //whitespace += '\n';

    return 0;
}

int Parser::valid_type(std::string& typeStr,
                       const Path& readPath,
                       std::set<Path>& antiDepsOfReadPath,
                       int& lineNo,
                       const std::string& callType,
                       const int& callLineNo,
                       std::ostream& eos)
{
    if(vars.basic_types.count(typeStr))
        return 1;
    else if(vars.typeDefs.count(typeStr))
    {
        VPos vpos;

        if(vars.find_fn(typeStr + ".is-valid", vpos))
        {
            std::string funcStr = "@" + typeStr + ".is-valid()";

            if(parse_replace('n', funcStr, "user-defined " + funcStr, readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                return 0; //return -1;

            return(std::atoi(funcStr.c_str()));
        }

        return 1;
    }

    std::string type = "";
    std::vector<std::string> params;

    size_t linePos = 0;

    for(; linePos<typeStr.size() && typeStr[linePos] != '<'; ++linePos)
        type += typeStr[linePos];

    if(linePos < typeStr.size() && typeStr[linePos] == '<')
    {
        //should this be an if statement with return on fail?
        read_params(params, linePos, typeStr, readPath, lineNo, callType, eos);

        /*if(type == "array")
        {
            if(params.size() != 2)
                return 0;
            if(!valid_type(params[0], readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                return 0;
            if(!isPosInt(params[1]))
            {
                if(!consoleLocked)
                    os_mtx->lock();
                start_err_ml(eos, readPath, sLineNo, lineNo) << callType << ": array<type, size>: array size must be non-negative integer, got " << quote(params[1]) << std::endl;
                os_mtx->unlock();
                return 0;
            }
            return 1;
        }
        else if(type == "map")
        {
            if(params.size() != 2)
                return 0;
            if(!valid_type(params[0], readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                return 0;
            if(!valid_type(params[1], readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                return 0;
            return 1;
        }
        else if(type == "vector")
        {
            if(params.size() != 1)
                return 0;
            if(!valid_type(params[0], readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                return 0;
            return 1;
        }
        else */if(vars.typeDefs.count(type))
        {
            VPos vpos;

            if(vars.find_fn(type + ".is-valid", vpos))
            {
                std::string funcStr = "@" + type + ".is-valid(" + join(params, ", ") + ")";

                if(parse_replace('n', funcStr, "user-defined " + funcStr, readPath, antiDepsOfReadPath, lineNo, callType, callLineNo, eos))
                    return 0; //return 1;

                return(std::atoi(funcStr.c_str()));
            }

            return 1;
        }
    }

   return 0;
}


void Parser::print_exprtk_parser_errs(std::ostream& eos, const exprtk::parser<double>& parser, const std::string& expr_str, const Path& readPath, const int& lineNo)
{
    size_t errLineNo;
    for(size_t i=0; i < parser.error_count(); ++i)
    {
        exprtk::parser_error::type error = parser.get_error(i);

        if(std::to_string(error.token.position) == "18446744073709551615")
            errLineNo = lineNo;
        else
            errLineNo = lineNo + std::count(expr_str.begin(), expr_str.begin() + error.token.position, '\n');

        eos << c_red << "error" << c_white << ": ";
        if(readPath.str() != "")
            eos << readPath;
        start_err(eos, readPath, errLineNo) << c_light_blue << "exprtk" << c_white << ": " << std::flush;
        eos << exprtk::parser_error::to_str(error.mode) << ": " << error.diagnostic << std::endl;
    }
}
