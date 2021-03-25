#include "Lolcat.h"


#if defined _WIN32 || defined _WIN64
	WinLolcatColor::WinLolcatColor(const int& Col)
	{
		col = Col;
	}

	WinLolcatColor::WinLolcatColor(const int& foreground, const int& background)
	{
		col = foreground + background*16;
	}

	std::ostream& operator<<(std::ostream& os, const WinLolcatColor& cc)
	{
		if(&os == &std::cout)
		{
			FlushConsoleInputBuffer(hConsole);
			SetConsoleTextAttribute(hConsole, cc.col);
		}

		return os;
	}

	int noColors = 9;
	std::vector<WinLolcatColor> colors = {WinLolcatColor(4),
	                                      WinLolcatColor(12),
	                                      WinLolcatColor(6),
	                                      WinLolcatColor(10),
	                                      WinLolcatColor(2),
	                                      WinLolcatColor(3),
	                                      WinLolcatColor(1),
	                                      WinLolcatColor(5),
	                                      WinLolcatColor(13)};
									  
	void lolcat_powershell()
	{
		noColors = 7;
		colors = std::vector<WinLolcatColor>{WinLolcatColor(12, 5),
											 WinLolcatColor(14, 5),
											 WinLolcatColor(10, 5),
											 WinLolcatColor(2, 5),
											 WinLolcatColor(3, 5),
											 WinLolcatColor(9, 5),
											 WinLolcatColor(13, 5)};
	}
#else
	const int noColors = 30;
	const std::string colors[] = {"\033[38;5;39m",
	                              "\033[38;5;38m",
	                              "\033[38;5;44m",
	                              "\033[38;5;43m",
	                              "\033[38;5;49m",
	                              "\033[38;5;48m",
	                              "\033[38;5;84m",
	                              "\033[38;5;83m",
	                              "\033[38;5;119m",
	                              "\033[38;5;118m",
	                              "\033[38;5;154m",
	                              "\033[38;5;148m",
	                              "\033[38;5;184m",
	                              "\033[38;5;178m",
	                              "\033[38;5;214m",
	                              "\033[38;5;208m",
	                              "\033[38;5;209m",
	                              "\033[38;5;203m",
	                              "\033[38;5;204m",
	                              "\033[38;5;198m",
	                              "\033[38;5;199m",
	                              "\033[38;5;163m",
	                              "\033[38;5;164m",
	                              "\033[38;5;128m",
	                              "\033[38;5;129m",
	                              "\033[38;5;93m",
	                              "\033[38;5;99m",
	                              "\033[38;5;63m",
	                              "\033[38;5;69m",
	                              "\033[38;5;33m"};
#endif

bool format = 1;
int tabWidth = 2;

int mod(const int& x, const int& m)
{
	int mod_val;
	if(x >= 0)
		return x%m;
	else
	{
		mod_val = (-x)%m;
		if(mod_val)
			return m-mod_val;
		else
			return 0;
	}
}

double gradient = 999;
size_t width, cWidth;

bool addLineNo = 0, zigzag = 0;
int posGrad = 1;

int lolcat(std::istream& is)
{
	std::string inLine;
	int color,
	    lineNo = 0,
	    r = rand()%noColors;

	if(gradient < 0)
		gradient = -gradient;

	while(!is.eof())
	{
		if(!getline(is, inLine))
			break;

		if(addLineNo)
			inLine = std::to_string(lineNo) + ": " + inLine;

		if(zigzag)
		{
			if(int(std::floor(lineNo/noColors))%2)
				color = r = (r+1)%noColors;
			else
				color = r = mod(r-1, noColors);
		}
		else
		{
			if(posGrad)
			{
				cWidth = (1-((lineNo+r)*gradient - std::floor((lineNo+r)*gradient))/1.0)*width;
				color = int((lineNo+r)*gradient)%noColors;
			}
			else
			{
				cWidth = (((lineNo+r)*gradient - std::floor((lineNo+r)*gradient))/1.0)*width;
				color = mod(-int((lineNo+r)*gradient), noColors); 
				//color = mod(noColors-int((lineNo+r)*gradient)%noColors, noColors); //alternative to above
			}
		}
			

		size_t i=0;
		std::cout << colors[color];

		while(i < inLine.size())
		{
			for(size_t I=0; I<width && i<inLine.size(); ++I)
			{
				if(format)
				{   //https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
					while(i < inLine.size() && (inLine[i] == '\\' || 
					                            inLine[i] == '\x0' || 
					                            //inLine[i] == "\e" ||
					                            inLine[i] == '\033' ||         // octag
					                            inLine.substr(i, 2) == "^[" || // ctrl key
					                            inLine[i] == '\x1b' ||         // hexadecimal
					                            inLine[i] == '\u001b'))        // unicode
					{
						int start = i++;
						while(i < inLine.size() && inLine[i] != ' ' && inLine[i] != '\n' && inLine[i] != '\t')
							++i;
						std::cout << inLine.substr(start, i-start);
					}
				}

				if(i < inLine.size())
					std::cout << inLine[i++];
			}

			if(width != cWidth)
				cWidth = width;
			color=(color+1)%noColors;
			std::cout << colors[color];
		}

		std::cout << std::endl;
		++lineNo;
	}
	
	std::cout << c_white << std::flush;

	return 0;
}

