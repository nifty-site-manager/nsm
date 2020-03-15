#ifndef GETLINE_H_
#define GETLINE_H_

#include <vector>

#include "ConsoleColor.h"
#include "Consts.h"
#include "FileSystem.h"
#include "StrFns.h"

void save_session(const std::string& path);

void write_prompt(const std::string& lang, const std::string& pwd, const char& promptCh);
void write_info(const std::string& info, 
	            const std::string& pwd,
                const char& promptCh,
	            const std::string& str,
	            size_t& sPos,
	            const size_t& usableLength,
				const size_t& linePos,
				const size_t& bLinePos);

#if defined _WIN32 || defined _WIN64
	#include <conio.h>
#else  //*nix

#endif

int getline(const std::string& lang, const bool& addPwd, const char& promptCh, std::string& str, bool trackLines);

#endif //GETLINE_H_
