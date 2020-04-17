#include "Getline.h"

std::vector<std::string> inputLines;
std::string homeDir;

void save_session(const std::string& path)
{
	std::ofstream ofs(path);

	for(size_t l=0; l<inputLines.size(); l++)
		ofs << inputLines[l] << std::endl;

	ofs.close();
}

void write_prompt(const std::string& lang, const std::string& pwd, const char& promptCh)
{
	//clears current line in console
	clear_console_line();
	if(lang != "")
	{
		std::cout << c_green << lang << c_white;
		if(pwd != "")
			std::cout << ":";
	}
	if(pwd != "")
		std::cout << c_purple << pwd << c_white;
	if(lang != "" || pwd != "")
		std::cout << promptCh << " ";
}

void write_input_line(const std::string& lang, 
	            const std::string& pwd,
                const char& promptCh,
	            const std::string& str,
	            size_t& sPos,
	            const size_t& usableLength,
				const size_t& linePos,
				const size_t& bLinePos)
{
	write_prompt(lang, pwd, promptCh);

	if(sPos + usableLength >= linePos)
	{
		if(str.size()-sPos <= usableLength)
		{
			std::cout << str.substr(sPos, str.size()-sPos);

			size_t ePos = 0;
			for(size_t i=ePos; i<bLinePos; i++)
				std::cout << "\b";
		}
		else
		{
			std::cout << str.substr(sPos, usableLength);

			size_t ePos = str.size() - sPos - usableLength;
			for(size_t i=ePos; i<bLinePos; i++)
				std::cout << "\b";
		}
	}
	else
	{
		sPos = str.size() - usableLength;
		std::cout << str.substr(sPos, usableLength);

		size_t ePos = 0;
		for(size_t i=ePos; i<bLinePos; i++)
			std::cout << "\b";
	}

	//std::cout << std::flush;
	std::fflush(stdout);
}

int nsm_getch()
{
	char c;

	#if defined _WIN32 || defined _WIN64
		c = _getch();
	#else
		enable_raw_mode();
		c = getchar();
		disable_raw_mode();
	#endif

	return c;
}

int rnbwcout(const std::string& str, const std::string& lolcatCmd)
{
    std::ofstream ofs(".lolcat.output");
    ofs << str << std::endl;
    ofs.close();

    int ret_val;

    #if defined _WIN32 || defined _WIN64
        ret_val = system(("cat .lolcat.output | " + lolcatCmd).c_str());
    #else
        ret_val = system(("cat .lolcat.output | " + lolcatCmd).c_str());
    #endif

    remove(".lolcat.output");

    return ret_val;
}

int rnbwcout(const std::set<std::string>& strs, const std::string& lolcatCmd)
{
    std::ofstream ofs(".lolcat.output");
    if(strs.size())
    {
	    auto str=strs.begin();
	    ofs << *str++;
	    for(; str!=strs.end(); ++str)
		    ofs << " " << *str;
		ofs << std::endl;
	}
    ofs.close();

    int ret_val;

    #if defined _WIN32 || defined _WIN64
        ret_val = system(("cat .lolcat.output | " + lolcatCmd).c_str());
    #else
        ret_val = system(("cat .lolcat.output | " + lolcatCmd).c_str());
    #endif

    remove(".lolcat.output");

    return ret_val;
}

