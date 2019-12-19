#ifndef TRACKED_INFO_H_
#define TRACKED_INFO_H_

#include "Path.h"
#include "Title.h"
#include "Quoted.h"

typedef std::string Name;

Title get_title(const Name& name);

struct TrackedInfo
{
    Title title;
    Name name;
    Path outputPath,
        contentPath,
        templatePath;
};

//output fn
std::ostream& operator<<(std::ostream& os, const TrackedInfo& tInfo);

//uses name to order/compare TrackedInfo
bool operator<(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2);

#endif //TRACKED_INFO_H_
