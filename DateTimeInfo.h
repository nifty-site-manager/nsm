#ifndef DATE_TIME_INFO_H_
#define DATE_TIME_INFO_H_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>

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

	//returns current date
	std::string currentDate();

	//returns current UTC date
	std::string currentUTCDate();

	//returns current year (YYYY)
	std::string currentYYYY();

	//returns current year (YY)
	std::string currentYY();

	//returns current time
	std::string currentTime();

	//returns current UTC time
	std::string currentUTCTime();

	//returns current time zone
	std::string currentTimezone();

	//returns current operating system
    std::string currentOS();
};

#endif //DATE_TIME_INFO_H_