#if defined _WIN32 || defined _WIN64
	int getline(const std::string& lang,
	            const bool& addPwd, 
	            const char& promptCh, 
	            const int& lolcat,
	            const std::string& lolcatCmd,
	            std::string& str, 
	            bool trackLines, 
	            const std::vector<std::string>& tabCompletionStrs)
	{
		char c;
		std::string pwd;
		size_t promptLength = 0, usableLength;
		int prevConsoleWidth = 100000, currConsoleWidth;
		
		size_t cLine = inputLines.size();
		std::string backupStr;

		size_t bLinePos = 0, linePos = str.size(); //line position of insertion
		size_t sPos = 0;

		if(!addPwd)
		{
			promptLength = lang.size() + 2; 
		}

		while(1)
		{
			currConsoleWidth = console_width();
			if(currConsoleWidth != prevConsoleWidth)
			{
				prevConsoleWidth = currConsoleWidth;
				if(addPwd)
				{
					pwd = get_pwd();
					homeDir = home_dir();
					if(homeDir != "" && homeDir == pwd.substr(0, homeDir.size()))
						pwd = "~" + pwd.substr(homeDir.size(), pwd.size()-homeDir.size());

					const int MIN_USABLE_LENGTH = currConsoleWidth/2;
					size_t maxPWDLength = std::max(0, (int)currConsoleWidth - (int)lang.size() - 3 - 1 - 2 - MIN_USABLE_LENGTH);
					if(pwd.size() > maxPWDLength)
						pwd = ".." + pwd.substr(pwd.size()-maxPWDLength, maxPWDLength);

					promptLength = lang.size() + pwd.size() + 3; 
				}
			}
			usableLength = std::max(0, (int)currConsoleWidth - (int)promptLength - 1);

			write_input_line(lang, pwd, promptCh, str, sPos, usableLength, linePos, bLinePos);

			c = _getch();

			if(c == '\r' || c == '\n' || c == 10) //new line
			{
				write_prompt(lang, pwd, promptCh);
				if(str.size() && str[str.size()-1] == '\\')
					std::cout << str.substr(0, str.size()-1) << std::endl;
				else
					std::cout << str << std::endl;

				if(trackLines)
					if(!inputLines.size() || str != inputLines[inputLines.size()-1])
						inputLines.push_back(str);

				if(c == 10) // ctrl enter
					return NSM_SENTER;
				else
					return 0;
			}
			else if(c == '\t') //tab completion
			{
				std::vector<int> searchPosVec, trimPosVec;
				bool foundCompletions = 0;

				int searchPos = linePos-1,
				    trimPos = -1;

				for(; searchPos >= 0 && 
				      str[searchPos] != '\n'; --searchPos)
				{
					if(str[searchPos] == ' ' || 
						str[searchPos] == '\t' ||
						str[searchPos] == '"' || 
						str[searchPos] == '\'' || 
						str[searchPos] == '`' || 
						str[searchPos] == '(' ||
						str[searchPos] == '[' ||
						str[searchPos] == '<' ||
						str[searchPos] == '{' ||
						str[searchPos] == ',')
					{
						searchPosVec.push_back(searchPos + 1);
						if(trimPos == -1)
							trimPosVec.push_back(searchPos + 1);
						else
							trimPosVec.push_back(trimPos);
					}
					if(searchPos && (str[searchPos] == '/' || str[searchPos] == '\\') && trimPos == -1)
						trimPos = searchPos+1;
				}
				searchPosVec.push_back(searchPos + 1);
				if(trimPos == -1)
					trimPosVec.push_back(searchPos + 1);
				else
					trimPosVec.push_back(trimPos);

				for(int i=searchPosVec.size()-1; i>=0; --i)
				{
					searchPos = searchPosVec[i];
					trimPos = trimPosVec[i];

					Path tabPath;
					std::set<std::string> paths, programs;
					std::string searchStr = str.substr(searchPos, linePos-searchPos);
					strip_leading_whitespace(searchStr);
					if(searchStr != "")
					{
						tabPath.set_file_path_from(searchStr.c_str());
						makeSearchable(tabPath);
						paths = lsSetStar(tabPath, -1);
					}

					if(paths.size())
					{
						foundCompletions = 1;

						auto path = paths.begin();
						std::string foundStr = *path;
						++path;
						for(; path!=paths.end(); ++path)
						{
							int pMax = std::min(foundStr.size(), path->size()),
							    pos = 0;
							for(; pos < pMax; ++pos)
							{
								if(foundStr[pos] != (*path)[pos])
									break;
							}
							foundStr = foundStr.substr(0, pos);
						}

						if(linePos-trimPos < foundStr.size())
							foundStr = foundStr.substr(linePos-trimPos, foundStr.size()-linePos+trimPos);
						else
							foundStr = "";

						if(foundStr == "" && paths.size() != 1)
						{
							std::cout << "\n";
							if(lolcat)
								rnbwcout(paths, lolcatCmd);
							else
							{
								coutPaths(tabPath.dir, paths, " ", 1, 20);
								std::cout << std::endl;
							}
						}
						else
						{
							if(paths.size() == 1)
							{
								if(!foundStr.size() || (foundStr[foundStr.size()-1] != '/' && foundStr[foundStr.size()-1] != '\\'))
								{
									//add close quote if tab completion started with a quote and not a directory
									if(searchPos > 0 && str[searchPos-1] == '"')
										foundStr += "\"";
									if(searchPos > 0 && str[searchPos-1] == '\'')
										foundStr += "'";

									//foundStr += " ";
								}
							}

							if(foundStr == "")
								std::cout << "\a" << std::flush;
							else
							{
								str = str.substr(0, linePos) + foundStr + str.substr(linePos, str.size()-linePos);
							
								for(size_t i=0; i<foundStr.size(); i++)
								{
									++linePos;
									if(sPos + usableLength +1 == linePos)
										++sPos;
								}
							}
						}

						break;
					}
					
					if(searchStr != "")
					{
						for(size_t j=0; j<tabCompletionStrs.size(); ++j)
						{
							if(searchStr == tabCompletionStrs[j].substr(0, searchStr.size()))
								programs.insert(tabCompletionStrs[j]);
						}
					}

					if(programs.size())
					{						
						foundCompletions = 1;

						if(programs.size() != 1)
						{
							std::cout << "\n";
							if(lolcat)
								rnbwcout(programs, lolcatCmd);
							else
							{
								coutPaths("", programs, " ", 0, 20);
								std::cout << std::endl;
							}
						}
						else
						{
							std::string foundStr = *programs.begin();
							foundStr = foundStr.substr(searchStr.size(), foundStr.size() - searchStr.size());
							//if(!foundStr.size() || (foundStr[foundStr.size()-1] != '/' && foundStr[foundStr.size()-1] != '\\'))
							//	foundStr += " ";

							if(foundStr == "")
								std::cout << "\a" << std::flush;
							else
							{
								str = str.substr(0, linePos) + foundStr + str.substr(linePos, str.size()-linePos);
						
								for(size_t i=0; i<foundStr.size(); i++)
								{
									++linePos;
									if(sPos + usableLength +1 == linePos)
										++sPos;
								}
							}
						}
						break;
					}
				}
				
				if(!foundCompletions)
				{
					for(int i=0; i<2; i++)
					{
						str = str.substr(0, linePos) + " " + str.substr(linePos, str.size()-linePos);
						++linePos;

						if(sPos + usableLength + 1 == linePos)
							++sPos;
					}
				}
			}
			else if(c == 0)
			{
				c = _getch();

				if(c == -108) //ctrl+tab
				{
					//std::cout << "\a" << std::flush;

					for(int i=0; i<2; i++)
					{
						str = str.substr(0, linePos) + " " + str.substr(linePos, str.size()-linePos);
						++linePos;

						if(sPos + usableLength + 1 == linePos)
							++sPos;
					}
				}
			}
			else if(c == 1) //ctrl a
			{
				bLinePos = str.size(); //line position of insertion
				linePos = sPos = 0;
			}
			else if(c == 2) //ctrl b
			{
				if(linePos > 0)
				{
					--linePos;
					++bLinePos;

					if(linePos < sPos)
						sPos = linePos;
				}
			}
			else if(c == 3 || c == 26) //ctrl c & ctrl z
			{
				//clear_console_line();
				std::cout << std::endl << c_red << "--terminated by user--" << c_white << std::endl;

				return NSM_KILL;
			}
			else if(c == 4) //ctrl d
			{
				str = str.substr(0, linePos);
				bLinePos = 0;
			}
			else if(c == 5) //ctrl e
			{
				bLinePos = sPos = 0;
				linePos = str.size();
			}
			else if(c == 6) //ctrl f
			{
				if(bLinePos > 0)
				{
					++linePos;
					--bLinePos;

					if(sPos + usableLength == linePos)
						++sPos;
				}
			}
			else if(c == 8) //backspace
			{
				if(linePos > 0)
				{
					str = str.substr(0, linePos-1) + str.substr(linePos, str.size()-linePos);
					--linePos;

					if(sPos > 0 && str.size() - sPos < usableLength)
						--sPos;
				}
			}
			else if(c == 127) //ctrl backspace
			{
				bool foundNonWhitespace = 0;
				do
				{
					if(linePos > 0)
					{
						if(str[linePos-1] != ' ' && str[linePos-1] != '\t')
							foundNonWhitespace = 1;

						str = str.substr(0, linePos-1) + str.substr(linePos, str.size()-linePos);
						--linePos;

						if(sPos > 0 && str.size() - sPos < usableLength)
							--sPos;
					}
					else
						break;
				}while(!foundNonWhitespace || (linePos > 0 && std::isalnum(str[linePos-1])));
			}
			else if(c == 18) //ctrl+r (same as opt/alt+enter)
			{
				write_prompt(lang, pwd, promptCh);
				std::cout << str << std::endl;

				if(trackLines)
					if(!inputLines.size() || str != inputLines[inputLines.size()-1])
						inputLines.push_back(str);

				return NSM_SENTER;
			}
			else if(c == -32) //check for arrow keys
			{
				c = _getch();

				if(c == -115) //ctrl up arrow
				{
				}
				else if(c == -111) //ctrl down arrow
				{
				}
				else if(c == 116) //ctrl right arrow
				{
					do
					{
						if(bLinePos > 0)
						{
							++linePos;
							--bLinePos;

							if(sPos + usableLength == linePos)
								++sPos;
						}
					}while(bLinePos > 0 && std::isalnum(str[linePos]));
				}
				else if(c == 115) //ctrl left arrow
				{
					do
					{
						if(linePos > 0)
						{
							--linePos;
							++bLinePos;

							if(linePos < sPos)
								sPos = linePos;
						}
					}while(linePos > 0 && std::isalnum(str[linePos]));
				}

				else if(c == 72) //up arrow
				{
					if(cLine > 0)
					{
						if(cLine == inputLines.size())
							backupStr = str;
						--cLine;
						str = inputLines[cLine];
						bLinePos = sPos = 0;
						linePos = str.size();
					}
				}
				else if(c == 80) //down arrow
				{
					if(cLine < inputLines.size())
					{
						++cLine;
						if(cLine == inputLines.size())
							str = backupStr;
						else
							str = inputLines[cLine];
						bLinePos = sPos = 0;
						linePos = str.size();
					}
				}
				else if(c == 77) //right arrow
				{
					if(bLinePos > 0)
					{
						++linePos;
						--bLinePos;

						if(sPos + usableLength == linePos)
							++sPos;
					}
				}
				else if(c == 75) //left arrow
				{
					if(linePos > 0)
					{
						--linePos;
						++bLinePos;

						if(linePos < sPos)
							sPos = linePos;
					}
				}
			}
			else
			{
				str = str.substr(0, linePos) + c + str.substr(linePos, str.size()-linePos);
				++linePos;

				if(sPos + usableLength +1 == linePos)
					++sPos;
			}

			write_input_line(lang, pwd, promptCh, str, sPos, usableLength, linePos, bLinePos);
		}

		return 0;
	}
