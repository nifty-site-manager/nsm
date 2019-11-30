#include "PageBuilder.h"

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

bool run_script(std::ostream& os, std::string scriptPath, std::mutex* os_mtx)
{
    if(std::ifstream(scriptPath))
    {
        size_t pos = scriptPath.substr(1, scriptPath.size()-1).find_first_of('.');
        std::string scriptExt = "";
        if(pos != std::string::npos)
            scriptExt = scriptPath.substr(pos+1, scriptPath.size()-pos-1);
        std::string execPath = "./.script" + std::to_string(sys_counter++) + scriptExt;
        std::string output_filename = ".script.out" + std::to_string(sys_counter++);
        //std::string output_filename = scriptPath + ".out" + std::to_string(sys_counter++);
        int result;

        #if defined _WIN32 || defined _WIN64
            if(unquote(execPath).substr(0, 2) == "./")
                execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
        #else  //unix
        #endif

        //copies script to backup location
        cpFile(scriptPath, scriptPath + ".backup");

        //moves script to main directory
        //note if just copy original script get 'Text File Busy' errors
        rename(scriptPath.c_str(), execPath.c_str());

        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
            result = system(("flatpak-spawn --host bash -c " + quote(execPath) + " > " + output_filename).c_str());
        else
            result = system((execPath + " > " + output_filename).c_str());

        //moves script back to its original location
        rename(execPath.c_str(), scriptPath.c_str());
        Path("", scriptPath + ".backup").removePath();

        std::ifstream ifxs(output_filename);
        std::string str;
        os_mtx->lock();
        while(getline(ifxs, str))
            os << str << std::endl;
        os_mtx->unlock();
        ifxs.close();
        Path("./", output_filename).removePath();

        if(result)
        {
            os_mtx->lock();
            os << "error: PageBuilder.cpp: run_script(" << quote(scriptPath) << "): script failed" << std::endl;
            os_mtx->unlock();
            return 1;
        }
    }

    return 0;
}

PageBuilder::PageBuilder(std::set<PageInfo>* Pages,
                         std::mutex* OS_mtx,
                         const Directory& ContentDir,
                         const Directory& SiteDir,
                         const std::string& ContentExt,
                         const std::string& PageExt,
                         const std::string& ScriptExt,
                         const Path& DefaultTemplate,
                         const std::string& UnixTextEditor,
                         const std::string& WinTextEditor)
{
    //sys_counter = 0;
    pages = Pages;
    os_mtx = OS_mtx;
    contentDir = ContentDir;
    siteDir = SiteDir;
    contentExt = ContentExt;
    pageExt = PageExt;
    scriptExt = ScriptExt;
    defaultTemplate = DefaultTemplate;
    unixTextEditor = UnixTextEditor;
    winTextEditor = WinTextEditor;
}

