#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "FileSystem.h"

bool save(const rapidjson::Document& doc, const Path& path);
std::string stringify(const rapidjson::Document& doc);
std::string minify(const rapidjson::Document& doc);