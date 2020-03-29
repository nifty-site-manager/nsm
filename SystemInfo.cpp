#include "SystemInfo.h"

#if defined _WIN32 || defined _WIN64
	#define INFO_BUFFER_SIZE 32767

	size_t console_width()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return csbi.srWindow.Right - csbi.srWindow.Left + 1;
	}

	size_t console_height()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}

	std::string home_dir()
	{
		char path[MAX_PATH];

		if(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path) != S_OK)
			return "#error";
		
		return path;
	}

	std::string app_dir()
	{
		char path[MAX_PATH];

		if(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK)
			return "#error";
		
		return path;
	}

	std::string get_hostname()
	{
		TCHAR  infoBuf[INFO_BUFFER_SIZE];
		DWORD  bufCharCount = INFO_BUFFER_SIZE;
	 
		if(!GetComputerName(infoBuf, &bufCharCount))
		    return "unknown";
	 
		return infoBuf;
	}
	 
	std::string get_username()
	{
		TCHAR  infoBuf[INFO_BUFFER_SIZE];
		DWORD  bufCharCount = INFO_BUFFER_SIZE;
	 
		if(!GetUserName(infoBuf, &bufCharCount))
		    return "unknown";
	 
		return infoBuf;
	}
#else  //*nix
	size_t console_width()
	{
		struct winsize size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

		return size.ws_col;
	}

	size_t console_height()
	{
		struct winsize size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

		return size.ws_row;
	}

	struct termios orig_termios;
	void disable_raw_mode() 
	{
		//system("stty cooked"); //backup option
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
		//should we flush here std::fflush(stdout) or std::cout << std::flush?
	}

	//https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
	void enable_raw_mode() 
	{
		//system("stty raw"); //backup option
		tcgetattr(STDIN_FILENO, &orig_termios);
		atexit(disable_raw_mode);
		struct termios raw = orig_termios;
		raw.c_lflag &= ~(ECHO | ICANON);
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
		//should we flush here std::fflush(stdout)?
	}

	std::string home_dir()
	{
		const char *homedir;

		if((homedir = getenv("HOME")) == NULL)
			homedir = getpwuid(getuid())->pw_dir;

		/*#if defined __FreeBSD__
			if(!dir_exists(std::string(homedir)))
				homedir = ("/usr" + std::string(homedir)).c_str();
		#endif*/

		return homedir;
	}

	std::string app_dir()
	{
		return home_dir();
	}

	const int NAME_MAXX = 64;

	std::string get_hostname()
	{
		/*char hostname[HOST_NAME_MAX];
		gethostname(hostname, HOST_NAME_MAX);*/

		char hostname[NAME_MAXX];
		gethostname(hostname, NAME_MAXX);

		return hostname;
	}

	std::string get_username()
	{
		/*char username[LOGIN_NAME_MAX];
		getlogin_r(username, LOGIN_NAME_MAX);*/

		char username[NAME_MAXX];
		getlogin_r(username, NAME_MAXX);

		return username;
	}
#endif

std::string replace_home_dir(std::string str)
{
	if(str.size() && str[0] == '~')
		return home_dir() + str.substr(1, str.size()-1);
	else if(str.substr(0, 13) == "%userprofile%")
		return home_dir() + str.substr(13, str.size()-13);
	else
		return str;
}
