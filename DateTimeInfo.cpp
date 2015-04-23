#include "DateTimeInfo.h"

DateTimeInfo::DateTimeInfo()
{
    cDate = currentDate();
    cTime = currentTime();
    cTimezone = currentTimezone();
}


//returns current date
std::string currentDate()
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
std::string currentTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);

    return buf;
}

//returns current time zone
std::string currentTimezone()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Z", &tstruct);

    return buf;
}
