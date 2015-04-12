#ifndef DATE_TIME_INFO_H_
#define DATE_TIME_INFO_H_

#include <cstdio>
#include <iostream>
#include <string>

//information about date and time
/*
    see http://en.cppreference.com/w/cpp/chrono/c/strftime for more
    options when returning information about the time and date.
*/

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


struct DateTimeInfo
{
    std::string cDate,
        cTime,
        cTimezone;

    DateTimeInfo()
    {
        cDate = currentDate();
        cTime = currentTime();
        cTimezone = currentTimezone();
    }
};

#endif //DATE_TIME_INFO_H_
