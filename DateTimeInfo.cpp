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

//returns current UTC date
std::string DateTimeInfo::currentUTCDate()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *gmtime(&now);
    //weekday month day year
    strftime(buf, sizeof(buf), "%A %B %d %Y", &tstruct);

    return buf;
}

//returns current date
std::string DateTimeInfo::currentYYYY()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    tstruct = *localtime(&now);
    //weekday month day year
    strftime(buf, sizeof(buf), "%Y", &tstruct);

    return buf;
}

//returns current date
std::string DateTimeInfo::currentYY()
{
    return currentYYYY().substr(2, 2);
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
    if(tstruct.tm_min < 10)
        ss << tstruct.tm_hour << ":0" << tstruct.tm_min;
    else
        ss << tstruct.tm_hour << ":" << tstruct.tm_min;
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
