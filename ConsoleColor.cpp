#include "ConsoleColor.h"

#if defined _WIN32 || defined _WIN64
	WinConsoleColor::WinConsoleColor()
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		col = 12; //red txt black background
		//15 white txt black background
	}

	WinConsoleColor::WinConsoleColor(const int& Col)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		col = Col;
	}

	WinConsoleColor::WinConsoleColor(const int& foreground, const int& background)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		col = foreground + background*16;
	}

	std::ostream& operator<<(std::ostream& os, const WinConsoleColor& cc)
	{
		FlushConsoleInputBuffer(cc.hConsole);
		SetConsoleTextAttribute(cc.hConsole, cc.col);

		return os;
	}

#endif
