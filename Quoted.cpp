#include "Quoted.h"

//reading a string which may be surrounded by quotes with spaces, and strips surrounding quotes
//note mutates white space into single spaces
bool read_quoted(std::istream &ifs, std::string &s)
{
    //reads first string
    if(!(ifs >> s))
        return 0;

    std::string s2;
    if(s[0] == '"')
    {
        while(s[s.length()-1] != '"')
        {
            ifs >> s2;
            s += " " + s2;
        }

        s.replace(0, 1, "");
        s.replace(s.length()-1, 1, "");
    }
    else if(s[0] == '\'')
    {
        while(s[s.length()-1] != '\'')
        {
            ifs >> s2;
            s+=" " + s2;
        }

        s.replace(0, 1, "");
        s.replace(s.length()-1, 1, "");
    }

    return 1;
};

//outputting a string with quotes surrounded if it contains space(s)
std::string quote(const std::string &unquoted)
{
    if(unquoted.find(' ') == std::string::npos)
        return unquoted;
    else if(unquoted.find('"') == std::string::npos)
        return "\"" + unquoted + "\"";
    else
        return "'" + unquoted + "'";
};

