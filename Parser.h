#ifndef PARSER_H_
#define PARSER_H_

#include <atomic>
#include <map>
#include <mutex>
#include <sstream>
#include <set>

#include "DateTimeInfo.h"
#include "FileSystem.h"
#include "TrackedInfo.h"

bool is_whitespace(const std::string& str);
std::string into_whitespace(const std::string& str);
void strip_trailing_whitespace(std::string& str);
bool run_script(std::ostream& os, const std::string& scriptPathStr, const bool& makeBackup, std::mutex* os_mtx);

struct Parser
{
	std::mutex* os_mtx;
    std::set<TrackedInfo>* trackedAll;
    TrackedInfo toBuild;
    DateTimeInfo dateTimeInfo;
    int codeBlockDepth,
        htmlCommentDepth;
    std::string indentAmount;
    bool contentAdded, parseParams;
    std::stringstream parsedText;
    std::ostringstream oss;
    std::set<Path> depFiles;
    std::map<std::string, std::string> strings;

    //project info
    Directory contentDir,
              outputDir;
    bool backupScripts;
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

    int build(const TrackedInfo& ToBuild, std::ostream& os);
    int read_and_process(const bool& indent,
                         std::istream& is,
                         const Path& readPath,
                         std::set<Path> antiDepsOfReadPath,
                         std::ostream& os,
                         std::ostream& eos);

    int read_path(std::string& pathRead,
                  size_t& linePos,
                  const std::string& inLine,
                  const Path& readPath,
                  const int& lineNo,
                  const std::string& callType,
                  std::ostream& os);
    int read_script_params(std::string& pathRead,
                           std::string& paramStr,
                           size_t& linePos,
                           const std::string& inLine,
                           const Path& readPath,
                           const int& lineNo,
                           const std::string& callType,
                           std::ostream& os);
    int read_msg(std::string& msgRead,
                 size_t& linePos,
                 const std::string& inLine,
                 const Path& readPath,
                 const int& lineNo,
                 const std::string& callType,
                 std::ostream& os);
	int read_sys_call(std::string& sys_call,
	                  size_t& linePos,
	                  const std::string& inLine,
	                  const Path& readPath,
	                  const int& lineNo,
	                  const std::string& callType,
	                  std::ostream& os);
    int read_def(std::string& varType,
                       std::vector<std::pair<std::string, std::string> >& vars,
                       size_t& linePos,
                       const std::string& inLine,
                       const Path& readPath,
                       const int& lineNo,
                       const std::string& callType,
                       std::ostream& os);
    int read_var(std::string& varName,
                 size_t& linePos,
                 const std::string& inLine,
                 const Path& readPath,
                 const int& lineNo,
                 const std::string& callType,
                 std::ostream& os);
    int read_param(std::string& param,
                   size_t& linePos,
                   const std::string& inLine,
                   const Path& readPath,
                   const int& lineNo,
                   const std::string& callType,
                   std::ostream& os);
};

#endif //PARSER_H_
