#ifndef PARSER_H_
#define PARSER_H_

#include <atomic>
#include <cmath>
#include <math.h>
#include <map>
#include <mutex>
#include <sstream>
#include <set>
#include <algorithm>
//#include <bits/stdc++.h> //doesn't work on osx, algorithm works instead

#include "DateTimeInfo.h"
#include "Expr.h"
#include "ExprtkFns.h"
#include "Getline.h"
#include "hashtk/HashTk.h"
#include "LuaFns.h"
#include "Lua.h"
#include "Pagination.h"
#include "SystemInfo.h"
#include "TrackedInfo.h"
#include "Variables.h"

#include "Timer.h"

int find_last_of_special(const std::string& s);

void setIncrMode(const int& IncrMode);

struct Parser
{
	std::mutex* os_mtx;
	std::set<TrackedInfo>* trackedAll;
	TrackedInfo toBuild;
	DateTimeInfo dateTimeInfo;
	int codeBlockDepth,
	    htmlCommentDepth;
	std::string indentAmount;
	bool addMemberFnsGlobal, addScopeGlobal, replaceVarsGlobal;
	bool contentAdded;
	std::string parsedText;
	int mode;
	bool lolcatActive, lolcatInit;
	std::string lolcatCmd;
	std::set<Path> depFiles, includedFiles;
	std::istringstream dummy_iss;

	std::vector<std::string> tabCompletionStrs;

	Pagination pagesInfo;

	Variables vars;
	Lua lua;
	exprtk::symbol_table<double>           symbol_table;
	exprtk::rtl::io::package<double>       basicio_package; //could potentially make these pointers and
	exprtk::rtl::io::file::package<double> fileio_package;  //only initialise if/when added
	exprtk::rtl::vecops::package<double>   vectorops_package;
	Expr expr;
	ExprSet exprset;

	exprtk_cd<double>            exprtk_cd_fn;
	exprtk_sys<double>           exprtk_sys_fn;
	exprtk_to_string<double>     exprtk_to_string_fn;
	exprtk_nsm_tonumber<double>  exprtk_nsm_tonumber_fn;
	exprtk_nsm_tostring<double>  exprtk_nsm_tostring_fn;
	exprtk_nsm_setnumber<double> exprtk_nsm_setnumber_fn;
	exprtk_nsm_setstring<double> exprtk_nsm_setstring_fn;
	exprtk_nsm_write<double>     exprtk_nsm_write_fn;

	//project info
	Directory contentDir,
	          outputDir;
	bool backupScripts, consoleLocked;
	Path consoleLockedPath;
	int consoleLockedOnLine;
	std::string contentExt,
	            outputExt,
	            scriptExt,
	            unixTextEditor,
	            winTextEditor;
	Path defaultTemplate;

	Parser(std::set<TrackedInfo>* TrackedAll,
	       std::mutex* OS_mtx,
	       const Directory& ContentDir,
	       const Directory& OutputDir,
	       const std::string& ContentExt,
	       const std::string& OutputExt,
	       const std::string& ScriptExt,
	       const Path& DefaultTemplate,
	       const bool& makeBackup,
	       const std::string& UnixTextEditor,
	       const std::string& WinTextEditor);

	int lua_addnsmfns();
	int lolcat_init(const std::string& lolcat_cmd);

	int run_script(std::ostream& os,
	               const Path& scriptPath, 
	               const bool& makeBackup, 
	               const bool& outputWhatDoing);

