#include "ConsoleColor.h"

#if defined _WIN32 || defined _WIN64
	bool addColour = 0;
#else
	bool addColour = 1;
#endif

void add_colour()
{
    addColour = 1;
}

void no_colour()
{
    addColour = 0;
}

#if defined _WIN32 || defined _WIN64
	bool use_ps_colours = 0;

	void use_powershell_colours()
	{
		use_ps_colours = 1;
	}

	int using_powershell_colours()
	{
		return use_ps_colours;
	}

	WinConsoleColor::WinConsoleColor(const int& Col)
	{
		col = Col;
		if(Col == 6)
			ps_col = 14 + 5*16;
		else
			ps_col = Col + 5*16;
	}

	WinConsoleColor::WinConsoleColor(const int& foreground, const int& background)
	{
		col = ps_col = foreground + background*16;
	}

	std::ostream& operator<<(std::ostream& os, const WinConsoleColor& cc)
	{
		#if defined __NO_COLOUR__
			if(cc.col){} //gets rid of warning
	    #else
	    	if(addColour && &os == &std::cout)
			{
				FlushConsoleInputBuffer(hConsole);
				if(use_ps_colours)
					SetConsoleTextAttribute(hConsole, cc.ps_col);
				else
					SetConsoleTextAttribute(hConsole, cc.col);
			}
	    #endif

		return os;
	}
#else
	NixConsoleColor::NixConsoleColor(const std::string& Col)
	{
		col = Col;
	}

	std::ostream& operator<<(std::ostream& os, const NixConsoleColor& cc)
	{
		#if defined __NO_COLOUR__
			if(cc.col.size()){} //gets rid of warning
	    #else
	    	if(addColour && &os == &std::cout)
				os << cc.col.c_str();
	    #endif

		return os;
	}
#endif
