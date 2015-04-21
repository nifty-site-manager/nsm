#include "Directory.h"

//removes ./ from the front if it's there
Directory comparable(const Directory &directory)
{
    if(directory.substr(0, 2) != "./")
        return directory;
    else
        return directory.substr(2, directory.length()-2);
};

//returns the leading directory if present
Directory leadingDir(const Directory &directory)
{
    int pos = directory.find_first_of('/');
    if(pos == std::string::npos)
        return "";
    else
        return directory.substr(0, pos+1);
};

Directory stripLeadingDir(const Directory &directory)
{
    int pos = directory.find_first_of('/');
    if(pos == std::string::npos)
        return "";
    else
    {
        pos++;
        return directory.substr(pos, directory.length()-pos);
    }
};

std::deque<Directory> dirDeque(const Directory &directory)
{
    Directory pwd = comparable(directory);
    std::deque<std::string> dDeque;

    while(pwd.size())
    {
        dDeque.push_back(leadingDir(pwd));
        pwd = stripLeadingDir(pwd);
    }

    return dDeque;
};

std::string pathBetween(const Directory &sourceDir, const Directory &targetDir)
{
    std::deque<std::string> sourceDeque = dirDeque(sourceDir),
        targetDeque = dirDeque(targetDir);

    while(sourceDeque.size() > 0 && targetDeque.size() > 0 && sourceDeque[0] == targetDeque[0])
    {
        sourceDeque.pop_front();
        targetDeque.pop_front();
    }

    std::string sourceToTarget = "";

    for(auto s=0; s<sourceDeque.size(); s++)
        sourceToTarget += "../";
    for(auto t=0; t<targetDeque.size(); t++)
        sourceToTarget += targetDeque[t];

    return sourceToTarget;
};