	int refresh_completions();
	int shell(std::string& lang, std::ostream& eos);
	int interpreter(std::string& lang, std::ostream& eos);
	int interactive(std::string& lang, std::ostream& eos);
	int run(const Path& path, char lang, std::ostream& eos);
	int build(const TrackedInfo& ToBuild,
	          std::atomic<double>& estNoPagesFinished,
	          std::atomic<int>& noPagesToBuild,
	          std::ostream& eos);
	int n_read_and_process(const bool& indent,
	                       const std::string& inStr,
	                       int lineNo,
	                       const Path& readPath,
	                       std::set<Path> antiDepsOfReadPath,
	                       std::string& outStr,
	                       std::ostream& eos);
	int n_read_and_process_fast(const bool& indent,
	                            const std::string& inStr,
	                            int lineNo,
	                            const Path& readPath,
	                            std::set<Path>& antiDepsOfReadPath,
	                            std::string& outStr,
	                            std::ostream& eos);
	int n_read_and_process_fast(const bool& indent,
	                            const bool& addOutput,
	                            const std::string& inStr,
	                            int lineNo,
	                            const Path& readPath,
	                            std::set<Path>& antiDepsOfReadPath,
	                            std::string& outStr,
	                            std::ostream& eos);
	int f_read_and_process(const bool& addOutput,
	                       const std::string& inStr,
	                       int lineNo,
	                       const Path& readPath,
	                       std::set<Path> antiDepsOfReadPath,
	                       std::string& outStr,
	                       std::ostream& eos);
	int f_read_and_process_fast(const bool& addOutput,
	                            const std::string& inStr,
	                            int lineNo,
	                            const Path& readPath,
	                            std::set<Path>& antiDepsOfReadPath,
	                            std::string& outStr,
	                            std::ostream& eos);
	int read_and_process_fn(const bool& indent,
	                        const std::string& baseIndentAmount,
	                        const char& lang,
	                        const bool& addOutput,
	                        const std::string& inStr,
	                        int& lineNo,
	                        size_t& linePos,
	                        const Path& readPath,
	                        std::set<Path>& antiDepsOfReadPath,
	                        std::string& outStr,
	                        std::ostream& eos);

	void get_line(const std::string& str, std::string& restOfLine, size_t& linePos);

	int try_system_call(const std::string& funcName, 
	                    const std::vector<std::string>& options,
	                    const std::vector<std::string>& params,
	                    const Path& readPath,
	                    std::set<Path>& antiDepsOfReadPath,
	                    const int& sLineNo,
	                    const int& lineNo,
	                    std::ostream& eos,
	                    std::string& outStr);
	int try_system_call_console(const std::string& funcName, 
	                    const std::vector<std::string>& options,
	                    const std::vector<std::string>& params,
	                    const Path& readPath,
	                    std::set<Path>& antiDepsOfReadPath,
	                    const int& sLineNo,
	                    const int& lineNo,
	                    std::ostream& eos,
	                    std::string& outStr);
	int try_system_call_inject(const std::string& funcName, 
	                    const std::vector<std::string>& options,
	                    const std::vector<std::string>& params,
	                    const Path& readPath,
	                    std::set<Path>& antiDepsOfReadPath,
	                    const int& sLineNo,
	                    const int& lineNo,
	                    std::ostream& eos,
	                    std::string& outStr);
	int try_system_call(const int& whereTo,
	                    const std::string& funcName, 
	                    const std::vector<std::string>& options,
	                    const std::vector<std::string>& params,
	                    const Path& readPath,
	                    std::set<Path>& antiDepsOfReadPath,
	                    const int& sLineNo,
	                    const int& lineNo,
	                    std::ostream& eos,
	                    std::string& outStr);

	int parse_replace(const char& lang,
	                  std::string& str,
	                  const std::string& strType,
	                  const Path& readPath,
	                  std::set<Path>& antiDepsOfReadPath,
	                  const int& lineNo,
	                  const std::string& callType,
	                  const int& callLineNo,
	                  std::ostream& eos);
	int parse_replace(const bool& addOutput,
	                  const char& lang,
	                  std::string& str,
	                  const std::string& strType,
	                  const Path& readPath,
	                  std::set<Path>& antiDepsOfReadPath,
	                  const int& lineNo,
	                  const std::string& callType,
	                  const int& callLineNo,
	                  std::ostream& eos);
	int parse_replace(const char& lang,
	                  std::vector<std::string>& strs,
	                  const std::string& strType,
	                  const Path& readPath,
	                  std::set<Path>& antiDepsOfReadPath,
	                  const int& lineNo,
	                  const std::string& callType,
	                  const int& callLineNo,
	                  std::ostream& eos);

	int replace_var(std::string& str,
	                const Path& readPath,
	                const int& lineNo,
	                const std::string& callType,
	                std::ostream& eos);
	int replace_vars(std::vector<std::string>& strs,
	                 const int& spos,
	                 const Path& readPath,
	                 const int& lineNo,
	                 const std::string& callType,
	                 std::ostream& eos);

	int get_bool(bool& bVal, const std::string& str);
	int get_bool(bool& bVal,
	             const std::string& str,
	             const Path& readPath,
	             const int& lineNo,
	             const std::string& callType,
	             std::ostream& eos);

