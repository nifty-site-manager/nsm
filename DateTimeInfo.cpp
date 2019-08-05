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
    if(tstruct.tm_hour < 10)
        ss << "0";
    ss << tstruct.tm_hour << ":";
    if(tstruct.tm_min < 10)
        ss << "0";
    ss << tstruct.tm_min << ":";
    if(tstruct.tm_sec < 10)
        ss << "0";
    ss << tstruct.tm_sec;

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

//returns current operating system
std::string DateTimeInfo::currentOS()
{
    #ifdef _WIN32
        return "Windows";
    #elif _WIN64
        return "Windows"
    #elif __APPLE__ //osx
        return "Macintosh";
    #elif __linux__
        return "Linux";
    #else  //unix
        return "Unix";
    #endif
}
