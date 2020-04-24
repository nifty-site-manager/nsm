#ifndef CONSOLECOLOR_H_
#define CONSOLECOLOR_H_

#include <cstdio>
#include <iostream>
#include <fstream>

void add_colour();

#if defined _WIN32 || defined _WIN64
	#include <Windows.h>

	void use_powershell_colours();
	int using_powershell_colours();

	const HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	struct WinConsoleColor
	{
		int col, ps_col;

		WinConsoleColor(const int& Col);
		WinConsoleColor(const int& foreground, const int& background);
	};

	std::ostream& operator<<(std::ostream& os, const WinConsoleColor& cc);

	const WinConsoleColor c_aqua(11);
	const WinConsoleColor c_light_blue(3);
	const WinConsoleColor c_blue(9);
	const WinConsoleColor c_gold(6);
	const WinConsoleColor c_green(10);
	const WinConsoleColor c_purple(13);
	const WinConsoleColor c_red(12);
	const WinConsoleColor c_dark_red(4);
	const WinConsoleColor c_white(15);
	const WinConsoleColor c_yellow(14);
#else  //*nix
	struct NixConsoleColor
	{
		std::string col;

		NixConsoleColor(const std::string& Col);
	};

	std::ostream& operator<<(std::ostream& os, const NixConsoleColor& cc);

	//check also http://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html
	const NixConsoleColor c_aqua({ 0x1b, '[', '1', ';', '3', '6', 'm', 0 });
	const NixConsoleColor c_light_blue({ 0x1b, '[', '1', ';', '3', '4', 'm', 0 });
	const NixConsoleColor c_blue({ 0x1b, '[', '0', ';', '3', '4', 'm', 0 });
	const NixConsoleColor c_gold({ 0x1b, '[', '0', ';', '3', '3', 'm', 0 });
	const NixConsoleColor c_green({ 0x1b, '[', '1', ';', '3', '2', 'm', 0 });
	const NixConsoleColor c_purple({ 0x1b, '[', '1', ';', '3', '5', 'm', 0 });
	const NixConsoleColor c_red({ 0x1b, '[', '1', ';', '3', '1', 'm', 0 });
	const NixConsoleColor c_dark_red({ 0x1b, '[', '0', ';', '3', '9', 'm', 0 });
	const NixConsoleColor c_white({ 0x1b, '[', '0', ';', '3', '9', 'm', 0 });
	const NixConsoleColor c_yellow({ 0x1b, '[', '1', ';', '3', '3', 'm', 0 });

	/*const char c_aqua[]       = { 0x1b, '[', '1', ';', '3', '6', 'm', 0 };
	const char c_light_blue[] = { 0x1b, '[', '1', ';', '3', '4', 'm', 0 };
	const char c_blue[]       = { 0x1b, '[', '0', ';', '3', '4', 'm', 0 };
	const char c_gold[]       = { 0x1b, '[', '0', ';', '3', '3', 'm', 0 };
	const char c_green[]      = { 0x1b, '[', '1', ';', '3', '2', 'm', 0 };
	const char c_purple[]     = { 0x1b, '[', '1', ';', '3', '5', 'm', 0 };
	const char c_red[]        = { 0x1b, '[', '1', ';', '3', '1', 'm', 0 };
	const char c_dark_red[]   = { 0x1b, '[', '0', ';', '3', '1', 'm', 0 };
	const char c_white[]      = { 0x1b, '[', '0', ';', '3', '9', 'm', 0 };
	const char c_yellow[]     = { 0x1b, '[', '1', ';', '3', '3', 'm', 0 };*/
#endif


#endif //CONSOLECOLOR_H_
