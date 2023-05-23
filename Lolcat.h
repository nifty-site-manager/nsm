#ifndef LOLCAT_H_
#define LOLCAT_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cmath>
#include <math.h>
#include <sstream>
#include <time.h>

#include "FileSystem.h"

#if defined _WIN32 || defined _WIN64
	#include <vector>
	#include <Windows.h>

	//defined in ConsoleColor.h
	//const HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	struct WinLolcatColor
	{
		int col;

		WinLolcatColor(const int& Col);
		WinLolcatColor(const int& foreground, const int& background);
	};

	std::ostream& operator<<(std::ostream& os, const WinLolcatColor& cc);

	void lolcat_powershell();
#else  //*nix
	
#endif

int lolcat(std::istream& is);
int lolfilter(std::istream &is);

int lolmain(const int& argc, const char* argv[]);

#endif //LOLCAT_H_
