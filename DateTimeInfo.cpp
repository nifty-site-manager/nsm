#include "DateTimeInfo.h"

DateTimeInfo::DateTimeInfo()
{
    cDate = currentDate();
    cTime = currentTime();
    cTimezone = currentTimezone();
}


//returns current date
std::string DateTimeInfo::currentDate()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    //weekday month day year
    strftime(buf, sizeof(buf), "%A %B %d %Y", &tstruct);

    return buf;
}

//returns current time
std::string DateTimeInfo::currentTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);

    return buf;
}

//returns current UTC time
std::string DateTimeInfo::currentUTCTime()
{
    time_t now = time(0);
    struct tm tstruct;

    tstruct = *gmtime(&now);
    std::stringstream ss;
    ss << (tstruct.tm_hour+10)%24 << ":" << tstruct.tm_min;
    return ss.str();
}

//returns current time zone
std::string DateTimeInfo::currentTimezone()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Z", &tstruct);

    return buf;
}