int PageBuilder::build(const PageInfo &PageToBuild, std::ostream& os)
{
    sys_counter = sys_counter%1000000000000000000;
    pageToBuild = PageToBuild;

    //os_mtx->lock();
    //os << std::endl;
    //os_mtx->unlock();

    //ensures content and template files exist
    if(!std::ifstream(pageToBuild.contentPath.str()))
    {
        os_mtx->lock();
        os << "error: cannot build " << pageToBuild.pagePath << " as content file " << pageToBuild.contentPath << " does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    if(!std::ifstream(pageToBuild.templatePath.str()))
    {
        os_mtx->lock();
        os << "error: cannot build " << pageToBuild.pagePath << " as template file " << pageToBuild.templatePath << " does not exist." << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //checks for non-default script extension
    Path extPath = pageToBuild.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

    std::string pageScriptExt;
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, pageScriptExt);
        ifs.close();
    }
    else
        pageScriptExt = scriptExt;

    //checks for pre-build scripts
    Path prebuildScript = pageToBuild.contentPath;
    prebuildScript.file = prebuildScript.file.substr(0, prebuildScript.file.find_first_of('.')) + "-pre-build" + pageScriptExt;
    if(run_script(os, prebuildScript.str(), os_mtx))
        return 1;

    //os_mtx->lock();
    //os << "building page " << pageToBuild.pagePath << std::endl;
    //os_mtx->unlock();

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    processedPage.clear();
    processedPage.str(std::string());
    pageDeps.clear();
    strings.clear();
    contentAdded = 0;

    //adds content and template paths to dependencies
    pageDeps.insert(pageToBuild.contentPath);
    pageDeps.insert(pageToBuild.templatePath);

    //opens up template file to start parsing from
    std::ifstream ifs(pageToBuild.templatePath.str());

    //creates anti-deps of page template set
    std::set<Path> antiDepsOfReadPath;

    //starts read_and_process from templatePath
    int result = read_and_process(1, ifs, pageToBuild.templatePath, antiDepsOfReadPath, processedPage, os);

    ifs.close();

    if(result == 0)
    {
        //ensures @inputcontent was found inside template dag
        if(!contentAdded)
        {
            os_mtx->lock();
            os << "error: content file " << pageToBuild.contentPath << " has not been used as a dependency within template file " << pageToBuild.templatePath << " or any of its dependencies" << std::endl;
            os_mtx->unlock();
            return 1;
        }

        //makes sure page file exists
        pageToBuild.pagePath.ensurePathExists();

        //makes sure we can write to page file
        chmod(pageToBuild.pagePath.str().c_str(), 0644);

        //writes processed page to page file
        std::ofstream pageStream(pageToBuild.pagePath.str());
        pageStream << processedPage.str() << "\n";
        pageStream.close();

        //makes sure user can't accidentally write to page file
        chmod(pageToBuild.pagePath.str().c_str(), 0444);

        //gets path for storing page information
        Path pageInfoPath = pageToBuild.pagePath.getInfoPath();

        //makes sure page info file exists
        pageInfoPath.ensurePathExists();

        //makes sure we can write to info file_
        chmod(pageInfoPath.str().c_str(), 0644);

        //writes page info file
        std::ofstream infoStream(pageInfoPath.str());
        infoStream << dateTimeInfo.currentTime() << " " << dateTimeInfo.currentDate() << "\n";
        infoStream << this->pageToBuild << "\n\n";
        for(auto pageDep=pageDeps.begin(); pageDep != pageDeps.end(); pageDep++)
            infoStream << *pageDep << "\n";
        infoStream.close();

        //makes sure user can't accidentally write to info file
        chmod(pageInfoPath.str().c_str(), 0444);

        //os_mtx->lock();
        //os << "page build successful" << std::endl;
        //os_mtx->unlock();
    }

    //checks for post-build scripts
    Path postbuildScripts = pageToBuild.contentPath;
    postbuildScripts.file = postbuildScripts.file.substr(0, postbuildScripts.file.find_first_of('.')) + "-post-build" + pageScriptExt;
    if(run_script(os, postbuildScripts.str(), os_mtx))
        return 1; //should a page be listed as failing to build if the post-build script fails?

    return result;
}