#else  //*nix
	int getline(const std::string& lang, 
	            const bool& addPwd, 
	            const char& promptCh, 
	            const int& lolcat,
	            const std::string& lolcatCmd,
	            std::string& str, 
	            bool trackLines, 
	            const std::vector<std::string>& tabCompletionStrs)
	{
		char c;
		std::string pwd;
		size_t promptLength = 0, usableLength;
		int prevConsoleWidth = 100000, currConsoleWidth;
		
		size_t cLine = inputLines.size();
		std::string backupStr;

		size_t bLinePos = 0, linePos = str.size(); //line position of insertion
		size_t sPos = 0;

		if(!addPwd)
		{
			promptLength = lang.size() + 2; 
		}

		enable_raw_mode();
		while(1)
		{
			currConsoleWidth = console_width();
			if(currConsoleWidth != prevConsoleWidth)
			{
				prevConsoleWidth = currConsoleWidth;
				if(addPwd)
				{
					pwd = get_pwd();
					homeDir = home_dir();
					if(homeDir != "" && homeDir == pwd.substr(0, homeDir.size()))
						pwd = "~" + pwd.substr(homeDir.size(), pwd.size()-homeDir.size());
					#if defined __FreeBSD__
						else
						{
							homeDir = "/usr" + homeDir;
							if(homeDir != "" && homeDir == pwd.substr(0, homeDir.size()))
								pwd = "~" + pwd.substr(homeDir.size(), pwd.size()-homeDir.size());
						}
					#endif

					const int MIN_USABLE_LENGTH = currConsoleWidth/2;
					size_t maxPWDLength = std::max(0, (int)currConsoleWidth - (int)lang.size() - 3 - 1 - 2 - MIN_USABLE_LENGTH);
					if(pwd.size() > maxPWDLength)
						pwd = ".." + pwd.substr(pwd.size()-maxPWDLength, maxPWDLength);

					promptLength = lang.size() + pwd.size() + 3; 
				}
			}
			usableLength = std::max(0, (int)currConsoleWidth - (int)promptLength - 1);

			write_input_line(lang, pwd, promptCh, str, sPos, usableLength, linePos, bLinePos);

			c = getchar();

			if(c == '\r' || c == '\n') //new line
			{
				write_prompt(lang, pwd, promptCh);
				if(str.size() && str[str.size()-1] == '\\')
					std::cout << str.substr(0, str.size()-1) << std::endl;
				else
					std::cout << str << std::endl;

				if(trackLines)
					if(!inputLines.size() || str != inputLines[inputLines.size()-1])
						inputLines.push_back(str);
				disable_raw_mode(); //system("stty cooked");
				return 0;
			}
			else if(c == '\t') //tab completion
			{
				std::vector<int> searchPosVec, trimPosVec;
				bool foundCompletions = 0;

				int searchPos = linePos-1,
				    trimPos = -1;

				for(; searchPos >= 0 && 
				      str[searchPos] != '\n'; --searchPos)
				{
					if(str[searchPos] == ' ' || 
						str[searchPos] == '\t' ||
						str[searchPos] == '"' || 
						str[searchPos] == '\'' || 
						str[searchPos] == '`' || 
						str[searchPos] == '(' ||
						str[searchPos] == '[' ||
						str[searchPos] == '<' ||
						str[searchPos] == '{' ||
						str[searchPos] == ',')
					{
						searchPosVec.push_back(searchPos + 1);
						if(trimPos == -1)
							trimPosVec.push_back(searchPos + 1);
						else
							trimPosVec.push_back(trimPos);
					}
					if(searchPos && (str[searchPos] == '/' || str[searchPos] == '\\') && trimPos == -1)
						trimPos = searchPos+1;
				}
				searchPosVec.push_back(searchPos + 1);
				if(trimPos == -1)
					trimPosVec.push_back(searchPos + 1);
				else
					trimPosVec.push_back(trimPos);

				for(int i=searchPosVec.size()-1; i>=0; --i)
				{
					searchPos = searchPosVec[i];
					trimPos = trimPosVec[i];

					Path tabPath;
					std::set<std::string> paths, programs;
					std::string searchStr = str.substr(searchPos, linePos-searchPos);
					strip_leading_whitespace(searchStr);
					if(searchStr != "")
					{
						tabPath.set_file_path_from(searchStr.c_str());
						makeSearchable(tabPath);
						paths = lsSetStar(tabPath, -1);
					}

					if(paths.size())
					{
						foundCompletions = 1;

						auto path = paths.begin();
						std::string foundStr = *path;
						++path;
						for(; path!=paths.end(); ++path)
						{
							int pMax = std::min(foundStr.size(), path->size()),
							    pos = 0;
							for(; pos < pMax; ++pos)
							{
								if(foundStr[pos] != (*path)[pos])
									break;
							}
							foundStr = foundStr.substr(0, pos);
						}

						if(linePos-trimPos < foundStr.size())
							foundStr = foundStr.substr(linePos-trimPos, foundStr.size()-linePos+trimPos);
						else
							foundStr = "";

						if(foundStr == "" && paths.size() != 1)
						{
							std::cout << "\n";
							if(lolcat)
								rnbwcout(paths, lolcatCmd);
							else
							{
								coutPaths(tabPath.dir, paths, " ", 1, 20);
								std::cout << std::endl;
							}
						}
						else
						{
							if(paths.size() == 1)
							{
								if(!foundStr.size() || (foundStr[foundStr.size()-1] != '/' && foundStr[foundStr.size()-1] != '\\'))
								{
									//add close quote if tab completion started with a quote and not a directory
									if(searchPos > 0 && str[searchPos-1] == '"')
										foundStr += "\"";
									if(searchPos > 0 && str[searchPos-1] == '\'')
										foundStr += "'";

									//foundStr += " ";
								}
							}

							if(foundStr == "")
								std::cout << "\a" << std::flush;
							else
							{
								str = str.substr(0, linePos) + foundStr + str.substr(linePos, str.size()-linePos);
							
								for(size_t i=0; i<foundStr.size(); i++)
								{
									++linePos;
									if(sPos + usableLength +1 == linePos)
										++sPos;
								}
							}
						}

						break;
					}
					
					if(searchStr != "")
					{
						for(size_t j=0; j<tabCompletionStrs.size(); ++j)
						{
							if(searchStr == tabCompletionStrs[j].substr(0, searchStr.size()))
								programs.insert(tabCompletionStrs[j]);
						}
					}

					if(programs.size())
					{						
						foundCompletions = 1;

						if(programs.size() != 1)
						{
							std::cout << "\n";
							if(lolcat)
								rnbwcout(programs, lolcatCmd);
							else
							{
								coutPaths("", programs, " ", 0, 20);
								std::cout << std::endl;
							}
						}
						else
						{
							std::string foundStr = *programs.begin();
							foundStr = foundStr.substr(searchStr.size(), foundStr.size() - searchStr.size());
							//if(!foundStr.size() || (foundStr[foundStr.size()-1] != '/' && foundStr[foundStr.size()-1] != '\\'))
							//	foundStr += " ";

							if(foundStr == "")
								std::cout << "\a" << std::flush;
							else
							{
								str = str.substr(0, linePos) + foundStr + str.substr(linePos, str.size()-linePos);
						
								for(size_t i=0; i<foundStr.size(); i++)
								{
									++linePos;
									if(sPos + usableLength +1 == linePos)
										++sPos;
								}
							}
						}
						break;
					}
				}
				
				if(!foundCompletions)
				{
					for(int i=0; i<2; i++)
					{
						str = str.substr(0, linePos) + " " + str.substr(linePos, str.size()-linePos);
						++linePos;

						if(sPos + usableLength + 1 == linePos)
							++sPos;
					}
				}
			}
			else if(c == 1) //ctrl a
			{
				bLinePos = str.size(); //line position of insertion
				linePos = sPos = 0;
			}
			else if(c == 2) //ctrl b
			{
				if(linePos > 0)
				{
					--linePos;
					++bLinePos;

					if(linePos < sPos)
						sPos = linePos;
				}
			}
			else if(c == 4) //ctrl d
			{
				str = str.substr(0, linePos);
				bLinePos = 0;
			}
			else if(c == 5) //ctrl e
			{
				bLinePos = sPos = 0;
				linePos = str.size();
			}
			else if(c == 6) //ctrl f
			{
				if(bLinePos > 0)
				{
					++linePos;
					--bLinePos;

					if(sPos + usableLength == linePos)
						++sPos;
				}
			}
			#if defined __FreeBSD__
		        else if(c == 127 || c == 31) //ctrl backspace or ctrl (shift) -
		    #else  //unix
		        else if(c == 8 || c == 31) //ctrl backspace or ctrl (shift) -
		    #endif
			{
				bool foundNonWhitespace = 0;
				do
				{
					if(linePos > 0)
					{
						if(str[linePos-1] != ' ' && str[linePos-1] != '\t')
							foundNonWhitespace = 1;

						str = str.substr(0, linePos-1) + str.substr(linePos, str.size()-linePos);
						--linePos;

						if(sPos > 0 && str.size() - sPos < usableLength)
							--sPos;
					}
					else
						break;
				}while(!foundNonWhitespace || (linePos > 0 && std::isalnum(str[linePos-1])));
			}
			#if defined __FreeBSD__
		        else if(c == 8) //backspace
		    #else  //unix
		        else if(c == 127) //backspace
		    #endif
			{
				if(linePos > 0)
				{
					str = str.substr(0, linePos-1) + str.substr(linePos, str.size()-linePos);
					--linePos;

					if(sPos > 0 && str.size() - sPos < usableLength)
						--sPos;
				}
			}
			else if(c == 18) //ctrl+r (same as opt/alt+enter)
			{
				write_prompt(lang, pwd, promptCh);
				std::cout << str << std::endl;

				if(trackLines)
					if(!inputLines.size() || str != inputLines[inputLines.size()-1])
						inputLines.push_back(str);
				disable_raw_mode(); //system("stty cooked");

				return NSM_SENTER;
			}
			else if(c == '\33' || c == 27) //check for arrow keys
			{
				c = getchar();

				if(c == 10) //option/alt + enter
				{
					write_prompt(lang, pwd, promptCh);
					std::cout << str << std::endl;

					if(trackLines)
						if(!inputLines.size() || str != inputLines[inputLines.size()-1])
							inputLines.push_back(str);
					disable_raw_mode(); //system("stty cooked");

					return NSM_SENTER;
				}
				else if(c == 91)
				{
					c = getchar();

					if(c == 49)
					{
						c = getchar();

						if(c == 59)
						{
							c = getchar();

							if(c == 51 || c == 53) //alt or ctrl
							{
								c = getchar();

								if(c == 65) //alt+up and ctrl+up
								{
								}
								else if(c == 66) //alt+down and ctrl+down
								{
								}
								else if(c == 67) //alt+right and ctrl+right
								{
									do
									{
										if(bLinePos > 0)
										{
											++linePos;
											--bLinePos;

											if(sPos + usableLength == linePos)
												++sPos;
										}
									}while(bLinePos > 0 && std::isalnum(str[linePos]));
								}
								else if(c == 68) //alt+left and ctrl+left
								{
									do
									{
										if(linePos > 0)
										{
											--linePos;
											++bLinePos;

											if(linePos < sPos)
												sPos = linePos;
										}
									}while(linePos > 0 && std::isalnum(str[linePos]));
								}
							}
						}
					}
					else if(c == 65) //up arrow
					{
						if(cLine > 0)
						{
							if(cLine == inputLines.size())
								backupStr = str;
							--cLine;
							str = inputLines[cLine];
							bLinePos = sPos = 0;
							linePos = str.size();
						}
					}
					else if(c == 66) //down arrow
					{
						if(cLine < inputLines.size())
						{
							++cLine;
							if(cLine == inputLines.size())
								str = backupStr;
							else
								str = inputLines[cLine];
							bLinePos = sPos = 0;
							linePos = str.size();
						}
					}
					else if(c == 67) //right arrow
					{
						if(bLinePos > 0)
						{
							++linePos;
							--bLinePos;

							if(sPos + usableLength == linePos)
								++sPos;
						}
					}
					else if(c == 68) //left arrow
					{
						if(linePos > 0)
						{
							--linePos;
							++bLinePos;

							if(linePos < sPos)
								sPos = linePos;
						}
					}
					else if(c == 90) //shift tab
						std::cout << "\a" << std::flush;
				}
				#if defined __APPLE__
					else if(c == 91)
					{
						c = getchar();

						if(c == 65) //opt+up
						{}
						else if(c == 66) //opt+down
						{}
					}
					else if(c == 98) //opt+left
					{
						do
						{
							if(linePos > 0)
							{
								--linePos;
								++bLinePos;

								if(linePos < sPos)
									sPos = linePos;
							}
						}while(linePos > 0 && std::isalnum(str[linePos]));
					}
					else if(c == 102) //opt+right
					{
						do
						{
							if(bLinePos > 0)
							{
								++linePos;
								--bLinePos;

								if(sPos + usableLength == linePos)
									++sPos;
							}
						}while(bLinePos > 0 && std::isalnum(str[linePos]));
					}
				#endif
				else if(c == 98) //alt b
				{
					do
					{
						if(linePos > 0)
						{
							--linePos;
							++bLinePos;

							if(linePos < sPos)
								sPos = linePos;
						}
					}while(linePos > 0 && std::isalnum(str[linePos]));
				}
				else if(c == 102) //alt f
				{
					do
					{
						if(bLinePos > 0)
						{
							++linePos;
							--bLinePos;

							if(sPos + usableLength == linePos)
								++sPos;
						}
					}while(bLinePos > 0 && std::isalnum(str[linePos]));
				}
				else if(c == 127) //alt backspace
				{
					bool foundNonWhitespace = 0;
					do
					{
						if(linePos > 0)
						{
							if(str[linePos-1] != ' ' && str[linePos-1] != '\t')
								foundNonWhitespace = 1;

							str = str.substr(0, linePos-1) + str.substr(linePos, str.size()-linePos);
							--linePos;

							if(sPos > 0 && str.size() - sPos < usableLength)
								--sPos;
						}
						else
							break;
					}while(!foundNonWhitespace || (linePos > 0 && std::isalnum(str[linePos-1])));
				}
			}
			else
			{
				str = str.substr(0, linePos) + c + str.substr(linePos, str.size()-linePos);
				++linePos;

				if(sPos + usableLength +1 == linePos)
					++sPos;
			}

			//write_input_line(lang, pwd, promptCh, str, sPos, usableLength, linePos, bLinePos);
		}

		disable_raw_mode();

		return 0;
	}
#endif