int lolfilter(std::istream &is)
{
	if(width == 0)
	{
		#if defined _WIN32 || defined _WIN64
			width = 7;
		#else
			width = 4;
		#endif
	}
	
	if(gradient != 999)
		return lolcat(is);
	else if(zigzag)
		return lolcat(is);
	#if defined _WIN32 || defined _WIN64
	#else
		else if(!(rand()%3))
		{
			zigzag = 1;
			return lolcat(is);
		}
	#endif
	else
	{
		gradient = 0.6;
		posGrad = rand()%2;
	}

	return lolcat(is);
}

int lolmain(const int& argc, const char* argv[])
{
srand(time(NULL));
	
	std::string param;
	for(int i=1; i<argc; i++)
	{
		param = argv[i];

		if(param == "-f")
			format = 1;
		else if(param.substr(0, 3) == "-g=")
		{
			gradient = std::strtod(param.substr(3, param.size()-3).c_str(), NULL);
			if(gradient < 0)
			{
				posGrad = 0;
				gradient = -gradient;
			}
		}
		else if(param == "-ln")
			addLineNo = 1;
		else if(param == "-ps")
		{
			#if defined _WIN32 || defined _WIN64
				lolcat_powershell();
			#endif
		}
		else if(param.substr(0, 4) == "-tw=")
			tabWidth = std::atoi(param.substr(4, param.size()-4).c_str());
		else if(param.substr(0, 3) == "-w=")
			width = std::atoi(param.substr(3, param.size()-3).c_str());
		else if(param == "-zz")
			zigzag = 1;
		else if(file_exists(param))
		{
			std::ifstream ifs(param);
			int ret_val = lolfilter(ifs);
			ifs.close();
			return ret_val;
		}
		else if(param == "-h" || param == "-help")
		{
			std::stringstream ss;

			ss << argv[0] << " is a cross-platform c++ implementation of the original lolcat,";
			ss << " which writes output to the terminal with rainbow colours" << std::endl;

			ss  << "=>" << " usage:" << std::endl;
			ss << " command | " << argv[0] << " (options)" << std::endl;
			ss << " " << argv[0] << " (options) file" << std::endl;

			ss << "=>" << " options:" << std::endl;
			ss << "      -f" << ": format" << std::endl;
			ss << "  -g=[d]" << ": set gradient" << std::endl;
			ss << "     -ln" << ": add line numbers" << std::endl;
			ss << "     -ps" << ": powershell colors" << std::endl;
			ss << " -tw=[i]" << ": set tab width" << std::endl;
			ss << "      -v" << ": print version" << std::endl;
			ss << "  -w=[i]" << ": set width" << std::endl;
			ss << "     -zz" << ": +/- gradient" << std::endl;

			return lolfilter(ss);
		}
		else if(param == "-v" || param == "-version")
		{
			std::stringstream ss;
			ss << "v1.0.0" << std::endl;

			return lolfilter(ss);
		}
		else
		{
			std::cout << "error: " << argv[0] << ": ";

			std::stringstream ss;
			ss << "do not recognise '" << param << "'" << std::endl;

			return lolfilter(ss);
		}
	}

	return lolfilter(std::cin);
}