//parses istream 'is' whilst writing processed version to ostream 'os' with error ostream 'eos'
int PageBuilder::read_and_process(const bool& indent,
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
    if(!indent) // not sure if this is needed?
        baseIndentAmount = "";
    int baseCodeBlockDepth = codeBlockDepth;

    int lineNo = 0,
        openCodeLineNo = 0;
    std::string inLine;
    while(getline(is, inLine))
    {
        lineNo++;

        if(lineNo > 1)
        {
            indentAmount = baseIndentAmount;
            if(codeBlockDepth)
                os << "\n";
            else
                os << "\n" << baseIndentAmount;
        }

        for(size_t linePos=0; linePos<inLine.length();)
        {
            if(inLine[linePos] == '\\') //checks whether to escape
            {
                linePos++;
                /*
                    see http://dev.w3.org/html5/html-author/charref for
                    more character references than you could ever need!
                */
                switch(inLine[linePos])
                {
                    case '~':
                        os << "&tilde;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '!':
                        os << "&excl;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(6, ' ');
                        break;
                    case '@':
                        os << "&commat;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '#':
                        os << "&num;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    /*case '$': //MUST HAVE MATHJAX HANDLE THIS
                        os << "&dollar;";
                        linePos++;
                        indentAmount += string(8, ' ');
                        break;*/
                    case '%':
                        os << "&percnt;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(8, ' ');
                        break;
                    case '^':
                        os << "&Hat;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    /*case '&': //SEEMS TO BREAK SOME VALID JAVASCRIPT CODE
                                //CHECK DEVELOPERS PERSONAL SITE GENEALOGY PAGE
                                //FOR EXAMPLE (SEARCH FOR \&)
                        os << "&amp;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;*/
                    case '*':
                        os << "&ast;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(5, ' ');
                        break;
                    case '?':
                        os << "&quest;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(7, ' ');
                        break;
                    case '<':
                        os << "&lt;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(4, ' ');
                        break;
                    case '>':
                        os << "&gt;";
                        linePos++;
                        if(indent)
                            indentAmount += std::string(4, ' ');
                        break;
                    default:
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
                    if(codeBlockDepth < baseCodeBlockDepth)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }

                //checks whether to escape <
                if(codeBlockDepth > 0 && inLine.substr(linePos+1, 4) != "code" && inLine.substr(linePos+1, 5) != "/code")
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

                os << '-';
                if(indent)
                    indentAmount += " ";
                linePos++;
            }
            else if(inLine[linePos] == '@') //checks for commands
            {
                if(linePos > 0 && inLine[linePos-1] == '\\')
                {
                    os << '@';
                    if(indent)
                        indentAmount += " ";
                    linePos++;
                }
                else if(inLine.substr(linePos, 11) == "@rawcontent")
                {
                    contentAdded = 1;
                    std::string replaceText = "@inputraw(" + quote(pageToBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 11, replaceText);
                }
                else if(inLine.substr(linePos, 13) == "@inputcontent")
                {
                    contentAdded = 1;
                    std::string replaceText = "@input(" + quote(pageToBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 13, replaceText);
                }
                else if(inLine.substr(linePos, 10) == "@inputhead")
                {
                    Path headPath = pageToBuild.contentPath;
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

                    std::string contExtPath = ".siteinfo/" + siteDir + pageToBuild.pageName + ".ext";

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

                    Path("", output_filename).removePath();
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
                    pageDeps.insert(inputPath);

                    if(inputPath == pageToBuild.contentPath)
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
                    pageDeps.insert(inputPath);

                    if(inputPath == pageToBuild.contentPath)
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
                    pageDeps.insert(depPath);

                    if(depPath == pageToBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(depPathStr))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @dep(" << quote(depPathStr) << ") failed as dependency does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 8) == "@script(" || inLine.substr(linePos, 9) == "@script*(")
                {
                    linePos+=std::string("@script").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

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
                    std::string scriptExt = "";
                    if(pos != std::string::npos)
                        scriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(sys_counter++) + scriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    if(unquote(scriptPathStr).substr(0, 2) == "./")
                        scriptPath.set_file_path_from(unquote(unquote(scriptPathStr).substr(2, unquote(scriptPathStr).size()-2)));
                    else
                        scriptPath.set_file_path_from(unquote(scriptPathStr));
                    pageDeps.insert(scriptPath);

                    if(!std::ifstream(scriptPath.str()))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    cpFile(scriptPathStr, scriptPathStr + ".backup");

                    //moves script to main directory
                    rename(scriptPathStr.c_str(), execPath.c_str());

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    rename(execPath.c_str(), scriptPathStr.c_str());
                    Path("", scriptPathStr + ".backup").removePath();

                    std::ifstream ifs(output_filename);
                    std::string str;
                    os_mtx->lock();
                    while(getline(ifs, str))
                        eos << str << std::endl;
                    os_mtx->unlock();
                    ifs.close();

                    Path("./", output_filename).removePath();

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(scriptPathStr) << ") failed" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 11) == "@scriptraw(" || inLine.substr(linePos, 12) == "@scriptraw*(")
                {
                    linePos+=std::string("@scriptraw").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

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
                    std::string scriptExt = "";
                    if(pos != std::string::npos)
                        scriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(sys_counter++) + scriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    if(unquote(scriptPathStr).substr(0, 2) == "./")
                        scriptPath.set_file_path_from(unquote(unquote(scriptPathStr).substr(2, unquote(scriptPathStr).size()-2)));
                    else
                        scriptPath.set_file_path_from(unquote(scriptPathStr));
                    pageDeps.insert(scriptPath);

                    if(scriptPath == pageToBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(scriptPath.str()))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    cpFile(scriptPathStr, scriptPathStr + ".backup");

                    //moves script to main directory
                    rename(scriptPathStr.c_str(), execPath.c_str());

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    rename(execPath.c_str(), scriptPathStr.c_str());
                    Path("", scriptPathStr + ".backup").removePath();

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //Path("./", output_filename).removePath();
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

                    Path("./", output_filename).removePath();
                }
                else if(inLine.substr(linePos, 14) == "@scriptoutput(" || inLine.substr(linePos, 15) == "@scriptoutput*(")
                {
                    linePos+=std::string("@scriptoutput").length();
                    std::string scriptPathStr, scriptParams;
                    std::string output_filename = ".@scriptoutput" + std::to_string(sys_counter++);

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

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
                    std::string scriptExt = "";
                    if(pos != std::string::npos)
                        scriptExt = scriptPathStr.substr(pos+1, scriptPathStr.size()-pos-1);
                    std::string execPath = "./.script" + std::to_string(sys_counter++) + scriptExt;

                    #if defined _WIN32 || defined _WIN64
                        if(unquote(execPath).substr(0, 2) == "./")
                            execPath = unquote(execPath).substr(2, unquote(execPath).size()-2);
                    #else  //unix
                    #endif

                    Path scriptPath;
                    if(unquote(scriptPathStr).substr(0, 2) == "./")
                        scriptPath.set_file_path_from(unquote(unquote(scriptPathStr).substr(2, unquote(scriptPathStr).size()-2)));
                    else
                        scriptPath.set_file_path_from(unquote(scriptPathStr));
                    pageDeps.insert(scriptPath);

                    if(scriptPath == pageToBuild.contentPath)
                        contentAdded = 1;

                    if(!std::ifstream(scriptPath.str()))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed as script does not exist" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }

                    //copies script to backup location
                    cpFile(scriptPathStr, scriptPathStr + ".backup");

                    //moves script to main directory
                    rename(scriptPathStr.c_str(), execPath.c_str());

                    //checks whether we're running from flatpak
                    int result;
                    if(std::ifstream("/.flatpak-info"))
                        result = system(("flatpak-spawn --host bash -c " + quote(execPath + " " + scriptParams) + " > " + output_filename).c_str());
                    else
                        result = system((execPath + " " + scriptParams + " > " + output_filename).c_str());

                    //moves script back to original location
                    rename(execPath.c_str(), scriptPathStr.c_str());
                    Path("", scriptPathStr + ".backup").removePath();

                    if(result)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(scriptPathStr) << ") failed" << std::endl;
                        eos << "       see " << quote(output_filename) << " for pre-error script output" << std::endl;
                        os_mtx->unlock();
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of script '" << scriptPathStr << "'" << std::endl;
                        os_mtx->unlock();
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    ifs.close();

                    Path("./", output_filename).removePath();
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

                    Path("./", output_filename).removePath();

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
                        //Path("./", output_filename).removePath();
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

                    Path("./", output_filename).removePath();
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
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of system call '" << sys_call << "'" << std::endl;
                        os_mtx->unlock();
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    ifs.close();

                    Path("./", output_filename).removePath();
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
                    sys_call += " " + quote(pageToBuild.contentPath.str());

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
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    std::ifstream ifs(output_filename);

                    //indent amount updated inside read_and_process
                    if(read_and_process(1, ifs, Path("", output_filename), antiDepsOfReadPath, os, eos) > 0)
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": failed to process output of system call '" << sys_call << "'" << std::endl;
                        os_mtx->unlock();
                        //Path("./", output_filename).removePath();
                        return 1;
                    }

                    ifs.close();

                    Path("./", output_filename).removePath();
                }
                else if(inLine.substr(linePos,11) == "@stringdef(" || inLine.substr(linePos,12) == "@stringdef*(")
                {
                    linePos+=std::string("@stringdef").length();
                    std::string varName, varVal;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_stringdef(varName, varVal, linePos, inLine, readPath, lineNo, "@stringdef()", eos))
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(varName);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "variable name"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        varName = oss.str();

                        iss = std::istringstream(varVal);
                        oss.str("");
                        oss.clear();

                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "variable value"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        varVal = oss.str();

                        indentAmount = oldIndent;
                    }

                    if(strings.count(varName))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": redeclaration of string('" << varName << "')" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                    else
                        strings[varName] = varVal;
                }
                else if(inLine.substr(linePos, 8) == "@string(" || inLine.substr(linePos, 9) == "@string*(")
                {
                    linePos+=std::string("@string").length();
                    std::string varName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_var(varName, linePos, inLine, readPath, lineNo, "@string()", eos))
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(varName);
                        oss.str("");
                        oss.clear();

                        std::string oldIndent = indentAmount;
                        indentAmount = "";

                        if(read_and_process(0, iss, Path("", "variable name"), antiDepsOfReadPath, oss, eos) > 0)
                        {
                            os_mtx->lock();
                            eos << "error: " << readPath << ": line " << lineNo << ": read_and_process() failed here" << std::endl;
                            os_mtx->unlock();
                            return 1;
                        }

                        varName = oss.str();

                        indentAmount = oldIndent;
                    }

                    if(strings.count(varName))
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


                        /*os << strings[varName];
                        if(indent)
                            indentAmount += into_whitespace(strings[varName]);*/
                    }
                    else
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": string('" << varName << "') was not declared in this scope" << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 8) == "@pathto(" || inLine.substr(linePos, 9) == "@pathto*(")
                {
                    linePos+=std::string("@pathto").length();
                    Name targetPageName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(targetPageName, linePos, inLine, readPath, lineNo, "@pathto()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(targetPageName);
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

                        targetPageName = oss.str();

                        indentAmount = oldIndent;
                    }

                    //throws error if target targetPageName isn't being tracked by Nift
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages->count(targetPageInfo))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetPageName << ") failed, Nift not tracking " << targetPageName << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }


                    Path targetPath = pages->find(targetPageInfo)->pagePath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    os << pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(inLine.substr(linePos, 12) == "@pathtopage(" || inLine.substr(linePos, 13) == "@pathtopage*(")
                {
                    linePos+=std::string("@pathtopage").length();
                    Name targetPageName;

                    if(inLine[linePos] == '*')
                    {
                        parseParams = 1;
                        linePos++;
                    }
                    else
                        parseParams = 0;

                    linePos++;

                    if(read_path(targetPageName, linePos, inLine, readPath, lineNo, "@pathtopage()", eos) > 0)
                        return 1;

                    if(parseParams)
                    {
                        std::istringstream iss(targetPageName);
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

                        targetPageName = oss.str();

                        indentAmount = oldIndent;
                    }

                    //throws error if target targetPageName isn't being tracked by Nift
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages->count(targetPageInfo))
                    {
                        os_mtx->lock();
                        eos << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetPageName << ") failed, Nift not tracking " << targetPageName << std::endl;
                        os_mtx->unlock();
                        return 1;
                    }


                    Path targetPath = pages->find(targetPageInfo)->pagePath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

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

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    os << pathToTarget.str();
                    if(indent)
                        indentAmount += into_whitespace(pathToTarget.str());
                }
                else if(inLine.substr(linePos, 10) == "@pagetitle")
                {
                    os << pageToBuild.pageTitle.str;
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.pageTitle.str);
                    linePos += std::string("@pagetitle").length();
                }
                else if(inLine.substr(linePos, 11) == "@page-title")
                {
                    os << pageToBuild.pageTitle.str;
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.pageTitle.str);
                    linePos += std::string("@page-title").length();
                }
                else if(inLine.substr(linePos, 9) == "@pagename")
                {
                    os << pageToBuild.pageName;
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.pageName);
                    linePos += std::string("@pagename").length();
                }
                else if(inLine.substr(linePos, 9) == "@pagepath")
                {
                    os << pageToBuild.pagePath.str();
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.pagePath.str());
                    linePos += std::string("@pagepath").length();
                }
                else if(inLine.substr(linePos, 12) == "@pagepageext")
                {
                    //checks for non-default page extension
                    Path extPath = pageToBuild.pagePath.getInfoPath();
                    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";

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
                        os << pageExt;
                        if(indent)
                            indentAmount += into_whitespace(pageExt);
                    }

                    linePos += std::string("@pagepageext").length();
                }
                else if(inLine.substr(linePos, 12) == "@contentpath")
                {
                    os << pageToBuild.contentPath.str();
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.contentPath.str());
                    linePos += std::string("@contentpath").length();
                }
                else if(inLine.substr(linePos, 15) == "@pagecontentext")
                {
                    //checks for non-default content extension
                    Path extPath = pageToBuild.pagePath.getInfoPath();
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

                    linePos += std::string("@pagecontentext").length();
                }
                else if(inLine.substr(linePos, 14) == "@pagescriptext")
                {
                    //checks for non-default script extension
                    Path extPath = pageToBuild.pagePath.getInfoPath();
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

                    linePos += std::string("@pagescriptext").length();
                }
                else if(inLine.substr(linePos, 13) == "@templatepath")
                {
                    os << pageToBuild.templatePath.str();
                    if(indent)
                        indentAmount += into_whitespace(pageToBuild.templatePath.str());
                    linePos += std::string("@templatepath").length();
                }
                else if(inLine.substr(linePos, 11) == "@contentdir")
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
                    linePos += std::string("@contentdir").length();
                }
                else if(inLine.substr(linePos, 8) == "@sitedir")
                {
                    if(siteDir.size() && (siteDir[siteDir.size()-1] == '/' || siteDir[siteDir.size()-1] == '\\'))
                    {
                        os << siteDir.substr(0, siteDir.size()-1);
                        if(indent)
                            indentAmount += into_whitespace(siteDir.substr(0, siteDir.size()-1));
                    }
                    else
                    {
                        os << siteDir;
                        if(indent)
                            indentAmount += into_whitespace(siteDir);
                    }
                    linePos += std::string("@sitedir").length();
                }
                else if(inLine.substr(linePos, 11) == "@contentext")
                {
                    os << contentExt;
                    if(indent)
                        indentAmount += into_whitespace(contentExt);
                    linePos += std::string("@contentext").length();
                }
                else if(inLine.substr(linePos, 8) == "@pageext")
                {
                    os << pageExt;
                    if(indent)
                        indentAmount += into_whitespace(pageExt);
                    linePos += std::string("@pageExt").length();
                }
                else if(inLine.substr(linePos, 10) == "@scriptext")
                {
                    os << scriptExt;
                    if(indent)
                        indentAmount += into_whitespace(scriptExt);
                    linePos += std::string("@scriptExt").length();
                }
                else if(inLine.substr(linePos, 16) == "@defaulttemplate")
                {
                    os << defaultTemplate.str();
                    if(indent)
                        indentAmount += into_whitespace(defaultTemplate.str());
                    linePos += std::string("@defaulttemplate").length();
                }
                else if(inLine.substr(linePos, 14) == "@buildtimezone")
                {
                    os << dateTimeInfo.cTimezone;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cTimezone);
                    linePos += std::string("@buildtimezone").length();
                }
                else if(inLine.substr(linePos, 13) == "@loadtimezone")
                {
                    os << "<script>document.write(new Date().toString().split(\"(\")[1].split(\")\")[0])</script>";
                    if(indent)
                        indentAmount += std::string(82, ' ');
                    linePos += std::string("@loadtimezone").length();
                }
                else if(inLine.substr(linePos, 9) == "@timezone")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.cTimezone;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cTimezone);
                    linePos += std::string("@timezone").length();
                }
                else if(inLine.substr(linePos, 10) == "@buildtime")
                {
                    os << dateTimeInfo.cTime;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cTime);
                    linePos += std::string("@buildtime").length();
                }
                else if(inLine.substr(linePos, 13) == "@buildUTCtime")
                {
                    os << dateTimeInfo.currentUTCTime();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
                    linePos += std::string("@buildUTCtime").length();
                }
                else if(inLine.substr(linePos, 10) == "@builddate")
                {
                    os << dateTimeInfo.cDate;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cDate);
                    linePos += std::string("@builddate").length();
                }
                else if(inLine.substr(linePos, 13) == "@buildUTCdate")
                {
                    os << dateTimeInfo.currentUTCDate();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
                    linePos += std::string("@buildUTCdate").length();
                }
                else if(inLine.substr(linePos, 12) == "@currenttime")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.cTime;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cTime);
                    linePos += std::string("@currenttime").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCtime")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.currentUTCTime();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentUTCTime());
                    linePos += std::string("@currentUTCtime").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentdate")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.cDate;
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.cDate);
                    linePos += std::string("@currentdate").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCdate")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.currentUTCDate();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentUTCDate());
                    linePos += std::string("@currentUTCdate").length();
                }
                else if(inLine.substr(linePos, 9) == "@loadtime")
                {
                    os << "<script>document.write((new Date().toLocaleString()).split(\",\")[1])</script>";
                    if(indent)
                        indentAmount += std::string(76, ' ');
                    linePos += std::string("@loadtime").length();
                }
                else if(inLine.substr(linePos, 12) == "@loadUTCtime")
                {
                    os << "<script>document.write((new Date().toISOString()).split(\"T\")[1].split(\".\")[0])</script>";
                    if(indent)
                        indentAmount += std::string(87, ' ');
                    linePos += std::string("@loadUTCtime").length();
                }
                else if(inLine.substr(linePos, 9) == "@loaddate")
                {
                    os << "<script>document.write((new Date().toLocaleString()).split(\",\")[0])</script>";
                    if(indent)
                        indentAmount += std::string(76, ' ');
                    linePos += std::string("@loaddate").length();
                }
                else if(inLine.substr(linePos, 12) == "@loadUTCdate")
                {
                    os << "<script>document.write((new Date().toISOString()).split(\"T\")[0])</script>";
                    if(indent)
                        indentAmount += std::string(73, ' ');
                    linePos += std::string("@loadUTCdate").length();
                }
                else if(inLine.substr(linePos, 10) == "@buildYYYY")
                {
                    os << dateTimeInfo.currentYYYY();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
                    linePos += std::string("@buildYYYY").length();
                }
                else if(inLine.substr(linePos, 8) == "@buildYY")
                {
                    os << dateTimeInfo.currentYY();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentYY());
                    linePos += std::string("@buildYY").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentYYYY")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.currentYYYY();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentYYYY());
                    linePos += std::string("@currentYYYY").length();
                }
                else if(inLine.substr(linePos, 10) == "@currentYY")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.currentYY();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentYY());
                    linePos += std::string("@currentYY").length();
                }
                else if(inLine.substr(linePos, 9) == "@loadYYYY")
                {
                    os << "<script>document.write(new Date().getFullYear())</script>";
                    if(indent)
                        indentAmount += std::string(57, ' ');
                    linePos += std::string("@loadYYYY").length();
                }
                else if(inLine.substr(linePos, 7) == "@loadYY")
                {
                    os << "<script>document.write(new Date().getFullYear()%100)</script>";
                    if(indent)
                        indentAmount += std::string(61, ' ');
                    linePos += std::string("@loadYY").length();
                }
                else if(inLine.substr(linePos, 8) == "@buildOS")
                {
                    os << dateTimeInfo.currentOS();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentOS());
                    linePos += std::string("@buildOS").length();
                }
                else if(inLine.substr(linePos, 10) == "@currentOS")
                { //this is left for backwards compatibility
                    os << dateTimeInfo.currentOS();
                    if(indent)
                        indentAmount += into_whitespace(dateTimeInfo.currentOS());
                    linePos += std::string("@currentOS").length();
                } //I'm not sure how to do loadOS sorry!
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

                    Path pathToFavicon(pathBetween(pageToBuild.pagePath.dir, faviconPath.dir), faviconPath.file);

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

                    Path pathToCSSFile(pathBetween(pageToBuild.pagePath.dir, cssPath.dir), cssPath.file);

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

                    Path pathToIMGFile(pathBetween(pageToBuild.pagePath.dir, imgPath.dir), imgPath.file);

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

                    Path pathToJSFile(pathBetween(pageToBuild.pagePath.dir, jsPath.dir), jsPath.file);

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

