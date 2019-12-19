#include "Parser.h"

std::atomic<long long int> sys_counter;

bool is_whitespace(const std::string& str)
{
    for(size_t i=0; i<str.size(); i++)
        if(str[i] != ' ' && str[i] != '\t')
            return 0;

    return 1;
}

std::string into_whitespace(const std::string& str)
{
    std::string whitespace = "";

    for(size_t i=0; i<str.size(); i++)
    {
        if(str[i] == '\t')
            whitespace += "\t";
        else
            whitespace += " ";
    }

    return whitespace;
}

void strip_trailing_whitespace(std::string& str)
{
    int pos=str.size()-1;

    for(; pos>=0; pos--)
        if(str[pos] != ' ' && str[pos] != '\t')
            break;

    str = str.substr(0, pos+1);
}

bool run_script(std::ostream& os, const std::string& scriptPathStr, const bool& makeBackup, std::mutex* os_mtx)
{
    if(std::ifstream(scriptPathStr))
    {
        int c = sys_counter++;
        size_t pos = scriptPathStr.substr(1, scriptPathStr.size()-1).find_first_of('.');
        std::string cScriptExt = "";
        if(pos != std::string::npos)
            cScriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
        std::string execPath = "./.script" + std::to_string(c) + cScriptExt;
        std::string output_filename = ".script.out" + std::to_string(c);
        //std::string output_filename = scriptPathStr + ".out" + std::to_string(c);
        int result;

        #if defined _WIN32 || defined _WIN64
            if(unquote(execPath).substr(0, 2) == "./")
                execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
        #else  //unix
        #endif

        //copies script to backup location
        if(makeBackup)
        {
            if(cpFile(scriptPathStr, scriptPathStr + ".backup"))
            {
                os_mtx->lock();
                os << "error: Parser.cpp: run_script(" << quote(scriptPathStr) << "): failed to copy " << quote(scriptPathStr) << " to " << quote(scriptPathStr + ".backup") << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        //moves script to main directory
        //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
        //sometimes this fails (on Windows) for some reason, so keeps trying until successful
        int mcount = 0;
        while(rename(scriptPathStr.c_str(), execPath.c_str()))
        {
            if(++mcount == 100)
            {
                os_mtx->lock();
                os << "warning: Parser.cpp: run_script(" << quote(scriptPathStr) << "): have tried to move " << quote(scriptPathStr) << " to " << quote(execPath) << " 100 times already, may need to abort" << std::endl;
                os_mtx->unlock();
            }
        }

        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
            result = system(("flatpak-spawn --host bash -c " + quote(execPath) + " > " + output_filename).c_str());
        else
            result = system((execPath + " > " + output_filename).c_str());

        //moves script back to its original location
        //sometimes this fails (on Windows) for some reason, so keeps trying until successful
        mcount = 0;
        while(rename(execPath.c_str(), scriptPathStr.c_str()))
        {
            if(++mcount == 100)
            {
                os_mtx->lock();
                os << "warning: Parser.cpp: run_script(" << quote(scriptPathStr) << "): have tried to move " << quote(execPath) << " to " << quote(scriptPathStr) << " 100 times already, may need to abort" << std::endl;
                os_mtx->unlock();
            }
        }

        //deletes backup copy
        if(makeBackup)
            remove_file(Path("", scriptPathStr + ".backup"));

        std::ifstream ifxs(output_filename);
        std::string str;
        os_mtx->lock();
        while(getline(ifxs, str))
            os << str << std::endl;
        os_mtx->unlock();
        ifxs.close();
        remove_file(Path("./", output_filename));

        if(result)
        {
            os_mtx->lock();
            os << "error: Parser.cpp: run_script(" << quote(scriptPathStr) << "): script failed" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    return 0;
}

Parser::Parser(std::set<TrackedInfo>* TrackedAll,
                         std::mutex* OS_mtx,
                         const Directory& ContentDir,
                         const Directory& OutputDir,
                         const std::string& ContentExt,
                         const std::string& OutputExt,
                         const std::string& ScriptExt,
                         const Path& DefaultTemplate,
                         const bool& makeBackup,
                         const std::string& UnixTextEditor,
                         const std::string& WinTextEditor)
{
    //sys_counter = 0;
    trackedAll = TrackedAll;
    os_mtx = OS_mtx;
    contentDir = ContentDir;
    outputDir = OutputDir;
    contentExt = ContentExt;
    outputExt = OutputExt;
    scriptExt = ScriptExt;
    defaultTemplate = DefaultTemplate;
    backupScripts = makeBackup;
    unixTextEditor = UnixTextEditor;
    winTextEditor = WinTextEditor;
}

int Parser::build(const TrackedInfo& ToBuild, std::ostream& os)
{
    sys_counter = sys_counter%1000000000000000000;
    toBuild = ToBuild;

    //ensures content and template files exist
    if(!std::ifstream(toBuild.contentPath.str()))
    {
        os_mtx->lock();
        os << "error: cannot build " << toBuild.outputPath << " as content file " << toBuild.contentPath << " does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    if(!std::ifstream(toBuild.templatePath.str()))
    {
        os_mtx->lock();
        os << "error: cannot build " << toBuild.outputPath << " as template file " << toBuild.templatePath << " does not exist." << std::endl;
        os_mtx->unlock();
        return 1;
    }

    depFiles.clear();

    //checks for non-default script extension
    Path extPath = toBuild.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

    std::string cScriptExt;
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, cScriptExt);
        ifs.close();
    }
    else
        cScriptExt = scriptExt;

    //checks for pre-build scripts
    if(std::ifstream("pre-build" + scriptExt))
        depFiles.insert(Path("", "pre-build" + scriptExt));
    Path prebuildScript = toBuild.contentPath;
    prebuildScript.file = prebuildScript.file.substr(0, prebuildScript.file.find_first_of('.')) + "-pre-build" + cScriptExt;
    if(std::ifstream(prebuildScript.str()))
        depFiles.insert(prebuildScript);
    if(run_script(os, prebuildScript.str(), backupScripts, os_mtx))
        return 1;

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    parsedText.clear();
    parsedText.str(std::string());
    strings.clear();
    contentAdded = 0;

    //adds content and template paths to dependencies
    depFiles.insert(toBuild.contentPath);
    depFiles.insert(toBuild.templatePath);

    //opens up template file to start parsing from
    std::ifstream ifs(toBuild.templatePath.str());

    //creates anti-deps of template set
    std::set<Path> antiDepsOfReadPath;

    //starts read_and_process from templatePath
    int result = read_and_process(1, ifs, toBuild.templatePath, antiDepsOfReadPath, parsedText, os);

    ifs.close();

    if(result == 0)
    {
        //ensures @inputcontent was found inside template dag
        if(!contentAdded)
        {
            os_mtx->lock();
            os << "error: content file " << toBuild.contentPath << " has not been used as a dependency within template file " << toBuild.templatePath << " or any of its dependencies" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //makes sure directory for output file exists
        toBuild.outputPath.ensureDirExists();

        //makes sure we can write to output file
        chmod(toBuild.outputPath.str().c_str(), 0644);

        //writes processed text to output file
        std::ofstream outputFile(toBuild.outputPath.str());
        outputFile << parsedText.str() << "\n";
        outputFile.close();

        //makes sure user can't accidentally write to output file
        chmod(toBuild.outputPath.str().c_str(), 0444);

        //checks for post-build scripts
        if(std::ifstream("post-build" + scriptExt))
            depFiles.insert(Path("", "post-build" + scriptExt));
        Path postbuildScript = toBuild.contentPath;
        postbuildScript.file = postbuildScript.file.substr(0, postbuildScript.file.find_first_of('.')) + "-post-build" + cScriptExt;
        if(std::ifstream(postbuildScript.str()))
            depFiles.insert(postbuildScript);
        if(run_script(os, postbuildScript.str(), backupScripts, os_mtx))
            return 1; //should an output file be listed as failing to build if the post-build script fails?

        //gets path for storing info file
        Path infoPath = toBuild.outputPath.getInfoPath();

        //makes sure info file exists
        infoPath.ensureDirExists();

        //makes sure we can write to info file_
        chmod(infoPath.str().c_str(), 0644);

        //writes info file
        std::ofstream infoStream(infoPath.str());
        infoStream << dateTimeInfo.currentTime() << " " << dateTimeInfo.currentDate() << "\n";
        infoStream << this->toBuild << "\n\n";
        for(auto depFile=depFiles.begin(); depFile != depFiles.end(); depFile++)
            infoStream << *depFile << "\n";
        infoStream.close();

        //makes sure user can't accidentally write to info file
        chmod(infoPath.str().c_str(), 0444);
    }

    return result;
}

//parses istream 'is' whilst writing processed version to ostream 'os' with error ostream 'eos'
int Parser::read_and_process(const bool& indent,
                                  std::istream& is,
                                  const Path& readPath,
                                  std::set<Path> antiDepsOfReadPath,
                                  std::ostream& os,
                                  std::ostream& eos)
{
    //adds read path to anti dependencies of read path
    if(std::ifstream(readPath.str()))
        antiDepsOfReadPath.insert(readPath);

    std::string baseIndentAmount = indentAmount;
    std::string beforePreBaseIndentAmount;
    if(!indent) // not sure if this is needed?
        baseIndentAmount = "";
    int baseCodeBlockDepth = codeBlockDepth;

    int lineNo = 0,
        openCodeLineNo = 0;
    bool firstLine = 1, lastLine = 0;
    std::string inLine;
    while(getline(is, inLine))
    {
        lineNo++;

        while(inLine == "@---")
        {
            int openLine = lineNo;
            std::stringstream ss;
            std::ostringstream oss;

            while(getline(is, inLine))
            {
                lineNo++;
                if(inLine == "@---")
                    break;

                ss << inLine << "\n";
            }

            if(inLine != "@---")
            {
                os_mtx->lock();
                eos << "error: " << readPath << ": lineNo " << openLine << ": open comment line @--- has no close line @--- (make sure there is no leading/trailing whitespace)" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            //parses comment stringstream
            std::string oldIndent = indentAmount;
            indentAmount = "";

            //readPath << ": line " << lineNo << ":
            if(read_and_process(0, ss, Path("", "special multi-line parsed comment"), antiDepsOfReadPath, oss, eos))
            {
                os_mtx->lock();
                eos << "error: " << readPath << ": lineNo " << openLine << ": @/* comment @*/: read_and_process() failed here" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            indentAmount = oldIndent;

            if(!getline(is, inLine))
                lastLine = 1;
            lineNo++;
        }

        if(lineNo > 1 && !firstLine && !lastLine) //could check !is.eof() rather than last line (trying to be clear)
        {
            indentAmount = baseIndentAmount;
            /*if(codeBlockDepth)
                os << "\n";
            else*/
            os << "\n" << baseIndentAmount;
        }
        firstLine = 0;

        for(size_t linePos=0; linePos<inLine.length();)
        {
            if(inLine.substr(linePos, 2) == "\\@") //checks whether to escape
            {
                os << "@";
                linePos += 2;
                if(indent)
                    indentAmount += " ";
            }
            else if(inLine[linePos] == '<') //checks about code blocks and html comments opening
            {
                //checks whether we're going down code block depth
                if(htmlCommentDepth == 0 && inLine.substr(linePos+1, 4) == "/pre")
                {
                    codeBlockDepth--;
                    if(codeBlockDepth == 0)
                        baseIndentAmount = beforePreBaseIndentAmount;
                    if(codeBlockDepth < baseCodeBlockDepth)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                if(inLine.substr(linePos, 4) == "<@--") //checks for raw multi-line comment
                {
                    linePos += 4;
                    int openLine = lineNo;

                    size_t endPos = inLine.find("--@>");
                    while(endPos == std::string::npos)
                    {
                        if(!getline(is, inLine))
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": lineNo " << openLine << ": open comment <@-- has no close --@>" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        lineNo++;
                        endPos = inLine.find("--@>");
                    }

                    linePos = endPos + 3; // linePos is incemented again further below (a bit gross!)
                }//else checks whether to escape <
                else if(codeBlockDepth > 0 && inLine.substr(linePos+1, 4) != "code" && inLine.substr(linePos+1, 5) != "/code")
                {
                    os << "&lt;";
                    if(indent)
                        indentAmount += into_whitespace("&lt;");
                }
                else
                {
                    os << '<';
                    if(indent)
                        indentAmount += " ";
                }

                //checks whether we're going up code block depth
                if(htmlCommentDepth == 0 && inLine.substr(linePos+1, 3) == "pre")
                {
                    if(codeBlockDepth == 0)
                    {
                        beforePreBaseIndentAmount = baseIndentAmount;
                        baseIndentAmount = "";
                    }
                    if(codeBlockDepth == baseCodeBlockDepth)
                        openCodeLineNo = lineNo;
                    codeBlockDepth++;
                }

                //checks whether we're going up html comment depth
                if(inLine.substr(linePos+1, 3) == "!--")
                    htmlCommentDepth++;

                linePos++;
            }
            else if(inLine[linePos] == '-') //checks about html comments closing
            {
                //checks whether we're going down html depth
                if(inLine.substr(linePos+1, 2) == "->")
                    htmlCommentDepth--;

                if(inLine.substr(linePos, 4) == "--@>")
                {
                    os_mtx->lock();
                    eos << "error: " << readPath << ": lineNo " << lineNo << ": close comment --@> has no open <@--" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }

                os << '-';
                if(indent)
                    indentAmount += " ";
                linePos++;
            }
            else if(inLine[linePos] == '@') //checks for commands
            {
                /*if(linePos > 0 && inLine[linePos-1] == '\\') //this doesn't work, still outputs escape character!
                                                               //plus it breaks @\\@\\ etc.
                {
                    os << '@';
                    if(indent)
                        indentAmount += " ";
                    linePos++;
                }*/
                if(inLine.substr(linePos, 10) == "@pagetitle") //can delete this later
                {
                    os << toBuild.title.str;
                    if(indent)
                        indentAmount += into_whitespace(toBuild.title.str);
                    linePos += std::string("@pagetitle").length();
                }
                else if(inLine.substr(linePos, 2) == "@{" ||
                        inLine.substr(linePos, 3) == "@*{" ||
                        inLine.substr(linePos, 2) == "@[" ||
                        inLine.substr(linePos, 3) == "@*[")
                {
                    linePos+=std::string("@").length();
                    int sLinePos = linePos;
                    std::string varName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_param(varName, linePos, inLine, readPath, lineNo, "@<..>", eos))
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(varName);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "@[variable name]"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        varName = oss.str();

                        indentAmount = oldIndent;
                    }

                    if(strings.count(varName)) //should this go after hard-coded constants? should we parse the string when printing?
                    {
                        std::istringstream iss(strings[varName]);
                        std::string ssLine, oldLine;
                        int ssLineNo = 0;

                        while(getline(iss, ssLine))
                        {
                            if(0 < ssLineNo++)
                                os << "\n" << indentAmount;
                            oldLine = ssLine;
                            os << ssLine;
                        }
                        indentAmount += into_whitespace(oldLine);
                    }
                    else if(varName == "title")
                    {
                        os << toBuild.title.str;
                        if(indent)
                            indentAmount += into_whitespace(toBuild.title.str);
                    }
                    else if(varName == "name")
                    {
                        os << toBuild.name;
                        if(indent)
                            indentAmount += into_whitespace(toBuild.name);
                    }
                    else if(varName == "contentpath")
                    {
                        os << toBuild.contentPath.str();
                        if(indent)
                            indentAmount += into_whitespace(toBuild.contentPath.str());
                    }
                    else if(varName == "outputpath")
                    {
                        os << toBuild.outputPath.str();
                        if(indent)
                            indentAmount += into_whitespace(toBuild.outputPath.str());
                    }
                    else if(varName == "contentext")
                    {
                        //checks for non-default content extension
                        Path extPath = toBuild.outputPath.getInfoPath();
                        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

                        if(std::ifstream(extPath.str()))
                        {
                            std::string ext;

                            std::ifstream ifs(extPath.str());
                            getline(ifs, ext);
                            ifs.close();

                            os << ext;
                            if(indent)
                                indentAmount += into_whitespace(ext);
                        }
                        else
                        {
                            os << contentExt;
                            if(indent)
                                indentAmount += into_whitespace(contentExt);
                        }
                    }
                    else if(varName == "outputext")
                    {
                        //checks for non-default output extension
                        Path extPath = toBuild.outputPath.getInfoPath();
                        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

                        if(std::ifstream(extPath.str()))
                        {
                            std::string ext;

                            std::ifstream ifs(extPath.str());
                            getline(ifs, ext);
                            ifs.close();

                            os << ext;
                            if(indent)
                                indentAmount += into_whitespace(ext);
                        }
                        else
                        {
                            os << outputExt;
                            if(indent)
                                indentAmount += into_whitespace(outputExt);
                        }
                    }
                    else if(varName == "scriptext")
                    {
                        //checks for non-default script extension
                        Path extPath = toBuild.outputPath.getInfoPath();
                        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

                        if(std::ifstream(extPath.str()))
                        {
                            std::string ext;

                            std::ifstream ifs(extPath.str());
                            getline(ifs, ext);
                            ifs.close();

                            os << ext;
                            if(indent)
                                indentAmount += into_whitespace(ext);
                        }
                        else
                        {
                            os << scriptExt;
                            if(indent)
                                indentAmount += into_whitespace(scriptExt);
                        }
                    }
                    else if(varName == "templatepath")
                    {
                        os << toBuild.templatePath.str();
                        if(indent)
                            indentAmount += into_whitespace(toBuild.templatePath.str());
                    }
                    else if(varName == "contentdir")
                    {
                        if(contentDir.size() && (contentDir[contentDir.size()-1] == '/' || contentDir[contentDir.size()-1] == '\\'))
                        {
                            os << contentDir.substr(0, contentDir.size()-1);
                            if(indent)
                                indentAmount += into_whitespace(contentDir.substr(0, contentDir.size()-1));
                        }
                        else
                        {
                            os << contentDir;
                            if(indent)
                                indentAmount += into_whitespace(contentDir);
                        }
                    }
                    else if(varName == "outputdir")
                    {
                        if(outputDir.size() && (outputDir[outputDir.size()-1] == '/' || outputDir[outputDir.size()-1] == '\\'))
                        {
                            os << outputDir.substr(0, outputDir.size()-1);
                            if(indent)
                                indentAmount += into_whitespace(outputDir.substr(0, outputDir.size()-1));
                        }
                        else
                        {
                            os << outputDir;
                            if(indent)
                                indentAmount += into_whitespace(outputDir);
                        }
                    }
                    else if(varName == "defaultcontentext")
                    {
                        os << contentExt;
                        if(indent)
                            indentAmount += into_whitespace(contentExt);
                    }
                    else if(varName == "defaultoutputext")
                    {
                        os << outputExt;
                        if(indent)
                            indentAmount += into_whitespace(outputExt);
                    }
                    else if(varName == "defaultscriptext")
                    {
                        os << scriptExt;
                        if(indent)
                            indentAmount += into_whitespace(scriptExt);
                    }
                    else if(varName == "defaulttemplate")
                    {
                        os << defaultTemplate.str();
                        if(indent)
                            indentAmount += into_whitespace(defaultTemplate.str());
                    }
                    else if(varName == "buildtimezone")
                    {
                        os << dateTimeInfo.cTimezone;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cTimezone);
                    }
                    else if(varName == "loadtimezone")
                    {
                        os << "<script>document.write(new Date().toString().split(\"(\")[1].split(\")\")[0])</script>";
                        if(indent)
                            indentAmount += std::string(82, ' ');
                    }
                    else if(varName == "timezone")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.cTimezone;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cTimezone);
                    }
                    else if(varName == "buildtime")
                    {
                        os << dateTimeInfo.cTime;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cTime);
                    }
                    else if(varName == "buildUTCtime")
                    {
                        os << dateTimeInfo.currentUTCTime();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
                    }
                    else if(varName == "builddate")
                    {
                        os << dateTimeInfo.cDate;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cDate);
                    }
                    else if(varName == "buildUTCdate")
                    {
                        os << dateTimeInfo.currentUTCDate();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
                    }
                    else if(varName == "currenttime")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.cTime;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cTime);
                    }
                    else if(varName == "currentUTCtime")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.currentUTCTime();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
                    }
                    else if(varName == "currentdate")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.cDate;
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.cDate);
                    }
                    else if(varName == "currentUTCdate")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.currentUTCDate();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
                    }
                    else if(varName == "loadtime")
                    {
                        os << "<script>document.write((new Date().toLocaleString()).split(\",\")[1])</script>";
                        if(indent)
                            indentAmount += std::string(76, ' ');
                    }
                    else if(varName == "loadUTCtime")
                    {
                        os << "<script>document.write((new Date().toISOString()).split(\"T\")[1].split(\".\")[0])</script>";
                        if(indent)
                            indentAmount += std::string(87, ' ');
                    }
                    else if(varName == "loaddate")
                    {
                        os << "<script>document.write((new Date().toLocaleString()).split(\",\")[0])</script>";
                        if(indent)
                            indentAmount += std::string(76, ' ');
                    }
                    else if(varName == "loadUTCdate")
                    {
                        os << "<script>document.write((new Date().toISOString()).split(\"T\")[0])</script>";
                        if(indent)
                            indentAmount += std::string(73, ' ');
                    }
                    else if(varName == "buildYYYY")
                    {
                        os << dateTimeInfo.currentYYYY();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
                    }
                    else if(varName == "buildYY")
                    {
                        os << dateTimeInfo.currentYY();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentYY());
                    }
                    else if(varName == "currentYYYY")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.currentYYYY();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
                    }
                    else if(varName == "currentYY")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.currentYY();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentYY());
                    }
                    else if(varName == "loadYYYY")
                    {
                        os << "<script>document.write(new Date().getFullYear())</script>";
                        if(indent)
                            indentAmount += std::string(57, ' ');
                    }
                    else if(varName == "loadYY")
                    {
                        os << "<script>document.write(new Date().getFullYear()%100)</script>";
                        if(indent)
                            indentAmount += std::string(61, ' ');
                    }
                    else if(varName == "buildOS")
                    {
                        os << dateTimeInfo.currentOS();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentOS());
                    }
                    else if(varName == "currentOS")
                    { //this is left for backwards compatibility
                        os << dateTimeInfo.currentOS();
                        if(indent)
                            indentAmount += into_whitespace(dateTimeInfo.currentOS());
                    }
                    else
                    {
                        os << "@";
                        linePos = sLinePos;
                        if(indent)
                            indentAmount += " ";
                    }
                }
                else if(inLine.substr(linePos, 2) == "@#")
                {
                    linePos = inLine.length();
                }
                else if(inLine.substr(linePos, 3) == "@//")
                {
                    linePos += 3;
                    std::string restOfLine = inLine.substr(linePos, inLine.size() - linePos);

                    std::istringstream iss(restOfLine);
                    std::ostringstream oss;

                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    //readPath << ": line " << lineNo << ":
                    if(read_and_process(0, iss, Path("", "single-line parsed comment"), antiDepsOfReadPath, oss, eos))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;

                    linePos = inLine.length();
                }
                else if(inLine.substr(linePos, 3) == "@\\t")
                {
                    linePos += 3;
                    os << "\t";
                    if(indent)
                        indentAmount += "\t";
                }
                else if(inLine.substr(linePos, 3) == "@\\n")
                {
                    linePos += 3;
                    indentAmount = baseIndentAmount;
                    if(codeBlockDepth)
                        os << "\n";
                    else
                        os << "\n" << baseIndentAmount;
                }
                else if(inLine.substr(linePos, 4) == "@!\\n")
                {
                    linePos += 4;
                    std::string restOfLine = inLine.substr(linePos, inLine.size() - linePos);

                    if(restOfLine.find("@!\\n") != std::string::npos)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": do not use @!\\n twice on the same line" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::istringstream iss(restOfLine);
                    std::ostringstream oss;

                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    //readPath << ": line " << lineNo << ":
                    if(read_and_process(0, iss, Path("", "special single-line parsed comment"), antiDepsOfReadPath, oss, eos))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;

                    getline(is, inLine);
                    lineNo++;
                    linePos=0;
                }
                else if(inLine.substr(linePos, 3) == "@/*")
                {
                    linePos += 3;
                    int openLine = lineNo;

                    std::stringstream ss;
                    std::ostringstream oss;

                    size_t endPos = inLine.find("@*/");

                    if(endPos != std::string::npos)
                        ss << inLine.substr(linePos, endPos - linePos);
                    else
                    {
                        while(endPos == std::string::npos)
                        {
                            ss << inLine.substr(linePos, inLine.size() - linePos) << "\n";

                            if(!getline(is, inLine))
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": lineNo " << openLine << ": open comment @/* has no close @*/" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            lineNo++;
                            linePos = 0;
                            endPos = inLine.find("@*/");
                        }

                        ss << inLine.substr(0, endPos);
                    }

                    linePos = endPos + 3;

                    //parses comment stringstream
                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    //readPath << ": line " << lineNo << ":
                    if(read_and_process(0, ss, Path("", "multi-line parsed comment"), antiDepsOfReadPath, oss, eos))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": lineNo " << openLine << ": @/* comment @*/: read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;
                }
                else if(inLine.substr(linePos, 3) == "@*/")
                {
                    os_mtx->lock();
                    eos << "error: " << readPath << ": lineNo " << lineNo << ": close comment @*/ has no open @/*" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                else if(inLine.substr(linePos, 2) == "@\\")
                {
                    linePos+=2;

                    switch(inLine[linePos])
                    {
                        case '\\':
                            linePos++;
                            os << "\\";
                            if(indent)
                                indentAmount += " ";
                            break;
                        case '@':
                            linePos++;
                            os << "@";
                            if(indent)
                                indentAmount += " ";
                            break;
                        case '<':
                            linePos++;
                            os << "&lt;";
                            if(indent)
                                indentAmount += std::string(4, ' ');
                            break;
                        case 't': //this isn't needed but here in case code is moved before @\\t code
                            linePos += 3;
                            os << "\t";
                            if(indent)
                                indentAmount += "\t";
                            break;
                        case 'n': //this isn't needed but here in case code is moved before @\\n code
                            linePos++;
                            indentAmount = baseIndentAmount;
                            if(codeBlockDepth)
                                os << "\n";
                            else
                                os << "\n" << baseIndentAmount;
                            break;
                        default:
                            os << "@\\";
                            if(indent)
                                indentAmount += "  ";
                    }
                }
                else if(inLine.substr(linePos, 4) == "@:=(" || inLine.substr(linePos, 5) == "@:=*(")
                {
                    linePos+=std::string("@:=").length();
                    std::string varType;
                    std::vector<std::pair<std::string, std::string> > vars;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_def(varType, vars, linePos, inLine, readPath, lineNo, "@:=(..)", eos))
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(varType);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "variable type"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": failed to parse variable type" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        varType = oss.str();

                        for(size_t v=0; v<vars.size(); v++)
                        {
                            iss = std::istringstream(vars[v].first);
                            oss.str("");
                            oss.clear();

                            indentAmount = "";

                            if(read_and_process(0, iss, Path("", "variable name"), antiDepsOfReadPath, oss, eos) > 0)
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": failed to parse variable name" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            vars[v].first = oss.str();

                            iss = std::istringstream(vars[v].second);
                            oss.str("");
                            oss.clear();

                            indentAmount = "";

                            if(read_and_process(0, iss, Path("", "variable value"), antiDepsOfReadPath, oss, eos) > 0)
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": failed to parse variable value" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }

                            vars[v].second = oss.str();
                        }

                        indentAmount = oldIndent;
                    }

                    if(varType == "string")
                    {
                        for(size_t v=0; v<vars.size(); v++)
                        {
                            if(strings.count(vars[v].first))
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": redeclaration of variable name '" << vars[v].first << "'" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else
                                strings[vars[v].first] = vars[v].second;
                        }
                    }
                    else
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": do not recognise the type '" << varType << "'" << std::endl;
                        eos << "note: more types including type defs will hopefully be coming soon" << std::endl; //can delete this later
                        os_mtx->unlock();
                    }
                }
                else if(inLine.substr(linePos, 11) == "@rawcontent")
                {
                    contentAdded = 1;
                    std::string replaceText = "@inputraw(" + quote(toBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 11, replaceText);
                }
                else if(inLine.substr(linePos, 13) == "@inputcontent")
                {
                    contentAdded = 1;
                    std::string replaceText = "@input(" + quote(toBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 13, replaceText);
                }
                else if(inLine.substr(linePos, 10) == "@inputhead")
                {
                    Path headPath = toBuild.contentPath;
                    headPath.file = headPath.file.substr(0, headPath.file.find_first_of('.')) + ".head";
                    if(std::ifstream(headPath.str()))
                    {
                        std::string replaceText = "@input(" + quote(headPath.str()) + ")";
                        inLine.replace(linePos, 10, replaceText);
                    }
                    else
                        inLine.replace(linePos, 10, "");
                }
                else if(inLine.substr(linePos, 7) == "@userin")
                {
                    linePos+=std::string("@userin").length();
                    std::string userInput, inputMsg = "";

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_msg(inputMsg, linePos, inLine, readPath, lineNo, "@userin()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(inputMsg);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "user input message"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        inputMsg = oss.str();

                        indentAmount = oldIndent;
                    }

                    os_mtx->lock();
                    std::cout << inputMsg << std::endl;
                    getline(std::cin, userInput);
                    os_mtx->unlock();

                    std::istringstream iss(userInput);
                    oss.str("");
                    oss.clear();

                    if(read_and_process(0, iss, Path("", "user input"), antiDepsOfReadPath, oss, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    userInput = oss.str();

                    os << userInput;
                    indentAmount += into_whitespace(userInput);
                }
                else if(inLine.substr(linePos, 11) == "@userfilein")
                {
                    linePos+=std::string("@userfilein").length();
                    std::string userInput, inputMsg = "";

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_msg(inputMsg, linePos, inLine, readPath, lineNo, "@userfilein()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(inputMsg);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "user file input message"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        inputMsg = oss.str();

                        indentAmount = oldIndent;
                    }

                    std::string output_filename = ".@userfilein" + std::to_string(sys_counter++);
                    int result;

                    std::string contExtPath = ".nsm/" + outputDir + toBuild.name + ".ext";

                    if(std::ifstream(contExtPath))
                    {
                        std::string ext;
                        std::ifstream ifs(contExtPath);
                        getline(ifs, ext);
                        ifs.close();
                        contExtPath += ext;
                    }
                    else
                        output_filename += contentExt;

                    os_mtx->lock();
                    std::ofstream ofs(output_filename);
                    ofs << inputMsg;
                    ofs.close();

                    #if defined _WIN32 || defined _WIN64
                        result = system((winTextEditor + " " + output_filename).c_str());
                    #else  //unix
                        result = system((unixTextEditor + " " + output_filename).c_str());
                    #endif
                    os_mtx->unlock();

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @userfinput(): text editor system call failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    if(!std::ifstream(output_filename))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @userfinput(): user did not save file" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    Path inputPath;
                    inputPath.set_file_path_from(output_filename);
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " would result in an input loop" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::ifstream ifs(inputPath.str());

                    //adds insert file
                    if(read_and_process(1, ifs, inputPath, antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    //indent amount updated inside read_and_process

                    ifs.close();

                    remove_file(Path("", output_filename));
                }
                else if(inLine.substr(linePos, 10) == "@inputraw(" || inLine.substr(linePos, 11) == "@inputraw*(")
                {
                    linePos+=std::string("@inputraw").length();
                    std::string inputPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(inputPathStr, linePos, inLine, readPath, lineNo, "@input()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(inputPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "input path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        inputPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    Path inputPath;
                    inputPath.set_file_path_from(inputPathStr);
                    depFiles.insert(inputPath);

                    if(inputPath == toBuild.contentPath)
                        contentAdded = 1;

                    //ensures insert file exists
                    if(!std::ifstream(inputPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::ifstream ifs(inputPath.str());
                    std::string fileLine, oldLine;
                    int fileLineNo = 0;

                    while(getline(ifs, fileLine))
                    {
                        if(0 < fileLineNo++)
                            os << "\n" << indentAmount;
                        oldLine = fileLine;
                        os << fileLine;
                    }
                    indentAmount += into_whitespace(oldLine);

                    ifs.close();
                }
                else if(inLine.substr(linePos, 7) == "@input(" || inLine.substr(linePos, 8) == "@input*(")
                {
                    linePos+=std::string("@input").length();
                    std::string inputPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(inputPathStr, linePos, inLine, readPath, lineNo, "@input()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(inputPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "input path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        inputPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    Path inputPath;
                    inputPath.set_file_path_from(inputPathStr);
                    depFiles.insert(inputPath);

                    if(inputPath == toBuild.contentPath)
                        contentAdded = 1;

                    //ensures insert file exists
                    if(!std::ifstream(inputPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " would result in an input loop" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    std::ifstream ifs(inputPath.str());

                    //adds insert file
                    if(read_and_process(1, ifs, inputPath, antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    //indent amount updated inside read_and_process

                    ifs.close();
                }
                else if(inLine.substr(linePos, 5) == "@dep(" || inLine.substr(linePos, 6) == "@dep*(")
                {
                    linePos+=std::string("@dep").length();
                    std::string depPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(depPathStr, linePos, inLine, readPath, lineNo, "@dep()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(depPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "dependency path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        depPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    Path depPath;
                    depPath.set_file_path_from(depPathStr);
                    depFiles.insert(depPath);

                    if(depPath == toBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(depPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @dep(" << quote(depPathStr) << ") failed as dependency does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 8) == "@script(" || inLine.substr(linePos, 8) == "@script*" || inLine.substr(linePos, 8) == "@script^")
                {
                    bool makeBackup = 1;
                    int c = sys_counter++;
                    int sLinePos = linePos;
                    linePos+=std::string("@script").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(c);
                    parseParams = 0;

                    while(inLine[linePos] == '*' || inLine[linePos] == '^')
                    {
                        if(inLine[linePos] == '*')
                        {
                            if(parseParams)
                                break;
                            parseParams = 1;
                            linePos++;
                        }
                        else if(inLine[linePos] == '^')
                        {
                            if(!makeBackup)
                                break;
                            makeBackup = 0;
                            linePos++;
                        }
                    }

                    if(inLine[linePos] != '(')
                    {
                        os << "@";
                        linePos = sLinePos + 1;
                        continue;
                    }

                    linePos++;

                    if(read_script_params(scriptPathStr, scriptParams, linePos, inLine, readPath, lineNo, "@script()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(scriptPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptPathStr = oss.str();

                        iss.str("");
                        iss.clear();
                        iss = std::istringstream(scriptParams);
                        oss.str("");
                        oss.clear();

                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script params"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptParams = oss.str();

                        indentAmount = oldIndent;
                    }

                    size_t pos = scriptPathStr.substr(1, scriptPathStr.size()-1).find_first_of('.');
                    std::string cScriptExt = "";
                    if(pos != std::string::npos)
                        cScriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(c) + cScriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    scriptPath.set_file_path_from(scriptPathStr);
                    depFiles.insert(scriptPath);

                    if(!std::ifstream(scriptPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    if(makeBackup)
                    {
                        if(cpFile(scriptPathStr, scriptPathStr + ".backup"))
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": failed to copy " << quote(scriptPathStr) << " to " << quote(scriptPathStr + ".backup") << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    //moves script to main directory
                    //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    int mcount = 0;
                    while(rename(scriptPathStr.c_str(), execPath.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << quote(scriptPathStr) << " to " << quote(execPath) << " 100 times already, may need to abort" << std::endl;
                            eos << "warning: you may need to move " << quote(execPath) << " back to " << quote(scriptPathStr) << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    mcount = 0;
                    while(rename(execPath.c_str(), scriptPathStr.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << execPath << " to " << scriptPathStr << " 100 times already, may need to abort" << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //deletes backup copy
                    if(makeBackup)
                        remove_file(Path("", scriptPathStr + ".backup"));

                    std::ifstream ifs(output_filename);
                    std::string str;
                    os_mtx->lock();
                    while(getline(ifs, str))
                        eos << str << std::endl;
                    os_mtx->unlock();
                    ifs.close();

                    remove_file(Path("./", output_filename));

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(scriptPathStr) << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 11) == "@scriptraw(" || inLine.substr(linePos, 11) == "@scriptraw*" || inLine.substr(linePos, 11) == "@scriptraw^")
                {
                    bool makeBackup = 1;
                    int c = sys_counter++;
                    int sLinePos = linePos;
                    linePos+=std::string("@scriptraw").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(c);
                    parseParams = 0;

                    while(inLine[linePos] == '*' || inLine[linePos] == '^')
                    {
                        if(inLine[linePos] == '*')
                        {
                            if(parseParams)
                                break;
                            parseParams = 1;
                            linePos++;
                        }
                        else if(inLine[linePos] == '^')
                        {
                            if(!makeBackup)
                                break;
                            makeBackup = 0;
                            linePos++;
                        }
                    }

                    if(inLine[linePos] != '(')
                    {
                        os << "@";
                        linePos = sLinePos + 1;
                        continue;
                    }

                    linePos++;

                    if(read_script_params(scriptPathStr, scriptParams, linePos, inLine, readPath, lineNo, "@scriptoutput()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(scriptPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptPathStr = oss.str();

                        iss.str("");
                        iss.clear();
                        iss = std::istringstream(scriptParams);
                        oss.str("");
                        oss.clear();

                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script params"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptParams = oss.str();

                        indentAmount = oldIndent;
                    }

                    size_t pos = scriptPathStr.substr(1, scriptPathStr.size()-1).find_first_of('.');
                    std::string cScriptExt = "";
                    if(pos != std::string::npos)
                        cScriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(c) + cScriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    scriptPath.set_file_path_from(scriptPathStr);
                    depFiles.insert(scriptPath);

                    if(scriptPath == toBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(scriptPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    if(makeBackup)
                    {
                        if(cpFile(scriptPathStr, scriptPathStr + ".backup"))
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": failed to copy " << quote(scriptPathStr) << " to " << quote(scriptPathStr + ".backup") << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    //moves script to main directory
                    //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    int mcount = 0;
                    while(rename(scriptPathStr.c_str(), execPath.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << scriptPathStr << " to " << execPath << " 100 times already, may need to abort" << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    mcount = 0;
                    while(rename(execPath.c_str(), scriptPathStr.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << execPath << " to " << scriptPathStr << " 100 times already, may need to abort" << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //deletes backup copy
                    if(makeBackup)
                        remove_file(Path("", scriptPathStr + ".backup"));

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);
                    std::string fileLine, oldLine;
                    int fileLineNo = 0;

                    while(getline(ifs, fileLine))
                    {
                        if(0 < fileLineNo++)
                            os << "\n" << indentAmount;
                        oldLine = fileLine;
                        os << fileLine;
                    }
                    indentAmount += into_whitespace(oldLine);

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else if(inLine.substr(linePos, 14) == "@scriptoutput(" || inLine.substr(linePos, 14) == "@scriptoutput*" || inLine.substr(linePos, 14) == "@scriptoutput^")
                {
                    bool makeBackup = 1;
                    int c = sys_counter++;
                    int sLinePos = linePos;
                    linePos+=std::string("@scriptoutput").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(c);
                    parseParams = 0;

                    while(inLine[linePos] == '*' || inLine[linePos] == '^')
                    {
                        if(inLine[linePos] == '*')
                        {
                            if(parseParams)
                                break;
                            parseParams = 1;
                            linePos++;
                        }
                        else if(inLine[linePos] == '^')
                        {
                            if(!makeBackup)
                                break;
                            makeBackup = 0;
                            linePos++;
                        }
                    }

                    if(inLine[linePos] != '(')
                    {
                        os << "@";
                        linePos = sLinePos + 1;
                        continue;
                    }

                    linePos++;

                    if(read_script_params(scriptPathStr, scriptParams, linePos, inLine, readPath, lineNo, "@scriptoutput()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(scriptPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptPathStr = oss.str();

                        iss.str("");
                        iss.clear();
                        iss = std::istringstream(scriptParams);
                        oss.str("");
                        oss.clear();

                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "script params"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        scriptParams = oss.str();

                        indentAmount = oldIndent;
                    }

                    size_t pos = scriptPathStr.substr(1, scriptPathStr.size()-1).find_first_of('.');
                    std::string cScriptExt = "";
                    if(pos != std::string::npos)
                        cScriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(c) + cScriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    scriptPath.set_file_path_from(scriptPathStr);
                    depFiles.insert(scriptPath);

                    if(scriptPath == toBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(scriptPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    if(makeBackup)
                    {
                        if(cpFile(scriptPathStr, scriptPathStr + ".backup"))
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": failed to copy " << quote(scriptPathStr) << " to " << quote(scriptPathStr + ".backup") << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }

                    //moves script to main directory
                    //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    int mcount = 0;
                    while(rename(scriptPathStr.c_str(), execPath.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << scriptPathStr << " to " << execPath << " 100 times already, may need to abort" << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                    mcount = 0;
                    while(rename(execPath.c_str(), scriptPathStr.c_str()))
                    {
                        if(++mcount == 100)
                        {
                            os_mtx->lock();
                            eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << execPath << " to " << scriptPathStr << " 100 times already, may need to abort" << std::endl;
                            os_mtx->unlock();
                        }
                    }

                    //deletes backup copy
                    if(makeBackup)
                        remove_file(Path("", scriptPathStr + ".backup"));

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of script '" << scriptPathStr << "'" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else if(inLine.substr(linePos, 8) == "@system(" || inLine.substr(linePos, 9) == "@system*(")
                {
                    linePos+=std::string("@system").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@system()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(sys_call);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "system call"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        sys_call = oss.str();

                        indentAmount = oldIndent;
                    }

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(sys_call).substr(0, 2) == "./")
                            sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
                    #else  //unix
                    #endif

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info")) //sys_call has to be quoted for cURL to work
                        result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());
                    else //sys_call has to be unquoted for cURL to work
                        result = system((sys_call + " > " + output_filename).c_str());

                    std::ifstream ifs(output_filename);
                    std::string str;
                    os_mtx->lock();
                    while(getline(ifs, str))
                        eos << str << std::endl;
                    os_mtx->unlock();
                    ifs.close();

                    remove_file(Path("./", output_filename));

                    //need sys_call quoted here for cURL to work
                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @system(" << quote(sys_call) << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 11) == "@systemraw(" || inLine.substr(linePos, 12) == "@systemraw*(")
                {
                    linePos+=std::string("@systemraw").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@systemoutput()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(sys_call);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "system call"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        sys_call = oss.str();

                        indentAmount = oldIndent;
                    }

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(sys_call).substr(0, 2) == "./")
                            sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
                    #else  //unix
                    #endif

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info")) //sys_call has to be quoted for cURL to work
                        result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());
                    else //sys_call has to be unquoted for cURL to work
                        result = system((sys_call + " > " + output_filename).c_str());

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @systemoutput(" << quote(sys_call) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error system output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);
                    std::string fileLine, oldLine;
                    int fileLineNo = 0;

                    while(getline(ifs, fileLine))
                    {
                        if(0 < fileLineNo++)
                            os << "\n" << indentAmount;
                        oldLine = fileLine;
                        os << fileLine;
                    }
                    indentAmount += into_whitespace(oldLine);

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else if(inLine.substr(linePos, 14) == "@systemoutput(" || inLine.substr(linePos, 15) == "@systemoutput*(")
                {
                    linePos+=std::string("@systemoutput").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@systemoutput()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(sys_call);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "system call"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        sys_call = oss.str();

                        indentAmount = oldIndent;
                    }

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(sys_call).substr(0, 2) == "./")
                            sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
                    #else  //unix
                    #endif

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info")) //sys_call has to be quoted for cURL to work
                        result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());
                    else //sys_call has to be unquoted for cURL to work
                        result = system((sys_call + " > " + output_filename).c_str());

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @systemoutput(" << quote(sys_call) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error system output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of system call '" << sys_call << "'" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else if(inLine.substr(linePos, 15) == "@systemcontent(" || inLine.substr(linePos, 16) == "@systemcontent*(")
                {
                    linePos+=std::string("@systemcontent").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemcontent" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@systemcontent()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(sys_call);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "system content call"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        sys_call = oss.str();

                        indentAmount = oldIndent;
                    }

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(sys_call).substr(0, 2) == "./")
                            sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
                    #else  //unix
                    #endif

                    contentAdded = 1;
                    sys_call += " " + quote(toBuild.contentPath.str());

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info")) //sys_call has to be quoted for cURL to work
                        result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());
                    else //sys_call has to be unquoted for cURL to work
                        result = system((sys_call + " > " + output_filename).c_str());

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @systemcontent(" << quote(sys_call) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error system output" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of system call '" << sys_call << "'" << std::endl;
                        os_mtx->unlock();
                        //remove_file(Path("./", output_filename));
                        return 1;
                    }

                    ifs.close();

                    remove_file(Path("./", output_filename));
                }
                else if(inLine.substr(linePos, 8) == "@pathto(" || inLine.substr(linePos, 9) == "@pathto*(")
                {
                    linePos+=std::string("@pathto").length();
                    Name targetName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(targetName, linePos, inLine, readPath, lineNo, "@pathto()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(targetName);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "@pathto path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        targetName = oss.str();

                        indentAmount = oldIndent;
                    }

                    //throws error if target targetName isn't being tracked by Nift
                    TrackedInfo targetInfo;
                    targetInfo.name = targetName;
                    if(!trackedAll->count(targetInfo))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }


                    Path targetPath = trackedAll->find(targetInfo)->outputPath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    os << pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(inLine.substr(linePos, 12) == "@pathtopage(" || inLine.substr(linePos, 13) == "@pathtopage*(")
                {
                    linePos+=std::string("@pathtopage").length();
                    Name targetName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(targetName, linePos, inLine, readPath, lineNo, "@pathtopage()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(targetName);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "@pathtopage path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        targetName = oss.str();

                        indentAmount = oldIndent;
                    }

                    //throws error if target targetName isn't being tracked by Nift
                    TrackedInfo targetInfo;
                    targetInfo.name = targetName;
                    if(!trackedAll->count(targetInfo))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }


                    Path targetPath = trackedAll->find(targetInfo)->outputPath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    os << pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(inLine.substr(linePos, 12) == "@pathtofile(" || inLine.substr(linePos, 13) == "@pathtofile*(")
                {
                    linePos+=std::string("@pathtofile").length();
                    Name targetFilePath;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(targetFilePath, linePos, inLine, readPath, lineNo, "@pathtofile()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(targetFilePath);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "@pathtofile path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        targetFilePath = oss.str();

                        indentAmount = oldIndent;
                    }

                    //throws error if targetFilePath doesn't exist
                    if(!std::ifstream(targetFilePath.c_str()))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    Path targetPath;
                    targetPath.set_file_path_from(targetFilePath);

                    Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    os << pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(inLine.substr(linePos, 5) == "@ent(" || inLine.substr(linePos, 6) == "@ent*(")
                {
                    linePos+=std::string("@ent").length();
                    std::string ent;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_param(ent, linePos, inLine, readPath, lineNo, "@<..>", eos))
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(ent);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "html entity"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": failed to parse html entity string" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        ent = oss.str();

                        indentAmount = oldIndent;
                    }

                    if(ent.size() == 1)
                    {
                        /*
                            see http://dev.w3.org/html5/html-author/charref for
                            more character references than you could ever need!
                        */
                        switch(ent[0])
                        {
                            case '`':
                                os << "&grave;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '~':
                                os << "&tilde;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '!':
                                os << "&excl;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '@':
                                os << "&commat;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '#':
                                os << "&num;";
                                if(indent)
                                    indentAmount += std::string(5, ' ');
                                break;
                            case '$': //MUST HAVE MATHJAX HANDLE THIS WHEN \$
                                os << "&dollar;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '%':
                                os << "&percnt;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '^':
                                os << "&Hat;";
                                if(indent)
                                    indentAmount += std::string(5, ' ');
                                break;
                            case '&':   //SEEMS TO BREAK SOME VALID JAVASCRIPT CODE WHEN \&
                                        //CHECK DEVELOPERS PERSONAL SITE GENEALOGY PAGE
                                        //FOR EXAMPLE (SEARCH FOR \&)
                                os << "&amp;";
                                if(indent)
                                    indentAmount += std::string(5, ' ');
                                break;
                            case '*':
                                os << "&ast;";
                                if(indent)
                                    indentAmount += std::string(5, ' ');
                                break;
                            case '?':
                                os << "&quest;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '<':
                                os << "&lt;";
                                if(indent)
                                    indentAmount += std::string(4, ' ');
                                break;
                            case '>':
                                os << "&gt;";
                                if(indent)
                                    indentAmount += std::string(4, ' ');
                                break;
                            case '(':
                                os << "&lpar;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case ')':
                                os << "&rpar;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '[':
                                os << "&lbrack;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case ']':
                                os << "&rbrack;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '{':
                                os << "&lbrace;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '}':
                                os << "&rbrace;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '-':
                                os << "&minus;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '_':
                                os << "&lowbar;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '=':
                                os << "&equals;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            case '+':
                                os << "&plus;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '|':
                                os << "&vert;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '\\':
                                os << "&bsol;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '/':
                                os << "&sol;";
                                if(indent)
                                    indentAmount += std::string(5, ' ');
                                break;
                            case ';':
                                os << "&semi;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case ':':
                                os << "&colon;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '\'':
                                os << "&apos;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case '"':
                                os << "&quot;";
                                if(indent)
                                    indentAmount += std::string(6, ' ');
                                break;
                            case ',':
                                os << "&comma;";
                                if(indent)
                                    indentAmount += std::string(7, ' ');
                                break;
                            case '.':
                                os << "&period;";
                                if(indent)
                                    indentAmount += std::string(8, ' ');
                                break;
                            default:
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": do not currently have an entity value for '" << ent << "'" << std::endl;
                                os_mtx->unlock();
                                return 1;
                        }
                    }
                    else if(ent == "£")
                    {
                        os << "&pound;";
                        if(indent)
                            indentAmount += std::string(7, ' ');
                    }
                    else if(ent == "¥")
                    {
                        os << "&yen;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                    }
                    else if(ent == "€")
                    {
                        os << "&euro;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "§" || ent == "section")
                    {
                        os << "&sect;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "+-")
                    {
                        os << "&pm;";
                        if(indent)
                            indentAmount += std::string(4, ' ');
                    }
                    else if(ent == "-+")
                    {
                        os << "&mp;";
                        if(indent)
                            indentAmount += std::string(4, ' ');
                    }
                    else if(ent == "!=")
                    {
                        os << "&ne;";
                        if(indent)
                            indentAmount += std::string(4, ' ');
                    }
                    else if(ent == "<=")
                    {
                        os << "&leq;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                    }
                    else if(ent == ">=")
                    {
                        os << "&geq;";
                        if(indent)
                            indentAmount += std::string(5, ' ');
                    }
                    else if(ent == "->")
                    {
                        os << "&rarr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "<-")
                    {
                        os << "&larr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "<->")
                    {
                        os << "&harr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "==>")
                    {
                        os << "&rArr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "<==")
                    {
                        os << "&lArr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "<==>")
                    {
                        os << "&hArr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else if(ent == "<=!=>")
                    {
                        os << "&nhArr;";
                        if(indent)
                            indentAmount += std::string(6, ' ');
                    }
                    else
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": do not currently have an entity value for '" << ent << "'" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 16) == "@faviconinclude(" || inLine.substr(linePos, 17) == "@faviconinclude*(") //checks for favicon include
                {
                    linePos+=std::string("@faviconinclude").length();
                    std::string faviconPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(faviconPathStr, linePos, inLine, readPath, lineNo, "@faviconinclude()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(faviconPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "favicon path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        faviconPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    //warns user if favicon file doesn't exist
                    if(!std::ifstream(faviconPathStr.c_str()))
                    {
                        os_mtx->lock();
                        eos << "warning: " << readPath << ": line " << lineNo << ": favicon file " << faviconPathStr << " does not exist" << std::endl;
                        os_mtx->unlock();
                    }

                    Path faviconPath;
                    faviconPath.set_file_path_from(faviconPathStr);

                    Path pathToFavicon(pathBetween(toBuild.outputPath.dir, faviconPath.dir), faviconPath.file);

                    std::string faviconInclude = "<link rel='icon' type='image/png' href='";
                    faviconInclude += pathToFavicon.str();
                    faviconInclude += "'>";

                    os << faviconInclude;
                    if(indent)
                        indentAmount += into_whitespace(faviconInclude);
                }
                else if(inLine.substr(linePos, 12) == "@cssinclude(" || inLine.substr(linePos, 13) == "@cssinclude*(") //checks for css includes
                {
                    linePos+=std::string("@cssinclude").length();
                    std::string cssPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(cssPathStr, linePos, inLine, readPath, lineNo, "@cssinclude()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(cssPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "css file path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        cssPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    //warns user if css file doesn't exist
                    if(!std::ifstream(cssPathStr.c_str()))
                    {
                        os_mtx->lock();
                        eos << "warning: " << readPath << ": line " << lineNo << ": css file " << cssPathStr << " does not exist" << std::endl;
                        os_mtx->unlock();
                    }

                    Path cssPath;
                    cssPath.set_file_path_from(cssPathStr);

                    Path pathToCSSFile(pathBetween(toBuild.outputPath.dir, cssPath.dir), cssPath.file);

                    std::string cssInclude = "<link rel='stylesheet' type='text/css' href='";
                    cssInclude += pathToCSSFile.str();
                    cssInclude += "'>";

                    os << cssInclude;
                    if(indent)
                        indentAmount += into_whitespace(cssInclude);
                }
                else if(inLine.substr(linePos, 12) == "@imginclude(" || inLine.substr(linePos, 13) == "@imginclude*(") //checks for img includes
                {
                    linePos+=std::string("@imginclude").length();
                    std::string imgPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(imgPathStr, linePos, inLine, readPath, lineNo, "@imginclude()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(imgPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "image file path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        imgPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    //warns user if img file doesn't exist
                    if(!std::ifstream(imgPathStr.c_str()))
                    {
                        os_mtx->lock();
                        eos << "warning: " << readPath << ": line " << lineNo << ": img file " << imgPathStr << " does not exist" << std::endl;
                        os_mtx->unlock();
                    }

                    Path imgPath;
                    imgPath.set_file_path_from(imgPathStr);

                    Path pathToIMGFile(pathBetween(toBuild.outputPath.dir, imgPath.dir), imgPath.file);

                    std::string imgInclude = "<img src=\"" + pathToIMGFile.str() + "\">";

                    os << imgInclude;
                    if(indent)
                        indentAmount += into_whitespace(imgInclude);
                }
                else if(inLine.substr(linePos, 11) == "@jsinclude(" || inLine.substr(linePos, 12) == "@jsinclude*(") //checks for js includes
                {
                    linePos+=std::string("@jsinclude").length();
                    std::string jsPathStr;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(jsPathStr, linePos, inLine, readPath, lineNo, "@jsinclude()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(jsPathStr);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "javascript file path"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        jsPathStr = oss.str();

                        indentAmount = oldIndent;
                    }

                    //warns user if js file doesn't exist
                    if(!std::ifstream(jsPathStr.c_str()))
                    {
                        os_mtx->lock();
                        eos << "warning: " << readPath << ": line " << lineNo << ": js file " << jsPathStr << " does not exist" << std::endl;
                        os_mtx->unlock();
                    }

                    Path jsPath;
                    jsPath.set_file_path_from(jsPathStr);

                    Path pathToJSFile(pathBetween(toBuild.outputPath.dir, jsPath.dir), jsPath.file);

                    //the type attribute is unnecessary for JavaScript resources.
                    //std::string jsInclude="<script type='text/javascript' src='";
                    std::string jsInclude="<script src='";
                    jsInclude += pathToJSFile.str();
                    jsInclude+="'></script>";

                    os << jsInclude;
                    if(indent)
                        indentAmount += into_whitespace(jsInclude);
                }
                else
                {
                    os << '@';
                    if(indent)
                        indentAmount += " ";
                    linePos++;
                }
            }
            else //regular character
            {
                if(indent)
                {
                    if(inLine[linePos] == '\t')
                        indentAmount += '\t';
                    else
                        indentAmount += ' ';
                }
                os << inLine[linePos++];
            }
        }
    }

    if(codeBlockDepth > baseCodeBlockDepth)
    {
        os_mtx->lock();
        eos << "error: " << readPath << ": line " << openCodeLineNo << ": <pre*> open tag has no following </pre> close tag." << std::endl;
        os_mtx->unlock();
        return 1;
    }

    return 0;
}

int Parser::read_path(std::string& pathRead,
                           size_t& linePos,
                           const std::string& inLine,
                           const Path& readPath,
                           const int& lineNo,
                           const std::string& callType,
                           std::ostream& os)
{
    pathRead = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no path provided
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no path provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads the input path
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        //reads path value
        for(; linePos < inLine.size(); ++linePos)
        {
            if(inLine[linePos] == '(')
                depth++;
            else if(inLine[linePos] == ')')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        strip_trailing_whitespace(pathRead);
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the path is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid path inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_script_params(std::string& pathRead,
                                    std::string& paramStr,
                                    size_t& linePos,
                                    const std::string& inLine,
                                    const Path& readPath,
                                    const int& lineNo,
                                    const std::string& callType,
                                    std::ostream& os)
{
    pathRead = paramStr = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": script path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no path provided
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no script path provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads the script path
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        //reads path value
        for(; linePos < inLine.size() && inLine[linePos] != ','; ++linePos)
        {
            if(inLine[linePos] == '(')
                depth++;
            else if(inLine[linePos] == ')')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                pathRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                pathRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            pathRead += inLine[linePos];
        }
        strip_trailing_whitespace(pathRead);

        //should we throw errors if depth > 1?
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": script path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    if(inLine[linePos] == ',')
    {
        linePos++;

        //skips over leading whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
            ++linePos;

        //throws error if either no closing bracket or a newline
        if(linePos >= inLine.size())
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": script parameters have no closing bracket ) or newline inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //throws error if no path provided
        if(inLine[linePos] == ')')
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": no script parameters provided inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //reads the input path
        if(inLine[linePos] == '\'')
        {
            ++linePos;
            for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    paramStr += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    paramStr += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                paramStr += inLine[linePos];
            }
            ++linePos;
        }
        else if(inLine[linePos] == '"')
        {
            ++linePos;
            for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    paramStr += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    paramStr += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                paramStr += inLine[linePos];
            }
            ++linePos;
        }
        else
        {
            int depth = 1;

            //reads path value
            for(; linePos < inLine.size(); ++linePos)
            {
                if(inLine[linePos] == '(')
                    depth++;
                else if(inLine[linePos] == ')')
                {
                    if(depth == 1)
                        break;
                    else
                        depth--;
                }

                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    paramStr += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    paramStr += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                paramStr += inLine[linePos];
            }
            strip_trailing_whitespace(paramStr);
        }

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
            ++linePos;

        //throws error if new line is between the path and close bracket
        if(linePos >= inLine.size())
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": script parameters have no closing bracket ) or newline inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    //throws error if the path is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid script path inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_msg(std::string& msgRead,
                          size_t& linePos,
                          const std::string& inLine,
                          const Path& readPath,
                          const int& lineNo,
                          const std::string& callType,
                          std::ostream& os)
{
    msgRead = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": user input message has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no path provided
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no user input message provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads the input path
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                msgRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                msgRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            msgRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                msgRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                msgRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            msgRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        //reads path value
        for(; linePos < inLine.size(); ++linePos)
        {
            if(inLine[linePos] == '(')
                depth++;
            else if(inLine[linePos] == ')')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                msgRead += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                msgRead += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            msgRead += inLine[linePos];
        }
        strip_trailing_whitespace(msgRead);
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": user input message has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the path is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid user input message inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_sys_call(std::string& sys_call,
                               size_t& linePos,
                               const std::string& inLine,
                               const Path& readPath,
                               const int& lineNo,
                               const std::string& callType,
                               std::ostream& os)
{
    sys_call = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no system call provided
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no system call provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads the input system call
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                sys_call += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                sys_call += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            sys_call += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                sys_call += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                sys_call += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            sys_call += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;
        for(; linePos < inLine.size(); ++linePos)
        {
            if(inLine[linePos] == '(')
                depth++;
            else if(inLine[linePos] == ')')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                sys_call += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                sys_call += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            sys_call += inLine[linePos];
        }
        strip_trailing_whitespace(sys_call);
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the system call and close bracket
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the system call is invalid (eg. has text after quoted system call)
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid system call inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_def(std::string& varType,
                           std::vector<std::pair<std::string, std::string> >& vars,
                           size_t& linePos,
                           const std::string& inLine,
                           const Path& readPath,
                           const int& lineNo,
                           const std::string& callType,
                           std::ostream& os)
{
    varType = "";
    vars.clear();
    std::pair<std::string, std::string> newVar("", "");

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no variable definition
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no variable definition inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads variable type
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varType += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varType += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        for(; linePos < inLine.size() && inLine[linePos] != ','; ++linePos)
        {
            if(inLine[linePos] == '(')
                depth++;
            else if(inLine[linePos] == ')')
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varType += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                varType += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varType += inLine[linePos];
        }
        strip_trailing_whitespace(varType);
    }

    //skips over whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no variable definition
    if(inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no variable definition inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    if(inLine[linePos] != ',')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": " << callType << " : variable definition has no comma between variable type and variables" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    while(inLine[linePos] == ',')
    {
        newVar.first = newVar.second = "";

        linePos++;

        //skips over whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
            ++linePos;

        //throws error if either no closing bracket or a newline
        if(linePos >= inLine.size())
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //throws error if no variable definition
        if(inLine[linePos] == ')')
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": incomplete variable definition inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //reads variable name
        if(inLine[linePos] == '\'')
        {
            ++linePos;
            for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                newVar.first += inLine[linePos];
            }
            ++linePos;
        }
        else if(inLine[linePos] == '"')
        {
            ++linePos;
            for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                newVar.first += inLine[linePos];
            }
            ++linePos;
        }
        else
        {
            int depth = 1;

            for(; linePos < inLine.size() && inLine[linePos] != ',' && inLine[linePos] != '='; ++linePos)
            {
                if(inLine[linePos] == '(')
                    depth++;
                else if(inLine[linePos] == ')')
                {
                    if(depth == 1)
                        break;
                    else
                        depth--;
                }

                if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                {
                    newVar.first += '\n';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                {
                    newVar.first += '\t';
                    linePos++;
                    continue;
                }
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                    linePos++;
                else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                    linePos++;
                newVar.first += inLine[linePos];
            }
            strip_trailing_whitespace(newVar.first);
        }

        //skips over whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
            ++linePos;

        if(linePos < inLine.size() && inLine[linePos] == '=') //reads variable value
        {
            linePos++;

            //skips over whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                ++linePos;

            //throws error if either no closing bracket or a newline
            if(linePos >= inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            //throws error if no variable definition
            if(inLine[linePos] == ')')
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": incomplete variable definition inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }

            //reads the variable value
            if(inLine[linePos] == '\'')
            {
                ++linePos;
                for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
                {
                    if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                    {
                        newVar.second += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                    {
                        newVar.second += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                        linePos++;
                    newVar.second += inLine[linePos];
                }
                ++linePos;
            }
            else if(inLine[linePos] == '"')
            {
                ++linePos;
                for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
                {
                    if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                    {
                        newVar.second += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                    {
                        newVar.second += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                        linePos++;
                    newVar.second += inLine[linePos];
                }
                ++linePos;
            }
            else
            {
                int depth = 1;

                //reads variable value
                for(; linePos < inLine.size() && inLine[linePos] != ','; ++linePos)
                {
                    if(inLine[linePos] == '(')
                        depth++;
                    else if(inLine[linePos] == ')')
                    {
                        if(depth == 1)
                            break;
                        else
                            depth--;
                    }

                    if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
                    {
                        newVar.second += '\n';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
                    {
                        newVar.second += '\t';
                        linePos++;
                        continue;
                    }
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                        linePos++;
                    newVar.second += inLine[linePos];
                }
                strip_trailing_whitespace(newVar.second);
            }

            //skips over hopefully trailing whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                ++linePos;

            //throws error if new line is between the variable definition and close bracket
            if(linePos >= inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        vars.push_back(newVar);
    }

    //throws error if new line is between the variable definition and close bracket
    if(linePos >= inLine.size()) //probably don't need this
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the variable definition is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid variable definition inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_param(std::string& param,
                          size_t& linePos,
                          const std::string& inLine,
                          const Path& readPath,
                          const int& lineNo,
                          const std::string& callType,
                          std::ostream& os)
{
    char closeBracket, openBracket;
    param = "";

    if(linePos <= 0)
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": requested parameter be read from linePos <= 0 with " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    else if(inLine[linePos-1] == '<')
    {
        openBracket = '<';
        closeBracket = '>';
    }
    else if(inLine[linePos-1] == '(')
    {
        openBracket = '(';
        closeBracket = ')';
    }
    else if(inLine[linePos-1] == '[')
    {
        openBracket = '[';
        closeBracket = ']';
    }
    else if(inLine[linePos-1] == '{')
    {
        openBracket = '{';
        closeBracket = '}';
    }
    else
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": could not determine open bracket type with " << callType << " call" << std::endl;
        os << "note: make sure read_param is called with linePos one past the open bracket" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": parameter has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if no variable definition
    if(inLine[linePos] == closeBracket)
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no parameter inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads variable name
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '\''; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                param += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                param += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            param += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; linePos < inLine.size() && inLine[linePos] != '"'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                param += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                param += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            param += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        int depth = 1;

        for(; linePos < inLine.size(); ++linePos)
        {
            if(inLine[linePos] == openBracket)
                depth++;
            else if(inLine[linePos] == closeBracket)
            {
                if(depth == 1)
                    break;
                else
                    depth--;
            }

            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                param += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                param += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            param += inLine[linePos];
        }
        strip_trailing_whitespace(param);
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the variable name and close bracket
    if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no closing bracket ) for parameter or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the parameter is invalid
    if(inLine[linePos] != closeBracket)
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid parameter inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}
