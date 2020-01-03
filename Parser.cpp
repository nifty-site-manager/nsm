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
            if(inLine[linePos] == '\\') //checks whether to escape
            {
                linePos++;
                if(inLine[linePos] == '@')
                {
                    os << "@";
                    linePos++;
                    if(indent)
                        indentAmount += " ";
                }
                else
                {
                    os << "\\";
                    if(indent)
                        indentAmount += " ";
                }
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

                    linePos = endPos + 4;

                    //skips to next non-whitespace
                    //skips over leading whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                        ++linePos;

                    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    {
                        if(!getline(is, inLine))
                            break;
                        linePos = 0;
                        ++lineNo;

                        //skips over hopefully leading whitespace
                        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                            ++linePos;
                    }

                    --linePos; // linePos is incemented again further below (a bit gross!)
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

                    os_mtx->lock();
                    eos << "warning: " << readPath << ": line " << lineNo << ": please change @pagetitle to either @[title] or @<title>, @pagetitle will stop working in a few versions" << std::endl;
                    os_mtx->unlock();
                }
                else if(inLine.substr(linePos, 13) == "@inputcontent")
                {
                    contentAdded = 1;
                    std::string replaceText = "@input{!p}(" + quote(toBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 13, replaceText);

                    os_mtx->lock();
                    eos << "warning: " << readPath << ": line " << lineNo << ": please change @inputcontent to @content{!p}(), @inputcontent will stop working in a few versions" << std::endl;
                    os_mtx->unlock();
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
                else if(inLine.substr(linePos, 4) == "@---")
                {
                    linePos += 4;
                    int openLine = lineNo;

                    std::stringstream ss;
                    std::ostringstream oss;

                    size_t endPos = inLine.find("@---", linePos);

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
                                eos << "error: " << readPath << ": line " << openLine << ": open/close comment @--- has no close/open @---" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            lineNo++;
                            linePos = 0;
                            endPos = inLine.find("@---");
                        }

                        ss << inLine.substr(0, endPos);
                    }

                    linePos = endPos + 4;

                    //skips to next non-whitespace
                    //skips over leading whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                        ++linePos;

                    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    {
                        if(!getline(is, inLine))
                            break;
                        linePos = 0;
                        ++lineNo;

                        //skips over hopefully leading whitespace
                        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                            ++linePos;
                    }

                    //parses comment stringstream
                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    //readPath << ": line " << lineNo << ":
                    if(read_and_process(0, ss, Path("", "multi-line parsed comment"), antiDepsOfReadPath, oss, eos))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << openLine << ": @--- comment @---: failed to parse comment text" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    indentAmount = oldIndent;
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
                                eos << "error: " << readPath << ": line " << openLine << ": open comment @/* has no close @*/" << std::endl;
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

                    //skips to next non-whitespace
                    //skips over leading whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                        ++linePos;

                    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    {
                        if(!getline(is, inLine))
                            break;
                        linePos = 0;
                        ++lineNo;

                        //skips over hopefully leading whitespace
                        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                            ++linePos;
                    }

                    //parses comment stringstream
                    std::string oldIndent = indentAmount;
                    indentAmount = "";

                    //readPath << ": line " << lineNo << ":
                    if(read_and_process(0, ss, Path("", "multi-line parsed comment"), antiDepsOfReadPath, oss, eos))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << openLine << ": @/* comment @*/: failed to parse comment text" << std::endl;
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
                else
                {
                    linePos++;
                    int sLinePos = linePos;
                    bool printVar = 0;
                    std::string funcName;
                    std::vector<std::string> options, params;

                    //reads function name
                    if(read_func_name(funcName, linePos, inLine, readPath, lineNo, eos, is))
                        return 1;

                    //reads options
                    if(linePos < inLine.size() && inLine[linePos] == '{')
                        if(read_options(options, linePos, inLine, readPath, lineNo, "@" + funcName + "()", eos, is))
                            return 1;

                    //bails early if not a function or print variable call
                    if(linePos >= inLine.size() ||
                       (funcName != "" && inLine[linePos] != '(') ||
                       (funcName == "" && inLine[linePos] != '[' && inLine[linePos] != '<'))
                    {
                        os << "@";
                        linePos = sLinePos;
                        if(indent)
                            indentAmount += " ";
                        continue;
                    }

                    //checks whether printing a variable
                    if(funcName == "" && (inLine[linePos] == '[' || inLine[linePos] == '<'))
                        printVar = 1;

					parseParams = 1;
					for(size_t o=0; o<options.size(); o++)
						if(options[o] == "!p")
							parseParams = 0;

                    if(parseParams)
                    {
                        if(parse_replace(funcName, "function name", readPath, antiDepsOfReadPath, lineNo, "@" + funcName + "()", eos))
                            return 1;

                        if(parse_replace(options, "options", readPath, antiDepsOfReadPath, lineNo, "@" + funcName + "()", eos))
                            return 1;
                    }

                    //reads parameters
                    if(funcName == ":=")
                    {

                    }
                    else
                    {
                        if(read_params(params, linePos, inLine, readPath, lineNo, "@" + funcName + "()", eos, is))
                            return 1;

                        if(parseParams && parse_replace(params, "parameters", readPath, antiDepsOfReadPath, lineNo, "@" + funcName + "()", eos))
                            return 1;
                    }

                    /*os_mtx->lock();
                    std::cout << "funcName: '" << funcName << "'" << std::endl;
                    std::cout << "options.size(): " << options.size() << std::endl;
					for(size_t o=0; o<options.size(); o++)
						std::cout << "'" << options[o] << "' ";
                    std::cout << std::endl;
                    std::cout << "params.size(): " << params.size() << std::endl;
					for(size_t p=0; p<params.size(); p++)
						std::cout << "'" << params[p] << "' ";
                    std::cout << std::endl;
                    os_mtx->unlock();*/

                    if(funcName == "" && printVar)
                    {
                        if(params.size() != 1)
                        {
                            os << "@";
                            linePos = sLinePos;
                            if(indent)
                                indentAmount += " ";
                            continue;
                        }

                        std::string varName = params[0];

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
                    else if(funcName == "input")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @input(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string inputPathStr = params[0];
                        bool ifExists = 0, inputRaw = 0;

                        Path inputPath;
                        inputPath.set_file_path_from(inputPathStr);
                        depFiles.insert(inputPath);

                        if(inputPath == toBuild.contentPath)
                            contentAdded = 1;

                        if(options.size())
                        {
                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(options[o] == "raw")
                                    inputRaw = 1;
                                else if(options[o] == "if-exists")
                                    ifExists = 1;
                            }
                        }

                        //ensures insert file exists
                        if(std::ifstream(inputPathStr))
                        {
                            if(!inputRaw)
                            {
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
                            else
                            {
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
                        }
                        else if(ifExists)
                        {
                            //skips over leading whitespace
                            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                                ++linePos;

                            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                            {
                                if(!getline(is, inLine))
                                    break;
                                linePos = 0;
                                ++lineNo;

                                //skips over hopefully leading whitespace
                                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                                    ++linePos;
                            }
                        }
                        else
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(funcName == "pathto")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @pathto(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        bool fromName = 0, toFile = 0;

                        if(options.size())
                        {
                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(options[o] == "name")
                                {
                                    if(toFile)
                                    {
                                        os_mtx->lock();
                                        eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << params[0] << ") failed, cannot have both options file and name" << std::endl;
                                        os_mtx->unlock();
                                        return 1;
                                    }
                                    fromName = 1;
                                }
                                else if(options [o] == "file")
                                {
                                    if(fromName)
                                    {
                                        os_mtx->lock();
                                        eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << params[0] << ") failed, cannot have both options file and name" << std::endl;
                                        os_mtx->unlock();
                                        return 1;
                                    }
                                    toFile = 1;
                                }
                            }
                        }

                        if(fromName == 0 && toFile == 0)
                            fromName = toFile = 1;

                        if(fromName)
                        {
                            Name targetName = params[0];
                            TrackedInfo targetInfo;
                            targetInfo.name = targetName;

                            if(trackedAll->count(targetInfo))
                            {
                                toFile = 0;
                                Path targetPath = trackedAll->find(targetInfo)->outputPath;
                                //targetPath.set_file_path_from(targetPathStr);

                                Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                                //adds path to target
                                os << pathToTarget.str();
                                if(indent)
                                    indentAmount += into_whitespace(pathToTarget.str());
                            }
                            else if(!toFile) //throws error if target targetName isn't being tracked by Nift
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }

                        if(toFile)
                        {
                            std::string targetFilePath = params[0];

                            if(std::ifstream(targetFilePath.c_str()))
                            {
                                fromName = 0;
                                Path targetPath;
                                targetPath.set_file_path_from(targetFilePath);

                                Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                                //adds path to target
                                os << pathToTarget.str();
                                if(indent)
                                    indentAmount += into_whitespace(pathToTarget.str());
                            }
                            else if(!fromName) //throws error if targetFilePath doesn't exist
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                            else
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << params[0] << "): " << quote(params[0]) << " is neither a tracked name nor a file that exists" << std::endl;
                                os_mtx->unlock();
                                return 1;
                            }
                        }
                    }
                    else if(funcName == "pathtopage")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @pathtopage(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        Name targetName = params[0];
                        TrackedInfo targetInfo;
                        targetInfo.name = targetName;

                        if(trackedAll->count(targetInfo))
                        {
                            Path targetPath = trackedAll->find(targetInfo)->outputPath;
                            //targetPath.set_file_path_from(targetPathStr);

                            Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                            //adds path to target
                            os << pathToTarget.str();
                            if(indent)
                                indentAmount += into_whitespace(pathToTarget.str());
                        }
                        else //throws error if target targetName isn't being tracked by Nift
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetName << ") failed, Nift not tracking " << targetName << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(funcName == "pathtofile")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @pathtofile(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string targetFilePath = params[0];

                        if(std::ifstream(targetFilePath.c_str()))
                        {
                            Path targetPath;
                            targetPath.set_file_path_from(targetFilePath);

                            Path pathToTarget(pathBetween(toBuild.outputPath.dir, targetPath.dir), targetPath.file);

                            //adds path to target
                            os << pathToTarget.str();
                            if(indent)
                                indentAmount += into_whitespace(pathToTarget.str());
                        }
                        else //throws error if targetFilePath doesn't exist
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(funcName == "content")
                    {
                        if(params.size() != 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @content(): expected 0 parameters, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        contentAdded = 1;
                        std::string replaceText = "@input{!p";
                        if(options.size())
                            for(size_t o=0; o<options.size(); o++)
                                if(options[o] != "!p")
                                    replaceText += ", " + options[o];
                        replaceText += "}(" + quote(toBuild.contentPath.str()) + ")";
                        inLine.replace(linePos, 0, replaceText);
                    }
                    else if(funcName == ":=")
                    {
                        std::string varType;
                        std::vector<std::pair<std::string, std::string> > vars;

                        if(read_def(varType, vars, linePos, inLine, readPath, lineNo, "@:=(..)", eos, is))
                            return 1;

                        if(parseParams)
                        {
                            std::istringstream iss(varType);
                            std::ostringstream oss;

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
                                std::ostringstream oss;

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
                                oss = std::ostringstream();

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
                    else if(funcName == "in")
                    {
                        if(params.size() > 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @in(): expected 0 or 1 parameters, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        bool fromFile = 0;
                        std::string userInput, inputMsg;
                        if(params.size() > 0)
                            inputMsg = params[0];

                        if(options.size())
                            for(size_t o=0; o<options.size(); o++)
                                if(options[o] == "from-file")
                                    fromFile = 1;

                        if(!fromFile)
                        {
                            os_mtx->lock();
                            if(inputMsg != "")
                                std::cout << inputMsg << std::endl;
                            getline(std::cin, userInput);
                            os_mtx->unlock();

                            std::istringstream iss(userInput);
                            std::ostringstream oss;

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
                        else
                        {
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

                            std::ofstream ofs(output_filename);
                            ofs << inputMsg;
                            ofs.close();

                            os_mtx->lock();
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
                    }
                    else if(funcName == "dep")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @dep(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string depPathStr = params[0];

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
                    else if(funcName == "script")
                    {
                        if(params.size() == 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": no script path provided" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        bool ifExists = 0,
                             inject = 0,
                             injectRaw = 0,
                             attachContentPath = 0,
                             makeBackup = 1;
                        int c = sys_counter++;
                        std::string output_filename = ".@scriptoutput" + std::to_string(c);

                        if(options.size())
                        {
                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(options[o] == "!bs")
                                    makeBackup = 0;
                                else if(options[o] == "if-exists")
                                    ifExists = 1;
                                else if(options[o] == "inject")
                                    inject = 1;
                                else if(options[o] == "raw")
                                    injectRaw = 1;
                                else if(options[o] == "content")
                                    attachContentPath = 1;
                            }
                        }

                        if(params.size() > 1)
                        {
                            params[1] = quote(params[1]);
                            for(size_t i=2; i<params.size(); i++)
                                params[1] += " " + quote(params[i]);
                        }
                        else if(params.size() == 1)
                            params.push_back("");

                        size_t pos = params[0].substr(1, params[0].size()-1).find_first_of('.');
                        std::string cScriptExt = "";
                        if(pos != std::string::npos)
                            cScriptExt = params[0].substr(pos+1, params[0].size()-pos-1);
                        std::string execPath = "./.script" + std::to_string(c) + cScriptExt;

                        if(inject && injectRaw)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(execPath) << "): inject and raw options are incompatible with each other" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        else if(attachContentPath)
                        {
                            contentAdded = 1;
                            params[1] = quote(toBuild.contentPath.str()) + " " + params[1];
                        }

                        #if defined _WIN32 || defined _WIN64
                            if(unquote(execPath).substr(0, 2) == "./")
                                execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                        #else  //unix
                        #endif

                        Path scriptPath;
                        scriptPath.set_file_path_from(params[0]);
                        if(scriptPath == toBuild.contentPath)
                            contentAdded = 1;
                        depFiles.insert(scriptPath);

                        if(std::ifstream(params[0]))
                        {
                            //copies script to backup location
                            if(makeBackup)
                            {
                                if(cpFile(params[0], params[0] + ".backup"))
                                {
                                    os_mtx->lock();
                                    eos << "error: " << readPath << ": line " << lineNo << ": failed to copy " << quote(params[0]) << " to " << quote(params[0] + ".backup") << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }

                            //moves script to main directory
                            //note if just copy original script or move copied script get 'Text File Busy' errors (can be quite rare)
                            //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                            int mcount = 0;
                            while(rename(params[0].c_str(), execPath.c_str()))
                            {
                                if(++mcount == 100)
                                {
                                    os_mtx->lock();
                                    eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << quote(params[0]) << " to " << quote(execPath) << " 100 times already, may need to abort" << std::endl;
                                    eos << "warning: you may need to move " << quote(execPath) << " back to " << quote(params[0]) << std::endl;
                                    os_mtx->unlock();
                                }
                            }

                            //checks whether we're running from flatpak
                            int result;
                            if(std::ifstream("/.flatpak-info"))
                                result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + params[1]) + " > " + output_filename).c_str());
                            else
                                result = system((execPath + " " + params[1] + " > " + output_filename).c_str());

                            //moves script back to original location
                            //sometimes this fails (on Windows) for some reason, so keeps trying until successful
                            mcount = 0;
                            while(rename(execPath.c_str(), params[0].c_str()))
                            {
                                if(++mcount == 100)
                                {
                                    os_mtx->lock();
                                    eos << "warning: " << readPath << ": line " << lineNo << ": have tried to move " << execPath << " to " << params[0] << " 100 times already, may need to abort" << std::endl;
                                    eos << "warning: you may need to move " << quote(execPath) << " back to " << quote(params[0]) << std::endl;
                                    os_mtx->unlock();
                                }
                            }

                            //deletes backup copy
                            if(makeBackup)
                                remove_file(Path("", params[0] + ".backup"));

                            if(inject)
                            {
                                if(result)
                                {
                                    os_mtx->lock();
                                    eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(params[0]) << ") failed" << std::endl;
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
                                    eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of script '" << params[0] << " " << params[1] << "'" << std::endl;
                                    os_mtx->unlock();
                                    //remove_file(Path("./", output_filename));
                                    return 1;
                                }

                                ifs.close();

                                remove_file(Path("./", output_filename));
                            }
                            else if(injectRaw)
                            {
                                if(result)
                                {
                                    os_mtx->lock();
                                    eos << "error: " << readPath << ": line " << lineNo << ": @scriptraw(" << quote(params[0]) << ") failed" << std::endl;
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
                            else
                            {
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
                                    eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(params[0]) << ") failed" << std::endl;
                                    os_mtx->unlock();
                                    return 1;
                                }
                            }
                        }
                        else if(ifExists)
                        {
                            //skips over leading whitespace
                            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                                ++linePos;

                            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                            {
                                if(!getline(is, inLine))
                                    break;
                                linePos = 0;
                                ++lineNo;

                                //skips over hopefully leading whitespace
                                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                                    ++linePos;
                            }
                        }
                        else
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(params[0]) << ") failed as script " << params[0] << " does not exist" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                    }
                    else if(funcName == "system")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @system(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        bool inject = 0, injectRaw = 0, attachContentPath = 0;
                        std::string sys_call = params[0];
                        std::string output_filename = ".@systemoutput" + std::to_string(sys_counter++);

                        #if defined _WIN32 || defined _WIN64
                            if(unquote(sys_call).substr(0, 2) == "./")
                                sys_call = unquote(sys_call).substr(2, unquote(sys_call).size()-2);
                        #else  //unix
                        #endif

                        if(options.size())
                        {
                            for(size_t o=0; o<options.size(); o++)
                            {
                                if(options[o] == "inject")
                                    inject = 1;
                                else if(options[o] == "raw")
                                    injectRaw = 1;
                                else if(options[o] == "content")
                                    attachContentPath = 1;
                            }
                        }

                        if(inject && injectRaw)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @system(" << quote(sys_call) << "): inject and raw options are incompatible with each other" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }
                        else if(attachContentPath)
                        {
                            contentAdded = 1;
                            sys_call += " " + quote(toBuild.contentPath.str());
                        }

                        //checks whether we're running from flatpak
                        int result;
                        if(std::ifstream("/.flatpak-info")) //sys_call has to be quoted for cURL to work
                            result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());
                        else //sys_call has to be unquoted for cURL to work
                            result = system((sys_call + " > " + output_filename).c_str());

                        if(inject)
                        {
                            if(result)
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": @system{inject}(" << quote(sys_call) << ") failed" << std::endl;
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
                        else if(injectRaw)
                        {
                            if(result)
                            {
                                os_mtx->lock();
                                eos << "error: " << readPath << ": line " << lineNo << ": @system{raw}(" << quote(sys_call) << ") failed" << std::endl;
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
                        else
                        {
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
                    }
                    else if(funcName == "ent")
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @ent(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string ent = params[0];

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
                    else if(funcName == "faviconinclude") //checks for favicon include
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @faviconinclude(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string faviconPathStr = params[0];

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
                    else if(funcName == "cssinclude") //checks for css includes
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @cssinclude(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string cssPathStr = params[0];

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
                    else if(funcName == "imginclude") //checks for img includes
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @imginclude(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string imgPathStr = params[0];

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
                    else if(funcName == "jsinclude") //checks for js includes
                    {
                        if(params.size() != 1)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": @jsinclude(): expected 1 parameter, got " << params.size() << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        std::string jsPathStr = params[0];

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

                        std::string jsInclude="<script src='";
                        jsInclude += pathToJSFile.str();
                        jsInclude+="'></script>";

                        os << jsInclude;
                        if(indent)
                            indentAmount += into_whitespace(jsInclude);
                    }
                    else
                    {
                        os << "@";
                        linePos = sLinePos;
                        if(indent)
                            indentAmount += " ";
                        continue;
                    }
                }
            }
            else //regular character
            {
                if(inLine[linePos] == '\n')
                {
                    ++linePos;
                    indentAmount = baseIndentAmount;
                    if(codeBlockDepth)
                        os << "\n";
                    else
                        os << "\n" << baseIndentAmount;
                }
                else
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

int Parser::parse_replace(std::string& str,
                  const std::string& strType,
                  const Path& readPath,
                  const std::set<Path>& antiDepsOfReadPath,
                  const int& lineNo,
                  const std::string& callType,
                  std::ostream& eos)
{
    std::istringstream iss(str);
    std::ostringstream oss;

    std::string oldIndent = indentAmount;
    indentAmount = "";

    if(read_and_process(0, iss, Path("", strType), antiDepsOfReadPath, oss, eos) > 0)
    {
        os_mtx->lock();
        eos << "error: " << readPath << ": line " << lineNo << ": failed to parse " << strType << " string inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    str = oss.str();

    indentAmount = oldIndent;

    return 0;
}

int Parser::parse_replace(std::vector<std::string>& strs,
                  const std::string& strType,
                  const Path& readPath,
                  const std::set<Path>& antiDepsOfReadPath,
                  const int& lineNo,
                  const std::string& callType,
                  std::ostream& eos)
{
    std::string oldIndent = indentAmount;

    for(size_t s=0; s<strs.size(); s++)
    {
        std::istringstream iss(strs[s]);
        std::ostringstream oss;

        indentAmount = "";

        if(read_and_process(0, iss, Path("", strType), antiDepsOfReadPath, oss, eos) > 0)
        {
            os_mtx->lock();
            eos << "error: " << readPath << ": line " << lineNo << ": failed to parse " << strType << " string inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        strs[s] = oss.str();
    }

    indentAmount = oldIndent;

    return 0;
}

int Parser::read_func_name(std::string& funcName,
                          size_t& linePos,
                          std::string& inLine,
                          const Path& readPath,
                          int& lineNo,
                          std::ostream& os,
                          std::istream& is)
{
    int sLineNo = lineNo;
    std::string extraLine;
    funcName = "";

    //normal @
    if(linePos >= inLine.size())
        return 0;

    //reads function name
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(; inLine[linePos] != '\'';)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                funcName += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                funcName += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            funcName += inLine[linePos];

            ++linePos;

            /*if(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                funcName += " ";*/
            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
            {
                if(!getline(is, extraLine))
                {
                    os_mtx->lock();
                    if(sLineNo == lineNo)
                        os << "error: " << readPath << ": line " << lineNo << ": no close ' for function name in '" << funcName << "..()' call" << std::endl;
                    else
                        os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close ' for function name in '" << funcName << "..()' call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                ++linePos;
                ++lineNo;

                //skips over hopefully trailing whitespace
                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                    ++linePos;
            }
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; inLine[linePos] != '"';)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                funcName += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                funcName += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            funcName += inLine[linePos];

            ++linePos;

            /*if(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                funcName += " ";*/
            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
            {
                if(!getline(is, extraLine))
                {
                    os_mtx->lock();
                    if(sLineNo == lineNo)
                        os << "error: " << readPath << ": line " << lineNo << ": no close \" for function name in '" << funcName << "..()' call" << std::endl;
                    else
                        os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close \" for function name in '" << funcName << "..()' call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                ++linePos;
                ++lineNo;

                //skips over hopefully trailing whitespace
                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                    ++linePos;
            }
        }
        ++linePos;
    }
    else
    {
        for(; linePos < inLine.size() && inLine[linePos] != ' ' && inLine[linePos] != '{' && inLine[linePos] != '(' && inLine[linePos] != '[' && inLine[linePos] != '<'; ++linePos)
        {
            if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                funcName += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 't')
            {
                funcName += '\t';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\\')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            funcName += inLine[linePos];
        }
        strip_trailing_whitespace(funcName);
    }

    //normal @
    //nothing to do
    /*if(linePos >= inLine.size() || (inLine[linePos] != '{' && inLine[linePos] != '(')
    {
    }*/

    return 0;
}

int Parser::read_def(std::string& varType,
                           std::vector<std::pair<std::string, std::string> >& vars,
                           size_t& linePos,
                           std::string& inLine,
                           const Path& readPath,
                           int& lineNo,
                           const std::string& callType,
                           std::ostream& os,
                           std::istream& is)
{
    int sLineNo = lineNo;
    std::string extraLine;
    varType = "";
    vars.clear();
    std::pair<std::string, std::string> newVar("", "");

    ++linePos;

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
    {
        if(!getline(is, extraLine))
        {
            os_mtx->lock();
            if(sLineNo == lineNo)
                os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            else
                os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        inLine = inLine.substr(0, linePos) + "\n" + extraLine;
        ++linePos;
        ++lineNo;

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;
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
            else if(depth == 1 && inLine[linePos] == ',')
                break;

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

    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
    {
        if(!getline(is, extraLine))
        {
            os_mtx->lock();
            if(sLineNo == lineNo)
                os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            else
                os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        inLine = inLine.substr(0, linePos) + "\n" + extraLine;
        ++linePos;
        ++lineNo;

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;
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

        while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
        {
            if(!getline(is, extraLine))
            {
                os_mtx->lock();
                if(sLineNo == lineNo)
                    os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                else
                    os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            inLine = inLine.substr(0, linePos) + "\n" + extraLine;
            ++linePos;
            ++lineNo;

            //skips over hopefully trailing whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                ++linePos;
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
                else if(depth && (inLine[linePos] == ',' || inLine[linePos] == '='))
                    break;

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

        while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
        {
            if(!getline(is, extraLine))
            {
                os_mtx->lock();
                if(sLineNo == lineNo)
                    os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                else
                    os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            inLine = inLine.substr(0, linePos) + "\n" + extraLine;
            ++linePos;
            ++lineNo;

            //skips over hopefully trailing whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                ++linePos;
        }

        if(linePos < inLine.size() && inLine[linePos] == '=') //reads variable value
        {
            linePos++;

            //skips over whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
                ++linePos;

            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
            {
                if(!getline(is, extraLine))
                {
                    os_mtx->lock();
                    if(sLineNo == lineNo)
                        os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                    else
                        os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                ++linePos;
                ++lineNo;

                //skips over hopefully trailing whitespace
                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                    ++linePos;
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
                    else if(depth == 1 && inLine[linePos] == ',')
                        break;

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

            while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
            {
                if(!getline(is, extraLine))
                {
                    os_mtx->lock();
                    if(sLineNo == lineNo)
                        os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                    else
                        os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                ++linePos;
                ++lineNo;

                //skips over hopefully trailing whitespace
                while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                    ++linePos;
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

int Parser::read_params(std::vector<std::string>& params,
                          size_t& linePos,
                          std::string& inLine,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& os,
                          std::istream& is)
{
    int sLineNo = lineNo;
    char closeBracket, openBracket;
    std::string param = "", extraLine;

    if(inLine[linePos] == '<')
    {
        openBracket = '<';
        closeBracket = '>';
    }
    else if(inLine[linePos] == '(')
    {
        openBracket = '(';
        closeBracket = ')';
    }
    else if(inLine[linePos] == '[')
    {
        openBracket = '[';
        closeBracket = ']';
    }
    else if(inLine[linePos] == '{')
    {
        openBracket = '{';
        closeBracket = '}';
    }
    else
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": could not determine open bracket type with " << callType << " call" << std::endl;
        os << "note: make sure read_params is called with linePos at the open bracket" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    do
    {
        param = "";

        ++linePos;

        //skips over leading whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;

        //no parameters
        if(linePos < inLine.size() && inLine[linePos] == closeBracket && params.size() == 0)
        {
            ++linePos;
            return 0;
        }

        while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
        {
            if(!getline(is, extraLine))
            {
                os_mtx->lock();
                if(sLineNo == lineNo)
                    os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                else
                    os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            inLine = inLine.substr(0, linePos) + "\n" + extraLine;
            ++linePos;
            ++lineNo;

            //skips over hopefully trailing whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                ++linePos;
        }

        //reads parameter
        if(inLine[linePos] == '\'')
        {
            ++linePos;
            for(; inLine[linePos] != '\'';)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size())
                {
                    if(inLine[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos+1] == '"')
                        linePos++;
                }
                param += inLine[linePos];

                ++linePos;

                /*if(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    param += " ";*/
                while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                {
                    if(!getline(is, extraLine))
                    {
                        os_mtx->lock();
                        if(sLineNo == lineNo)
                            os << "error: " << readPath << ": line " << lineNo << ": no close ' for option/parameter in " << callType << " call" << std::endl;
                        else
                            os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close ' for option/parameter in " << callType << " call" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                    ++linePos;
                    ++lineNo;

                    //skips over hopefully trailing whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                        ++linePos;
                }
            }
            ++linePos;
        }
        else if(inLine[linePos] == '"')
        {
            ++linePos;
            for(; inLine[linePos] != '"';)
            {
                if(inLine[linePos] == '\\' && linePos+1 < inLine.size())
                {
                    if(inLine[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos+1] == '"')
                        linePos++;
                }
                param += inLine[linePos];

                ++linePos;

                /*if(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    param += " ";*/
                while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                {
                    if(!getline(is, extraLine))
                    {
                        os_mtx->lock();
                        if(sLineNo == lineNo)
                            os << "error: " << readPath << ": line " << lineNo << ": no close \" for option/parameter in " << callType << " call" << std::endl;
                        else
                            os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close \" for option/parameter in " << callType << " call" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                    ++linePos;
                    ++lineNo;

                    //skips over hopefully trailing whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                        ++linePos;
                }
            }
            ++linePos;
        }
        else
        {
            int depth = 1;

            for(;;)
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
                else if(depth == 1 && inLine[linePos] == ',')
                    break;

                if(inLine[linePos] == '\\' && linePos+1 < inLine.size())
                {
                    if(inLine[linePos+1] == 'n')
                    {
                        param += '\n';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == 't')
                    {
                        param += '\t';
                        linePos+=2;
                        continue;
                    }
                    else if(inLine[linePos+1] == '\\')
                        linePos++;
                    else if(inLine[linePos+1] == '\'')
                        linePos++;
                    else if(inLine[linePos+1] == '"')
                        linePos++;
                }
                param += inLine[linePos];

                ++linePos;

                /*if(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                    param += " ";*/
                /*else if(inLine[linePos] == '\n')
                {
                    param += " ";
                    //skips over hopefully trailing whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                        ++linePos;
                }*/
                while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
                {
                    if(!getline(is, extraLine))
                    {
                        os_mtx->lock();
                        if(sLineNo == lineNo)
                            os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                        else
                            os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    inLine = inLine.substr(0, linePos) + "\n" + extraLine;
                    ++linePos;
                    ++lineNo;

                    //skips over hopefully trailing whitespace
                    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                        ++linePos;
                }
            }
            strip_trailing_whitespace(param);

            if(param == "") //throws error if missing parameter
            {
                os_mtx->lock();
                if(sLineNo == lineNo)
                    os << "error: " << readPath << ": line " << lineNo << ": option/parameter missing inside " << callType << " call" << std::endl;
                else
                    os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": option/parameter missing inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
        }

        params.push_back(param);

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;

        while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
        {
            if(!getline(is, extraLine))
            {
                os_mtx->lock();
                if(sLineNo == lineNo)
                    os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                else
                    os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            inLine = inLine.substr(0, linePos) + "\n" + extraLine;
            ++linePos;
            ++lineNo;

            //skips over hopefully trailing whitespace
            while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
                ++linePos;
        }
    }
    while(inLine[linePos] == ',');

    //don't actually need this as it's checked at end of do while loop
    /*if(linePos < inLine.size() && inLine[linePos] == '\n')
    {
        param += " ";
        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;
    }*/
    while(linePos >= inLine.size() || (inLine[linePos] == '@' && inLine.substr(linePos, 2) == "@#"))
    {
        if(!getline(is, extraLine))
        {
            os_mtx->lock();
            if(sLineNo == lineNo)
                os << "error: " << readPath << ": line " << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            else
                os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": no close bracket for " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        inLine = inLine.substr(0, linePos) + "\n" + extraLine;
        ++linePos;
        ++lineNo;

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t' || inLine[linePos] == '\n'))
            ++linePos;
    }

    //throws error if the parameter is invalid
    if(inLine[linePos] != closeBracket)
    {
        os_mtx->lock();
        if(sLineNo == lineNo)
            os << "error: " << readPath << ": line " << lineNo << ": invalid option/parameter for " << callType << " call" << std::endl;
        else
            os << "error: " << readPath << ": lines " << sLineNo << "-" << lineNo << ": invalid option/parameter for " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int Parser::read_options(std::vector<std::string>& options,
                          size_t& linePos,
                          std::string& inLine,
                          const Path& readPath,
                          int& lineNo,
                          const std::string& callType,
                          std::ostream& os,
                          std::istream& is)
{
    while(linePos < inLine.size() && inLine[linePos] == '{')
    {
        if(read_params(options, linePos, inLine, readPath, lineNo, callType, os, is))
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": failed to read options inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    //throws error if new line is between the options and parameters
    //actually, a normal @
    /*if(linePos >= inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no closing bracket ) for parameters or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }*/

    //throws error if there is no parameters following
    //actually, a normal @
    /*if(inLine[linePos] != '(')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no parameters after options inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }*/

    return 0;
}