int PageBuilder::read_path(std::string& pathRead,
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
    if(linePos == inLine.size())
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
        for(; inLine[linePos] != '\''; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; inLine[linePos] != '"'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        //reads path value
        for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos == inLine.size())
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

int PageBuilder::read_script_params(std::string& pathRead,
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
    if(linePos == inLine.size())
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
        for(; inLine[linePos] != '\''; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": script path has no closing single quote or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; inLine[linePos] != '"'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": script path has no closing double quote \" or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        //reads path value
        for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": script path has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            pathRead += inLine[linePos];
        }
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos == inLine.size())
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
        if(linePos == inLine.size())
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
            for(; inLine[linePos] != '\''; ++linePos)
            {
                if(linePos == inLine.size())
                {
                    os_mtx->lock();
                    os << "error: " << readPath << ": line " << lineNo << ": script parameters have no closing single quote or newline inside " << callType << " call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                paramStr += inLine[linePos];
            }
            ++linePos;
        }
        else if(inLine[linePos] == '"')
        {
            ++linePos;
            for(; inLine[linePos] != '"'; ++linePos)
            {
                if(linePos == inLine.size())
                {
                    os_mtx->lock();
                    os << "error: " << readPath << ": line " << lineNo << ": script parameters have no closing double quote \" or newline inside " << callType << " call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                paramStr += inLine[linePos];
            }
            ++linePos;
        }
        else
        {
            //reads path value
            for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
            {
                if(linePos == inLine.size())
                {
                    os_mtx->lock();
                    os << "error: " << readPath << ": line " << lineNo << ": script parameters have no closing bracket ) or newline inside " << callType << " call" << std::endl;
                    os_mtx->unlock();
                    return 1;
                }
                paramStr += inLine[linePos];
            }
        }

        //skips over hopefully trailing whitespace
        while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
            ++linePos;

        //throws error if new line is between the path and close bracket
        if(linePos == inLine.size())
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

int PageBuilder::read_msg(std::string& msgRead,
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
    if(linePos == inLine.size())
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
        for(; inLine[linePos] != '\''; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": user input message has no closing single quote or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                msgRead += '\n';
                linePos++;
                continue;
            }
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
        for(; inLine[linePos] != '"'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": user input message has no closing double quote \" or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                msgRead += '\n';
                linePos++;
                continue;
            }
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
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": user input message should be quoted inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the path and close bracket
    if(linePos == inLine.size())
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

int PageBuilder::read_sys_call(std::string& sys_call,
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
    if(linePos == inLine.size())
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
        for(; inLine[linePos] != '\''; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing single quote or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
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
        for(; inLine[linePos] != '"'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing double quote \" or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
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
        for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            sys_call += inLine[linePos];
        }
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the system call and close bracket
    if(linePos == inLine.size())
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

int PageBuilder::read_stringdef(std::string& varName,
                                std::string& varVal,
                                size_t& linePos,
                                const std::string& inLine,
                                const Path& readPath,
                                const int& lineNo,
                                const std::string& callType,
                                std::ostream& os)
{
    varName = varVal = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline or no variable definition
    if(linePos == inLine.size() || inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no variable definition provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads variable name
    for(; inLine[linePos] != ')' && inLine[linePos] != '=' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
    {
        if(linePos == inLine.size())
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
            linePos++;
        else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
            linePos++;
        varName += inLine[linePos];
    }

    //skips over whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    if(linePos == inLine.size() || inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": incomplete variable definition inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    else if(inLine[linePos] != '=')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": variable definition has no = between variable name and value inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    linePos++;

    //skips over whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    if(linePos == inLine.size() || inLine[linePos] == ')')
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
        for(; inLine[linePos] != '\''; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing single quote or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varVal += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varVal += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(; inLine[linePos] != '"'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing double quote \" or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varVal += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varVal += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        //reads variable value
        for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx->lock();
                os << "error: " << readPath << ": line " << lineNo << ": variable definition has no closing bracket ) or newline inside " << callType << " call" << std::endl;
                os_mtx->unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == 'n')
            {
                varVal += '\n';
                linePos++;
                continue;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            varVal += inLine[linePos];
        }
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the variable definition and close bracket
    if(linePos == inLine.size())
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

int PageBuilder::read_var(std::string& varName,
                          size_t& linePos,
                          const std::string& inLine,
                          const Path& readPath,
                          const int& lineNo,
                          const std::string& callType,
                          std::ostream& os)
{
    varName = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline or no variable name
    if(linePos == inLine.size() || inLine[linePos] == ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": no variable name provided inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //reads variable name
    for(; inLine[linePos] != ')' && inLine[linePos] != ' ' && inLine[linePos] != '\t'; ++linePos)
    {
        if(linePos == inLine.size())
        {
            os_mtx->lock();
            os << "error: " << readPath << ": line " << lineNo << ": variable name has no closing bracket ) or newline inside " << callType << " call" << std::endl;
            os_mtx->unlock();
            return 1;
        }
        else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
            linePos++;
        else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
            linePos++;
        varName += inLine[linePos];
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the variable name and close bracket
    if(linePos == inLine.size())
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": variable name has no closing bracket ) or newline inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    //throws error if the variable name is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx->lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid variable name inside " << callType << " call" << std::endl;
        os_mtx->unlock();
        return 1;
    }

    ++linePos;

    return 0;
}
