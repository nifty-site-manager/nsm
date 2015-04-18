#include "Filename.h"

Filename strippedExtension(const Filename &filename)
{
    int pos = filename.find_last_of('.');
    if(pos == std::string::npos)
        return filename;
    else
        return filename.substr(0, pos);
};
