#include "TrackedInfo.h"

Title get_title(const Name& name)
{
	if(name.find_last_of('/') == std::string::npos && name.find_last_of('\\') == std::string::npos)
		return Title(name);
	else if(name == "/" || name == "\\")
		return Title("index");
	else if(name[name.size()-1] == '/' || name[name.size()-1] == '\\')
		return Title(name.substr(0, name.size()-1));
	else
		return Title(name.substr(name.find_last_of('/') + 1, name.length() - name.find_last_of('/') - 1));
}

//uses name to order/compare TrackedInfo
bool operator<(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2)
{
	return (quote(tInfo1.name) < quote(tInfo2.name));
}

bool operator==(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2)
{
	return (quote(tInfo1.name) == quote(tInfo2.name));
}

std::ostream& operator<<(std::ostream& os, const TrackedInfo& tInfo)
{
	os << quote(tInfo.name) << "\n";
	os << tInfo.title << "\n";
	os << tInfo.templatePath;

	return os;
}

std::string& operator<<(std::string& str, const TrackedInfo& tInfo)
{
	str += quote(tInfo.name) + "\n";
	str += tInfo.title.str + "\n";
	str += quote(tInfo.templatePath.str());

	return str;
}

std::unordered_set<std::string>::hasher HashFn = std::unordered_set<std::string>().hash_function();
int TrackedInfoIndex::operator()(const TrackedInfo &trackedInfo) const
{
	return HashFn(trackedInfo.name);
}
