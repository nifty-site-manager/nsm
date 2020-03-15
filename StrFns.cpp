#include "StrFns.h"

bool is_whitespace(const std::string& str)
{
    for(size_t i=0; i<str.size(); i++)
        if(str[i] != ' ' && str[i] != '\t')
            return 0;

    return 1;
}

std::string into_whitespace(const std::string& str)
{
    std::string whitespace = "";

    for(size_t i=0; i<str.size(); i++)
    {
        if(str[i] == '\t')
            whitespace += "\t";
        else
            whitespace += " ";
    }

    return whitespace;
}

void strip_leading_line(std::string& str)
{
	size_t pos = str.find_first_of('\n');

	if(pos != std::string::npos)
		str = str.substr(pos+1, str.size()-pos+1);
	else
		str = "";
}

void strip_trailing_line(std::string& str)
{
	size_t pos = str.find_last_of('\n');

	if(pos != std::string::npos)
		str = str.substr(0, pos);
	else
		str = "";
}

void strip_leading_whitespace(std::string& str)
{
    size_t pos=0;

    for(; pos<str.size(); ++pos)
        if(str[pos] != ' ' && str[pos] != '\t')
            break;

    str = str.substr(pos, str.size()-pos);
}

void strip_leading_whitespace_multiline(std::string& str)
{
    size_t pos=0;

    for(; pos<str.size(); ++pos)
        if(str[pos] != ' ' && str[pos] != '\t' && str[pos] != '\n')
            break;

    str = str.substr(pos, str.size()-pos);
}

void strip_trailing_whitespace(std::string& str)
{
    int pos=str.size()-1;

    for(; pos>=0; pos--)
        if(str[pos] != ' ' && str[pos] != '\t')
            break;

    str = str.substr(0, pos+1);
}

void strip_trailing_whitespace_multiline(std::string& str)
{
    int pos=str.size()-1;

    for(; pos>=0; pos--)
        if(str[pos] != ' ' && str[pos] != '\t' && str[pos] != '\n')
            break;

    str = str.substr(0, pos+1);
}

void strip_surrounding_whitespace(std::string& str)
{
    size_t spos=0, epos=str.size()-1;

    for(; spos<str.size(); ++spos)
        if(str[spos] != ' ' && str[spos] != '\t')
            break;

    if(spos > epos)
        str = "";
    else
    {
        for(; spos < epos; --epos)
            if(str[epos] != ' ' && str[epos] != '\t')
                break;

        str = str.substr(spos, 1+epos-spos);
    }
}

void strip_surrounding_whitespace_multiline(std::string& str)
{
    size_t spos=0, epos=str.size()-1;

    for(; spos<str.size(); ++spos)
        if(str[spos] != ' ' && str[spos] != '\t' && str[spos] != '\n')
            break;

    if(spos > epos)
        str = "";
    else
    {
        for(; spos < epos; --epos)
            if(str[epos] != ' ' && str[epos] != '\t' && str[epos] != '\n')
                break;

        str = str.substr(spos, 1+epos-spos);
    }
}

std::string join(const std::vector<std::string>& vec, const std::string& str)
{
    std::string ans = "";

    if(vec.size())
    {
        ans += vec[0];
        for(size_t v=1; v<vec.size(); v++)
            ans += str + vec[v];
    }

    return ans;
}

std::string findAndReplaceAll(const std::string& orig, const std::string& toSearch, const std::string& replaceStr)
{
	std::string data = orig;

	// Get the first occurrence
	size_t pos = data.find(toSearch);

	// Repeat till end is reached
	while( pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos =data.find(toSearch, pos + replaceStr.size());
	}

	return data;
}
