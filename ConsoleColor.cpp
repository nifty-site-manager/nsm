#include "ConsoleColor.h"

#if defined _WIN32 || defined _WIN64
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
		if(&os == &std::cout)
		{
			FlushConsoleInputBuffer(cc.hConsole);
			SetConsoleTextAttribute(cc.hConsole, cc.col);
		}

		return os;
	}
#else
	NixConsoleColor::NixConsoleColor(const std::string& Col)
	{
		col = Col;
	}

	std::ostream& operator<<(std::ostream& os, const NixConsoleColor& cc)
	{
		if(&os == &std::cout)
			os << cc.col.c_str();

		return os;
	}
#endif
