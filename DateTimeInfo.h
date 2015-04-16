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

struct DateTimeInfo
{
    std::string cDate,
        cTime,
        cTimezone;

    DateTimeInfo();
};

//returns current date
std::string currentDate();

//returns current time
std::string currentTime();

//returns current time zone
std::string currentTimezone();

#endif //DATE_TIME_INFO_H_
