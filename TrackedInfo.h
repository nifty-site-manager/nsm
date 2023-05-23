#ifndef TRACKED_INFO_H_
#define TRACKED_INFO_H_

#include <unordered_set>

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
	std::string contentExt, outputExt, scriptExt;
};

//uses name to order/compare TrackedInfo
bool operator<(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2);
bool operator==(const TrackedInfo& tInfo1, const TrackedInfo& tInfo2);

//output fn
std::ostream& operator<<(std::ostream& os, const TrackedInfo& tInfo);
std::string& operator<<(std::string& str, const TrackedInfo& tInfo);

//std::unordered_set<std::string>::hasher HashFn = std::unordered_set<std::string>().hash_function();
struct TrackedInfoIndex
{
	int operator()(const TrackedInfo &trackedInfo) const;
};

/*struct TrackedInfoIndex
{
	int operator()(const TrackedInfo &trackedInfo) const
	{
		return std::atoi(trackedInfo.name.c_str());
		//return trackedInfo.name;
	};
};*/


#endif //TRACKED_INFO_H_
