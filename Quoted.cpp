#include "Quoted.h"

//reading a string which may be surrounded by quotes with spaces, and strips surrounding quotes
//note mutates white space into single spaces
bool read_quoted(std::istream &is, std::string &s)
{
	//reads first string
	if(!(is >> s))
		return 0;

	std::string s2;
	if(s[0] == '"')
	{
		while(s[s.length()-1] != '"')
		{
			is >> s2;
			s += " " + s2;
		}

		s.replace(0, 1, "");
		s.replace(s.length()-1, 1, "");
	}
	else if(s[0] == '\'')
	{
		while(s[s.length()-1] != '\'')
		{
			is >> s2;
			s+=" " + s2;
		}

		s.replace(0, 1, "");
		s.replace(s.length()-1, 1, "");
	}

	return 1;
}

//outputting a string with quotes surrounded if it contains space(s)
std::string quote(const std::string &unquoted)
{
	if(unquoted == "")
		return "''";
	//checks if already quoted
	else if((unquoted.size() > 1 && unquoted[0] == '"' && unquoted[unquoted.size()-1] == '"') ||
	   (unquoted.size() > 1 && unquoted[0] == '\'' && unquoted[unquoted.size()-1] == '\''))
		return unquoted;
	/*else if(unquoted.find(' ') == std::string::npos)
	{
		for(size_t i=0; i<unquoted.size(); i++)
		{
			//quotes anything with special characters
			if(!std::isalnum(unquoted[i]))
			{
				#if defined _WIN32 || defined _WIN64
					if(unquoted.find('"') == std::string::npos)
						return "\"" + unquoted + "\"";
					else if(unquoted.find('\'') == std::string::npos)
						return "'" + unquoted + "'";
				#else
					if(unquoted.find('\'') == std::string::npos)
						return "'" + unquoted + "'";
					else if(unquoted.find('"') == std::string::npos)
						return "\"" + unquoted + "\"";
				#endif
				else
					return "'" + unquoted + "'";
			}
		}
		return unquoted;
	}*/
	#if defined _WIN32 || defined _WIN64
		else if(unquoted.find('"') == std::string::npos)
			return "\"" + unquoted + "\"";
		else if(unquoted.find('\'') == std::string::npos)
			return "'" + unquoted + "'";
	#else  //unix
		else if(unquoted.find('\'') == std::string::npos)
			return "'" + unquoted + "'";
		else if(unquoted.find('"') == std::string::npos)
			return "\"" + unquoted + "\"";
	#endif
	else
		return "'" + unquoted + "'";
}

//outputting a string with no quotes surrounded if it is quoted
std::string unquote(const std::string &quoted)
{
	if(quoted.size() > 1)
	{
		if(quoted[0] == '"' && quoted[quoted.size()-1] == '"')
			return quoted.substr(1, quoted.size()-2);
		else if(quoted[0] == '\'' && quoted[quoted.size()-1] == '\'')
			return quoted.substr(1, quoted.size()-2);
	}

	return quoted;
}

