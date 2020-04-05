#include "NumFns.h"

int intLength(int i)
{
	int length = 1;

	if(i < 0)
		i = -i;

	for(;i >= 10; i /= 10)
		++length;

	return length;
}

int llintLength(long long int i)
{
	int length = 1;

	if(i < 0)
		i = -i;

	for(;i >= 10; i /= 10)
		++length;

	return length;
}

bool isInt(const std::string& str)
{
    if(!str.size())
        return 0;

    if(str.size() && str[0] != '-' && !std::isdigit(str[0]))
        return 0;

    for(size_t i=1; i<str.size(); i++)
        if(!std::isdigit(str[i]))
            return 0;

    return 1;
}

bool isPosInt(const std::string& str)
{
    if(str.size() == 0 || str == "0")
        return 0;

    for(size_t i=0; i<str.size(); i++)
        if(!std::isdigit(str[i]))
            return 0;

    return 1;
}

bool isNonNegInt(const std::string& str)
{
    if(!str.size())
        return 0;

    for(size_t i=0; i<str.size(); i++)
        if(!std::isdigit(str[i]))
            return 0;

    return 1;
}

bool isDouble(const std::string& str)
{
	if(!str.size())
		return 0;

	bool founde = 0, founddp = 0;

	for(size_t i=0; i<str.size(); ++i)
	{
		if(str[i] == 'e')
		{
			if(founde)
				return 0;
			founde = 1;
			if(i+1 < str.size() && str[i+1] == '-')
				++i;
		}
		else if(str[i] == '-')
		{
			if(i > 0 && str[i-1] != 'e')
				return 0;
		}
		else if(str[i] == '+')
		{
			if(i > 0 && str[i-1] != 'e')
				return 0;
		}
		else if(str[i] == '.')
		{
			if(founde || founddp)
				return 0;
			founddp = 1;
		}
		else if(!std::isdigit(str[i]))
			return 0;
	}

	return 1;
}

/*returns a number specifying type
	0 == int
	1 == double
	2 == string
*/
int getTypeInt(const std::string& str)
{
	if(!str.size())
		return 2;

	int typeInt = 0;

	bool founde = 0, founddp = 0;

	for(size_t i=0; i<str.size(); ++i)
	{
		if(str[i] == 'e')
		{
			if(founde)
				return 2;
			else if(!typeInt)
				typeInt = 1;
			founde = 1;
			if(i+1 < str.size() && str[i+1] == '-')
				++i;
		}
		else if(str[i] == '-')
		{
			if(i > 0 && str[i-1] != 'e')
				return 2;
		}
		else if(str[i] == '+')
		{
			if(i > 0 && str[i-1] != 'e')
				return 0;
		}
		else if(str[i] == '.')
		{
			if(founde || founddp)
				return 2;
			else if(!typeInt)
				typeInt = 1;
			founddp = 1;
		}
		else if(!std::isdigit(str[i]))
			return 2;
	}

	return typeInt;
}

std::string int_to_timestr(int s)
{
    if(s < 0)
        return "-" + int_to_timestr(-s);
	else if(s < 60)
		return std::to_string(s) + "s";
	else if(s < 3600)
	{
		int m = s/60;
		s -= 60*m;
		return std::to_string(m) + "m" + int_to_timestr(s);
	}
	else if(s < 86400)
	{
		int h = s/3600;
		s -= 3600*h;
		int m = s/60;
		return std::to_string(h) + "h" + std::to_string(m) + "m";
	}
	else //if(s < 604800)
	{
		int d = s/86400;
		s -= 86400*d;
		int h = s/3600;
		s -= 3600*h;
		int m = s/60;
		return std::to_string(d) + "d" + std::to_string(h) + "h" + std::to_string(m) + "m";
	}
}

std::string double_to_string_prec(const double& d, const int& prec)
{
    std::ostringstream oss;
    oss << std::setprecision(prec) << d;
    return oss.str();
}

