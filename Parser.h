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

    int parse_replace(std::string& str,
                   const std::string& strType,
                   const Path& readPath,
                   const std::set<Path>& antiDepsOfReadPath,
                   const int& lineNo,
                   const std::string& callType,
                   std::ostream& os);
    int parse_replace(std::vector<std::string>& strs,
                   const std::string& strType,
                   const Path& readPath,
                   const std::set<Path>& antiDepsOfReadPath,
                   const int& lineNo,
                   const std::string& callType,
                   std::ostream& os);

    int read_func_name(std::string& funcName,
                   size_t& linePos,
                   std::string& inLine,
                   const Path& readPath,
                   int& lineNo,
                   std::ostream& os,
                   std::istream& is);
    int read_def(std::string& varType,
                       std::vector<std::pair<std::string, std::string> >& vars,
                       size_t& linePos,
                       std::string& inLine,
                       const Path& readPath,
                       int& lineNo,
                       const std::string& callType,
                       std::ostream& os,
                       std::istream& is);
    int read_params(std::vector<std::string>& params,
                   size_t& linePos,
                   std::string& inLine,
                   const Path& readPath,
                   int& lineNo,
                   const std::string& callType,
                   std::ostream& os,
                   std::istream& is);
    int read_options(std::vector<std::string>& options,
                   size_t& linePos,
                   std::string& inLine,
                   const Path& readPath,
                   int& lineNo,
                   const std::string& callType,
                   std::ostream& os,
                   std::istream& is);
};

#endif //PARSER_H_
