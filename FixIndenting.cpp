/*
	Program to ensure leading indenting is tabs when an editor messes that up

	WARNING: make a copy of your codebase before using this code to clean up the
	         leading indenting in case it wreaks havoc on your codebase.

	         ---> USE AT YOUR OWN RISK! <---

	example usage: 
		g++ FixIndenting.cpp -o FixIndenting; ./FixIndenting *.cc *.cpp *.h

	You can also set up `make fix-indenting` as is done in the Nift Makefile

	Copyright (c) 2015-present
	Creator: Nicholas Ham
	https://n-ham.com
*/

#include <cstdio>
#include <iostream>
#include <fstream>

std::string f_txt, path;
std::ifstream ifs;

inline bool string_from_file()
{
	ifs.open(path);
	if(!ifs.is_open())
		return 0;
	getline(ifs, f_txt, (char) ifs.eof());
	ifs.close();
	return 1;	
}

int main(int argc, const char* argv[])
{
	ifs.exceptions(std::ios::failbit | std::ios::badbit);

	for(size_t f=1; f<argc; ++f)
	{
		int line_no = 1;
		bool carryLine = 0;
		int li_count = 0;
		size_t c2;

		path = argv[f];
		if(!string_from_file())
		{
			std::cout << "error: " << argv[f] << ": file does not exist" << std::endl;
			return 0;
		}
		std::ofstream ofs(argv[f]);

		for(size_t c=0; c<f_txt.size(); ++c, ++line_no)
		{
			if(carryLine)
			{
				for(int li=0; li < li_count; ++li)
				{
					if(f_txt[c] == '\t')
						++c;
					else if(f_txt.substr(c, 4) == "    ")
						c += 4;
					else
					{
						std::cout << "error: " << argv[f] << ": " << line_no << ": carry line has the wrong indenting" << std::endl;
						std::cout << li_count << std::endl;
						return 1;
					}

					ofs << "\t";
				}
			}
			else
			{
				li_count = 0;
				while(1)
				{
					if(f_txt[c] == '\t')
						++c;
					else if(f_txt.substr(c, 4) == "    ")
						c += 4;
					else break;

					ofs << "\t";
					++li_count;
				}
			}

			ofs << f_txt.substr(c, (c2 = std::min(f_txt.size()-1, f_txt.find_first_of('\n', c))) - c) << std::endl;
			c = c2;
			if(c > 0 && f_txt[c-1] == ' ')
				--c2;
			carryLine = ((c2 > 0 && (f_txt[c2-1] == ',' || f_txt[c2-1] == '\\')) || 
			             (c2 > 1 && (f_txt.substr(c2-2, 2) == "&&" || f_txt.substr(c2-2, 2) == "||")));
		}

		ofs.close();
	}

	return 0;
}
