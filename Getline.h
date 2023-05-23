#ifndef GETLINE_H_
#define GETLINE_H_

#include <vector>

#include "ConsoleColor.h"
#include "Consts.h"
#include "FileSystem.h"
#include "Lolcat.h"
#include "StrFns.h"

void save_session(const std::string& path);

void write_prompt(const std::string& lang, const std::string& pwd, const std::string& promptCh);
void write_info(const std::string& info, 
	            const std::string& pwd,
                const std::string& promptCh,
	            const std::string& str,
	            size_t& sPos,
	            const size_t& usableLength,
				const size_t& linePos,
				const size_t& bLinePos);

#if defined _WIN32 || defined _WIN64
	#include <conio.h>
#else  //*nix

#endif

int nsm_getch();

int rnbwcout(const std::string& str);
int rnbwcout(const std::set<std::string>& strs, const std::string& lolcatCmd);

int getline(const std::string& lang,
            const bool& addPwd, 
            const std::string& promptCh, 
	        const int& lolcatActive,
            std::string& str, 
            bool trackLines, 
            const std::vector<std::string>& tabCompletionStrs);

#endif //GETLINE_H_
