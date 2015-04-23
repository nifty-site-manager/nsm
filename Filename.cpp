#include "Filename.h"

Filename strippedExtension(const Filename &filename)
{
    size_t pos = filename.find_last_of('.');
    if(pos == std::string::npos)
        return filename;
    else
        return filename.substr(0, pos);
}