	  int read_str_from_stream(const VPos& spos, std::string& str);
	  int getline_from_stream(const VPos& spos, std::string& str);
	  int set_var_from_str(const VPos& vpos,
	                       const std::string& value,
	                       const Path& readPath,
	                       const int& lineNo,
	                       const std::string& callType,
	                     std::ostream& eos);
	int add_fn(const std::string& fnName,
	           const char& fnLang,
	           const std::string& fnBlock,
	           const std::string& fnType,
	           const size_t& layer,
	           const bool& isConst,
	           const bool& isPrivate,
	           const bool& isUnscoped,
	           const bool& addOut,
	           const std::unordered_set<std::string>& inScopes,
	           const Path& readPath,
	           const int& lineNo,
	           const std::string& callType,
	           std::ostream &eos);


	int read_func_name(std::string& funcName,
	                   bool& parseFuncName,
	                   size_t& linePos,
	                   const std::string& inStr,
	                   const Path& readPath,
	                   int& lineNo,
	                   std::ostream& eos);
	int read_def(std::string& varType,
	             std::vector<std::pair<std::string, std::vector<std::string> > >& vars,
	             size_t& linePos,
	             const std::string& inStr,
	             const Path& readPath,
	             int& lineNo,
	             const std::string& callType,
	             std::ostream& eos);
	int read_sh_params(std::vector<std::string>& params,
	                   const char& separator,
	                   size_t& linePos,
	                   const std::string& inStr,
	                   const Path& readPath,
	                   int& lineNo,
	                   const std::string& callType,
	                   std::ostream& eos);
	int read_params(std::vector<std::string>& params,
	                size_t& linePos,
	                const std::string& inStr,
	                const Path& readPath,
	                int& lineNo,
	                const std::string& callType,
	                std::ostream& eos);
	int read_params(std::vector<std::string>& params,
	                const char& separator,
	                size_t& linePos,
	                const std::string& inStr,
	                const Path& readPath,
	                int& lineNo,
	                const std::string& callType,
	                std::ostream& eos);
	int read_paramsStr(std::string& paramsStr,
	                   bool& parseParams,
	                   size_t& linePos,
	                   const std::string& inStr,
	                   const Path& readPath,
	                   int& lineNo,
	                   const std::string& callType,
	                   std::ostream& eos);
	int read_optionsStr(std::string& optionsStr,
	                    bool& parseOptions,
	                    size_t& linePos,
	                    const std::string& inStr,
	                    const Path& readPath,
	                    int& lineNo,
	                    const std::string& callType,
	                    std::ostream& eos);
	int read_options(std::vector<std::string>& options,
	                 size_t& linePos,
	                 const std::string& inStr,
	                 const Path& readPath,
	                 int& lineNo,
	                 const std::string& callType,
	                 std::ostream& eos);
	int read_block(std::string& block,
	               size_t& linePos,
	               const std::string& inStr,
	               const Path& readPath,
	               int& lineNo,
	               int& bLineNo,
	               const std::string& callType,
	               std::ostream& eos);
	int read_block_del(std::string& block,
	               size_t& linePos,
	               const std::string& inStr,
	               const Path& readPath,
	               int& lineNo,
	               int& bLineNo,
	               const std::string& callType,
	               std::ostream& eos);
	int read_else_blocks(std::vector<std::string>& conditions,
	                     std::vector<int>& cLineNos,
	                     std::vector<std::string>& blocks,
	                     std::vector<int>& bLineNos,
	                     std::string& whitespace,
	                     size_t& linePos,
	                     const std::string& inStr,
	                     const Path& readPath,
	                     int& lineNo,
	                     const std::string& callType,
	                     std::ostream& eos);

	int valid_type(std::string& typeStr,
	               const Path& readPath,
	               std::set<Path>& antiDepsOfReadPath,
	               int& lineNo,             //can this be constant?
				   const std::string& callType,
				   const int& callLineNo,
				   std::ostream& eos);

	void print_exprtk_parser_errs(std::ostream& eos, 
	                              const exprtk::parser<double>& parser, 
	                              const std::string& expr_str, 
	                              const Path& readPath, 
	                              const int& lineNo);
};

#endif //PARSER_H_
