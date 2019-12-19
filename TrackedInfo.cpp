#include "TrackedInfo.h"

Title get_title(const Name& name)
{
    if(name.find_last_of('/') == std::string::npos)
        return Title(name);

    return Title(name.substr(name.find_last_of('/') + 1, name.length() - name.find_last_of('/') - 1));
}

//uses name to order/compare TrackedInfo
bool operator<(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2)
{
    return (quote(tInfo1.name) < quote(tInfo2.name));
}

std::ostream& operator<<(std::ostream& os, const TrackedInfo& tInfo)
{
    os << quote(tInfo.name) << "\n";
    os << tInfo.title << "\n";
    os << tInfo.templatePath;

    return os;
}
