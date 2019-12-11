#ifndef PAGE_BUILDER_H_
#define PAGE_BUILDER_H_

#include <atomic>
#include <map>
#include <mutex>
#include <sstream>
#include <set>

#include "DateTimeInfo.h"
#include "FileSystem.h"
#include "PageInfo.h"

bool is_whitespace(const std::string& str);
std::string into_whitespace(const std::string& str);
bool run_script(std::ostream& os, const std::string& scriptPathStr, const bool& makeBackup, std::mutex* os_mtx);

struct PageBuilder
{
	std::mutex* os_mtx;
    std::set<PageInfo>* pages;
    PageInfo pageToBuild;
    DateTimeInfo dateTimeInfo;
    int codeBlockDepth,
        htmlCommentDepth;
    std::string indentAmount;
    bool contentAdded, parseParams;
    std::stringstream processedPage;
    std::ostringstream oss;
    std::set<Path> pageDeps;
    std::map<std::string, std::string> strings;

    //site info
    Directory contentDir,
              siteDir;
    bool backupScripts;
    std::string contentExt,
                pageExt,
                scriptExt,
                unixTextEditor,
                winTextEditor;
    Path defaultTemplate;

    PageBuilder(std::set<PageInfo>* Pages,
                std::mutex* OS_mtx,
                const Directory& ContentDir,
                const Directory& SiteDir,
                const std::string& ContentExt,
                const std::string& PageExt,
                const std::string& ScriptExt,
                const Path& DefaultTemplate,
                const bool& makeBackup,
                const std::string& UnixTextEditor,
                const std::string& WinTextEditor);

    int build(const PageInfo& PageToBuild, std::ostream& os);
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
    int read_stringdef(std::string& varName,
                       std::string& varVal,
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
};

#endif //PAGE_BUILDER_H_
