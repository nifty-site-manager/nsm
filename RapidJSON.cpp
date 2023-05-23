#include "RapidJSON.h"

bool save(const rapidjson::Document& doc, const Path& path)
{
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    Path(path).ensureDirExists();
    std::ofstream ofs(path.str());

    ofs << sb.GetString();

    ofs.close();

    return 0;

}

std::string stringify(const rapidjson::Document& doc)
{   
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

std::string minify(const rapidjson::Document& doc)
{   
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}