#ifndef SYSTEMINFO_H_
#define SYSTEMINFO_H_

#include <cstdio>
#include <iostream>

#if defined _WIN32 || defined _WIN64
	//get hostname
	#include <windows.h>
	#include <tchar.h>

	//// console-dimensions

	//#include <windows.h>

	//// homedir & app dir

	//#include <windows.h>
	#include <shlobj.h>
#else  //*nix
	//get hostname
	#include <unistd.h>
	#include <limits.h>

	//// console-dimensions

	//#include <unistd.h>
	#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ


	//// raw-mode

	#include <termios.h>
	//#include <unistd.h>
	//https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html

	void disable_raw_mode();
	void enable_raw_mode();

	//// home-dir

	//#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>

#endif

size_t console_width();
size_t console_height();

std::string home_dir();
std::string app_dir();

std::string replace_home_dir(const std::string& str);

std::string get_hostname();
std::string get_username();

#endif //SYSTEMINFO_H_
