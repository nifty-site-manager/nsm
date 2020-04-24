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

bool replaceTabs = 0;
int tabWidth = 4;
std::string replace_tabs(const std::string& str)
{
	std::string replacedStr;
	for(size_t i=0; i<str.size(); ++i)
	{
		if(str[i] == '\t')
			replacedStr += std::string(tabWidth, ' ');
		else
			replacedStr += str[i];
	}
	return replacedStr;
}

int mod(const int& x, const int& m)
{
	if(x >= 0)
		return x%m;
	else
		return mod(m+x, m);
}

double gradient = 999;
int width = 0;

bool addLineNo = 0, zigzag = 0;
int posGrad = 1;

int zigzagcat(std::istream& is)
{
	std::string inLine;
	int color,
	    lineNo = 0,
	    sColor = rand()%noColors;
	while(!is.eof())
	{
		if(!getline(is, inLine))
			break;

		if(replaceTabs)
			inLine = replace_tabs(inLine);

		if(addLineNo)
			inLine = std::to_string(lineNo) + ": " + inLine;
		if(int(std::floor(lineNo/noColors))%2)
			color = sColor = (sColor+1)%noColors;
		else
			color = sColor = mod(sColor-1, noColors);

		for(size_t i=0; i<inLine.size(); color=(color+1)%noColors, i+=width)
			std::cout << colors[color] << inLine.substr(i, width);
		std::cout << std::endl;

		++lineNo;
	}

	std::cout << c_white << std::flush;

	return 0;
}

int lolcat(std::istream& is)
{
	std::string inLine;
	int color,
	    lineNo = 0,
	    r = rand()%noColors,
		sWidth;
	while(!is.eof())
	{
		if(!getline(is, inLine))
			break;

		if(replaceTabs)
			inLine = replace_tabs(inLine);

		if(addLineNo)
			inLine = std::to_string(lineNo) + ": " + inLine;
		if(posGrad)
			color = int((lineNo+r)*gradient)%noColors;
		else
			//color = mod(-int((lineNo+r)*gradient), noColors); //this is too slow
			color = mod(noColors-int((lineNo+r)*gradient)%noColors, noColors);

		size_t i=0;
		if(posGrad)
			sWidth = (1-((lineNo+r)*gradient - std::floor((lineNo+r)*gradient))/1.0)*width;
		else
			sWidth = (((lineNo+r)*gradient - std::floor((lineNo+r)*gradient))/1.0)*width;
		std::cout << colors[color] << inLine.substr(i, sWidth);
		color=(color+1)%noColors;
		i += sWidth;
		for(; i<inLine.size(); color=(color+1)%noColors, i+=width)
			std::cout << colors[color] << inLine.substr(i, width);
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
		return zigzagcat(is);
	#if defined _WIN32 || defined _WIN64
	#else
		else if(!(rand()%3))
			return zigzagcat(is);
	#endif

	#if defined _WIN32 || defined _WIN64
		gradient = 0.35;
		posGrad = rand()%2;
	#else
		gradient = 0.6;
		posGrad = rand()%2;
	#endif

	return lolcat(is);
}

int lolmain(const int& argc, const char* argv[])
{
	srand(time(NULL));
	
	std::string param;
	for(int i=2; i<argc; i++)
	{
		param = argv[i];
		if(param.substr(0, 3) == "-g=")
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
				use_powershell_colours();
			#endif
		}
		else if(param == "-t")
			replaceTabs = 1;
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
			ss << " -g=[d]" << ": set gradient" << std::endl;
			ss << "    -ln" << ": add line numbers" << std::endl;
			ss << "    -ps" << ": powershell colors" << std::endl;
			ss << "      -t" << ": replace tabs" << std::endl;
			ss << " -tw=[i]" << ": set tab width" << std::endl;
			ss << "     -v" << ": print version" << std::endl;
			ss << " -w=[i]" << ": set width" << std::endl;
			ss << "    -zz" << ": +/- gradient" << std::endl;

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
			std::cout << "error: " << argv[0] << " lolcat: ";

			std::stringstream ss;
			ss << "do not recognise '" << param << "'" << std::endl;

			return lolfilter(ss);
		}
	}

	return lolfilter(std::cin);
}
