#include "PageBuilder.h"

PageBuilder::PageBuilder(const std::set<PageInfo> &Pages)
{
    pages = Pages;
    counter = 0;
}

bool PageBuilder::run_page_prebuild_scripts(std::ostream& os)
{
    Path prebuildPath = pageToBuild.contentPath;
    prebuildPath.file = prebuildPath.file.substr(0, prebuildPath.file.find_first_of('.')) + ".pre-build.bat";
    if(std::ifstream(prebuildPath.str()))
    {
        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
        {
            int result = system(("flatpak-spawn --host bash -c \"'" + prebuildPath.str() + "'\" > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: pre build script " << quote(prebuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
        else //prebuildPath.str() needs to be quoted for page names with spaces
        {
            int result = system((quote(prebuildPath.str()) + " > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: pre build script " << quote(prebuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
    }
    prebuildPath = pageToBuild.contentPath;
    prebuildPath.file = prebuildPath.file.substr(0, prebuildPath.file.find_first_of('.')) + ".pre-build.sh";
    if(std::ifstream(prebuildPath.str()))
    {
        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
        {
            int result = system(("flatpak-spawn --host bash -c \"'" + prebuildPath.str() + "'\" > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: pre build script " << quote(prebuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
        else //prebuildPath.str() needs to be quoted for page names with spaces
        {
            int result = system((quote(prebuildPath.str()) + " > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: pre build script " << quote(prebuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
    }

    return 0;
}

bool PageBuilder::run_page_postbuild_scripts(std::ostream& os)
{
    Path postbuildPath = pageToBuild.contentPath;
    postbuildPath.file = postbuildPath.file.substr(0, postbuildPath.file.find_first_of('.')) + ".post-build.bat";
    if(std::ifstream(postbuildPath.str()))
    {
        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
        {
            int result = system(("flatpak-spawn --host bash -c \"'" + postbuildPath.str() + "'\" > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: post build script " << quote(postbuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
        else //postbuildPath.str() needs to be quoted for page names with spaces
        {
            int result = system((quote(postbuildPath.str()) + " > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: post build script " << quote(postbuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
    }
    postbuildPath = pageToBuild.contentPath;
    postbuildPath.file = postbuildPath.file.substr(0, postbuildPath.file.find_first_of('.')) + ".post-build.sh";
    if(std::ifstream(postbuildPath.str()))
    {
        //checks whether we're running from flatpak
        if(std::ifstream("/.flatpak-info"))
        {
            int result = system(("flatpak-spawn --host bash -c \"'" + postbuildPath.str() + "'\" > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: post build script " << quote(postbuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
        else //postbuildPath.str() needs to be quoted for page names with spaces
        {
            int result = system((quote(postbuildPath.str()) + " > .out.txt").c_str());

            std::ifstream ifs(".out.txt");
            std::string str;
            os_mtx.lock();
            while(getline(ifs, str))
                os << str << std::endl;
            os_mtx.unlock();
            ifs.close();
            Path("./", ".out.txt").removePath();

            if(result)
            {
                os_mtx.lock();
                os << "error: post build script " << quote(postbuildPath.str()) << " failed" << std::endl;
                os_mtx.unlock();
                return 1;
            }
        }
    }

    return 0;
}

int PageBuilder::build(const PageInfo &PageToBuild, std::ostream& os)
{
    counter = counter%1000000000000000000;
    pageToBuild = PageToBuild;

    //os_mtx.lock();
    //os << std::endl;
    //os_mtx.unlock();

    //ensures content and template files exist
    if(!std::ifstream(pageToBuild.contentPath.str()))
    {
        os_mtx.lock();
        os << "error: cannot build " << pageToBuild.pagePath << " as content file " << pageToBuild.contentPath << " does not exist" << std::endl;
        os_mtx.unlock();
        return 1;
    }
    if(!std::ifstream(pageToBuild.templatePath.str()))
    {
        os_mtx.lock();
        os << "error: cannot build " << pageToBuild.pagePath << " as template file " << pageToBuild.templatePath << " does not exist." << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //checks for pre-build scripts
    if(this->run_page_prebuild_scripts(os))
        return 1;

    //os_mtx.lock();
    //os << "building page " << pageToBuild.pagePath << std::endl;
    //os_mtx.unlock();

    //makes sure variables are at default values
    codeBlockDepth = htmlCommentDepth = 0;
    indentAmount = "";
    contentAdded = 0;
    processedPage.clear();
    processedPage.str(std::string());
    pageDeps.clear();
    contentAdded = 0;

    //adds content and template paths to dependencies
    pageDeps.insert(pageToBuild.contentPath);
    pageDeps.insert(pageToBuild.templatePath);

    //creates anti-deps of readPath set
    std::set<Path> antiDepsOfReadPath;

    //starts read_and_process from templatePath
    int result = read_and_process(pageToBuild.templatePath, antiDepsOfReadPath, os);

    if(result == 0)
    {
        //ensures @inputcontent was found inside template dag
        if(!contentAdded)
        {
            os_mtx.lock();
            os << "error: @inputcontent not found within template file " << pageToBuild.templatePath << " or any of its dependencies, content from " << pageToBuild.contentPath << " has not been inserted" << std::endl;
            os_mtx.unlock();
            return 1;
        }

        //makes sure page file exists
        pageToBuild.pagePath.ensurePathExists();

        //makes sure we can write to page file
        chmod(pageToBuild.pagePath.str().c_str(), 0644);

        //writes processed page to page file
        std::ofstream pageStream(pageToBuild.pagePath.str());
        pageStream << processedPage.str() << std::endl;
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
        infoStream << dateTimeInfo.currentTime() << " " << dateTimeInfo.currentDate() << std::endl;
        infoStream << this->pageToBuild << std::endl << std::endl;
        for(auto pageDep=pageDeps.begin(); pageDep != pageDeps.end(); pageDep++)
            infoStream << *pageDep << std::endl;
        infoStream.close();

        //makes sure user can't accidentally write to info file
        chmod(pageInfoPath.str().c_str(), 0444);

        //os_mtx.lock();
        //os << "page build successful" << std::endl;
        //os_mtx.unlock();
    }

    //checks for post-build scripts
    if(this->run_page_postbuild_scripts(os))
        return 1;

    return result;
}

//reads file whilst writing processed version to ofs
int PageBuilder::read_and_process(const Path &readPath, std::set<Path> antiDepsOfReadPath, std::ostream& os)
{
    //adds read path to anti dependencies of read path
    antiDepsOfReadPath.insert(readPath);

    std::ifstream ifs(readPath.str());
    std::string baseIndentAmount = indentAmount;
    int baseCodeBlockDepth = codeBlockDepth;

    int lineNo = 0,
        openCodeLineNo = 0;
    std::string inLine;
    while(getline(ifs, inLine))
    {
        lineNo++;

        if(lineNo > 1)
        {
            indentAmount = baseIndentAmount;
            if(codeBlockDepth)
                processedPage << std::endl;
            else
                processedPage << std::endl << baseIndentAmount;
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
                        processedPage << "&tilde;";
                        linePos++;
                        indentAmount += std::string(7, ' ');
                        break;
                    case '!':
                        processedPage << "&excl;";
                        linePos++;
                        indentAmount += std::string(6, ' ');
                        break;
                    case '@':
                        processedPage << "&commat;";
                        linePos++;
                        indentAmount += std::string(8, ' ');
                        break;
                    case '#':
                        processedPage << "&num;";
                        linePos++;
                        indentAmount += std::string(5, ' ');
                        break;
                    /*case '$': //MUST HAVE MATHJAX HANDLE THIS
                        processedPage << "&dollar;";
                        linePos++;
                        indentAmount += string(8, ' ');
                        break;*/
                    case '%':
                        processedPage << "&percnt;";
                        linePos++;
                        indentAmount += std::string(8, ' ');
                        break;
                    case '^':
                        processedPage << "&Hat;";
                        linePos++;
                        indentAmount += std::string(5, ' ');
                        break;
                    /*case '&': //SEEMS TO BREAK SOME VALID JAVASCRIPT CODE
                                //CHECK DEVELOPERS PERSONAL SITE GENEALOGY PAGE
                                //FOR EXAMPLE (SEARCH FOR \&)
                        processedPage << "&amp;";
                        linePos++;
                        indentAmount += std::string(5, ' ');
                        break;*/
                    case '*':
                        processedPage << "&ast;";
                        linePos++;
                        indentAmount += std::string(5, ' ');
                        break;
                    case '?':
                        processedPage << "&quest;";
                        linePos++;
                        indentAmount += std::string(7, ' ');
                        break;
                    case '<':
                        processedPage << "&lt;";
                        linePos++;
                        indentAmount += std::string(4, ' ');
                        break;
                    case '>':
                        processedPage << "&gt;";
                        linePos++;
                        indentAmount += std::string(4, ' ');
                        break;
                    default:
                        processedPage << "\\";
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
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }
                }

                //checks whether to escape <
                if(codeBlockDepth > 0 && inLine.substr(linePos+1, 4) != "code" && inLine.substr(linePos+1, 5) != "/code")
                {
                    processedPage << "&lt;";
                    indentAmount += std::string("&lt;").length();
                }
                else
                {
                    processedPage << '<';
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

                processedPage << '-';
                indentAmount += " ";
                linePos++;
            }
            else if(inLine[linePos] == '@') //checks for commands
            {
                if(linePos > 0 && inLine[linePos-1] == '\\')
                {
                    processedPage << '@';
                    indentAmount += " ";
                    linePos++;
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
                else if(inLine.substr(linePos, 7) == "@input(")
                {
                    linePos+=std::string("@input(").length();
                    std::string inputPathStr;

                    if(read_path(inputPathStr, linePos, inLine, readPath, lineNo, "@input()", os) > 0)
                        return 1;

                    Path inputPath;
                    inputPath.set_file_path_from(inputPathStr);
                    pageDeps.insert(inputPath);

                    //ensures insert file exists
                    if(!std::ifstream(inputPathStr))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " would result in an input loop" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }

                    //adds insert file
                    if(read_and_process(inputPath, antiDepsOfReadPath, os) > 0)
                        return 1;
                    //indent amount updated inside read_and_process
                }
                else if(inLine.substr(linePos, 5) == "@dep(")
                {
                    linePos+=std::string("@dep(").length();
                    std::string sys_call;

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@dep()", os) > 0)
                        return 1;

                    Path depPath;
                    depPath.set_file_path_from(sys_call);
                    pageDeps.insert(depPath);

                    if(!std::ifstream(sys_call))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @dep(" << quote(sys_call) << ") failed as dependency does not exist" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }
                }
                else if(inLine.substr(linePos, 8) == "@script(")
                {
                    linePos+=std::string("@script(").length();
                    std::string sys_call;
                    std::string output_filename = ".@scriptoutput" + std::to_string(counter++);

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@script()", os) > 0)
                        return 1;

                    Path scriptPath;
                    scriptPath.set_file_path_from(sys_call);
                    pageDeps.insert(scriptPath);

                    if(!std::ifstream(sys_call))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(sys_call) << ") failed as script does not exist" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }

                    //checks whether we're running from flatpak
                    if(std::ifstream("/.flatpak-info"))
                    {
                        int result = system(("flatpak-spawn --host bash -c \"'" + sys_call + "'\" > " + output_filename).c_str());

                        std::ifstream ifs(output_filename);
                        std::string str;
                        os_mtx.lock();
                        while(getline(ifs, str))
                            os << str << std::endl;
                        os_mtx.unlock();
                        ifs.close();
                        Path("./", output_filename).removePath();

                        //need sys_call quoted weirdly here for script paths containing spaces
                        if(result)
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            return 1;
                        }
                    }
                    else //sys_call has to be quoted for script paths containing spaces
                    {
                        int result = system((quote(sys_call) + " > " + output_filename).c_str());

                        std::ifstream ifs(output_filename);
                        std::string str;
                        os_mtx.lock();
                        while(getline(ifs, str))
                            os << str << std::endl;
                        os_mtx.unlock();
                        ifs.close();
                        Path("./", output_filename).removePath();

                        if(result)
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @script(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            return 1;
                        }
                    }
                }
                else if(inLine.substr(linePos, 14) == "@scriptoutput(")
                {
                    linePos+=std::string("@scriptoutput(").length();
                    std::string sys_call;
                    std::string output_filename = ".@scriptoutput" + std::to_string(counter++);

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@scriptoutput()", os) > 0)
                        return 1;

                    Path scriptPath;
                    scriptPath.set_file_path_from(sys_call);
                    pageDeps.insert(scriptPath);

                    if(!std::ifstream(sys_call))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(sys_call) << ") failed as script does not exist" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }

                    //checks whether we're running from flatpak
                    if(std::ifstream("/.flatpak-info"))
                    {
                        //need sys_call quoted weirdly here for script paths containing spaces
                        if(system(("flatpak-spawn --host bash -c \"'" + sys_call + "'\" > " + output_filename).c_str()))
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            Path("./", output_filename).removePath();
                            return 1;
                        }
                    }
                    else if(system((quote(sys_call) + " > " + output_filename).c_str())) //sys_call has to be quoted for script paths containing spaces
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @scriptoutput(" << quote(sys_call) << ") failed" << std::endl;
                        os_mtx.unlock();
                        Path("./", output_filename).removePath();
                        return 1;
                    }

                    //indent amount updated inside read_and_process
                    if(read_and_process(Path("", output_filename), antiDepsOfReadPath, os) > 0)
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": failed to process output of script '" << sys_call << "'" << std::endl;
                        os_mtx.unlock();
                        Path("./", output_filename).removePath();
                        return 1;
                    }

                    Path("./", output_filename).removePath();
                }
                else if(inLine.substr(linePos, 8) == "@system(")
                {
                    linePos+=std::string("@system(").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemoutput" + std::to_string(counter++);

                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@system()", os) > 0)
                        return 1;

                    //checks whether we're running from flatpak
                    if(std::ifstream("/.flatpak-info"))
                    {
                        int result = system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str());

                        std::ifstream ifs(output_filename);
                        std::string str;
                        os_mtx.lock();
                        while(getline(ifs, str))
                            os << str << std::endl;
                        os_mtx.unlock();
                        ifs.close();
                        Path("./", output_filename).removePath();

                        //need sys_call quoted here for cURL to work
                        if(result)
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @system(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            return 1;
                        }
                    }
                    else //sys_call has to be unquoted for cURL to work
                    {
                        int result = system((sys_call + " > " + output_filename).c_str());

                        std::ifstream ifs(output_filename);
                        std::string str;
                        os_mtx.lock();
                        while(getline(ifs, str))
                            os << str << std::endl;
                        os_mtx.unlock();
                        ifs.close();
                        Path("./", output_filename).removePath();

                        if(result)
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @system(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            return 1;
                        }
                    }
                }
                else if(inLine.substr(linePos, 14) == "@systemoutput(")
                {
                    linePos+=std::string("@systemoutput(").length();
                    std::string sys_call;
                    std::string output_filename = ".@systemoutput" + std::to_string(counter++);


                    if(read_sys_call(sys_call, linePos, inLine, readPath, lineNo, "@systemoutput()", os) > 0)
                        return 1;

                    //checks whether we're running from flatpak
                    if(std::ifstream("/.flatpak-info"))
                    {
                        //need sys_call quoted here for cURL to work
                        if(system(("flatpak-spawn --host bash -c " + quote(sys_call) + " > " + output_filename).c_str()))
                        {
                            os_mtx.lock();
                            os << "error: " << readPath << ": line " << lineNo << ": @systemoutput(" << quote(sys_call) << ") failed" << std::endl;
                            os_mtx.unlock();
                            Path("./", output_filename).removePath();
                            return 1;
                        }
                    }
                    else if(system((sys_call + " > " + output_filename).c_str())) //sys_call has to be unquoted for cURL to work
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @systemoutput(" << quote(sys_call) << ") failed" << std::endl;
                        os_mtx.unlock();
                        Path("./", output_filename).removePath();
                        return 1;
                    }

                    //indent amount updated inside read_and_process
                    if(read_and_process(Path("", output_filename), antiDepsOfReadPath, os) > 0)
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": failed to process output of system call '" << sys_call << "'" << std::endl;
                        os_mtx.unlock();
                        Path("./", output_filename).removePath();
                        return 1;
                    }

                    Path("./", output_filename).removePath();
                }
                else if(inLine.substr(linePos, 8) == "@pathto(")
                {
                    linePos+=std::string("@pathto(").length();
                    Name targetPageName;

                    if(read_path(targetPageName, linePos, inLine, readPath, lineNo, "@pathto()", os) > 0)
                        return 1;

                    //throws error if target targetPageName isn't being tracked by Nift
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetPageName << ") failed, Nift not tracking " << targetPageName << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }


                    Path targetPath = pages.find(targetPageInfo)->pagePath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    processedPage << pathToTarget.str();
                    indentAmount += pathToTarget.str().length();
                }
                else if(inLine.substr(linePos, 12) == "@pathtopage(")
                {
                    linePos+=std::string("@pathtopage(").length();
                    Name targetPageName;

                    if(read_path(targetPageName, linePos, inLine, readPath, lineNo, "@pathtopage()", os) > 0)
                        return 1;

                    //throws error if target targetPageName isn't being tracked by Nift
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetPageName << ") failed, Nift not tracking " << targetPageName << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }


                    Path targetPath = pages.find(targetPageInfo)->pagePath;
                    //targetPath.set_file_path_from(targetPathStr);

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    processedPage << pathToTarget.str();
                    indentAmount += pathToTarget.str().length();
                }
                else if(inLine.substr(linePos, 12) == "@pathtofile(")
                {
                    linePos+=std::string("@pathtofile(").length();
                    Name targetFilePath;

                    if(read_path(targetFilePath, linePos, inLine, readPath, lineNo, "@pathtofile()", os) > 0)
                        return 1;

                    //throws error if targetFilePath doesn't exist
                    if(!std::ifstream(targetFilePath.c_str()))
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }
                    Path targetPath;
                    targetPath.set_file_path_from(targetFilePath);

                    Path pathToTarget(pathBetween(pageToBuild.pagePath.dir, targetPath.dir), targetPath.file);

                    //adds path to target
                    processedPage << pathToTarget.str();
                    indentAmount += pathToTarget.str().length();
                }
                else if(inLine.substr(linePos, 10) == "@pagetitle")
                {
                    processedPage << pageToBuild.pageTitle.str;
                    indentAmount += pageToBuild.pageTitle.str.length();
                    linePos += std::string("@pagetitle").length();
                }
                else if(inLine.substr(linePos, 11) == "@page-title")
                {
                    processedPage << pageToBuild.pageTitle.str;
                    indentAmount += pageToBuild.pageTitle.str.length();
                    linePos += std::string("@page-title").length();
                }
                else if(inLine.substr(linePos, 14) == "@buildtimezone")
                {
                    processedPage << dateTimeInfo.cTimezone;
                    indentAmount += dateTimeInfo.cTimezone.length();
                    linePos += std::string("@buildtimezone").length();
                }
                else if(inLine.substr(linePos, 13) == "@loadtimezone")
                {
                    processedPage << "<script>document.write(new Date().toString().split(\"(\")[1].split(\")\")[0])</script>";
                    indentAmount += 82;
                    linePos += std::string("@loadtimezone").length();
                }
                else if(inLine.substr(linePos, 9) == "@timezone")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.cTimezone;
                    indentAmount += dateTimeInfo.cTimezone.length();
                    linePos += std::string("@timezone").length();
                }
                else if(inLine.substr(linePos, 10) == "@buildtime")
                {
                    processedPage << dateTimeInfo.cTime;
                    indentAmount += dateTimeInfo.cTime.length();
                    linePos += std::string("@buildtime").length();
                }
                else if(inLine.substr(linePos, 13) == "@buildUTCtime")
                {
                    processedPage << dateTimeInfo.currentUTCTime();
                    indentAmount += dateTimeInfo.currentUTCTime().length();
                    linePos += std::string("@buildUTCtime").length();
                }
                else if(inLine.substr(linePos, 10) == "@builddate")
                {
                    processedPage << dateTimeInfo.cDate;
                    indentAmount += dateTimeInfo.cDate.length();
                    linePos += std::string("@builddate").length();
                }
                else if(inLine.substr(linePos, 13) == "@buildUTCdate")
                {
                    processedPage << dateTimeInfo.currentUTCDate();
                    indentAmount += dateTimeInfo.currentUTCDate().length();
                    linePos += std::string("@buildUTCdate").length();
                }
                else if(inLine.substr(linePos, 12) == "@currenttime")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.cTime;
                    indentAmount += dateTimeInfo.cTime.length();
                    linePos += std::string("@currenttime").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCtime")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.currentUTCTime();
                    indentAmount += dateTimeInfo.currentUTCTime().length();
                    linePos += std::string("@currentUTCtime").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentdate")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.cDate;
                    indentAmount += dateTimeInfo.cDate.length();
                    linePos += std::string("@currentdate").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCdate")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.currentUTCDate();
                    indentAmount += dateTimeInfo.currentUTCDate().length();
                    linePos += std::string("@currentUTCdate").length();
                }
                else if(inLine.substr(linePos, 9) == "@loadtime")
                {
                    processedPage << "<script>document.write((new Date().toLocaleString()).split(\",\")[1])</script>";
                    indentAmount += 76;
                    linePos += std::string("@loadtime").length();
                }
                else if(inLine.substr(linePos, 12) == "@loadUTCtime")
                {
                    processedPage << "<script>document.write((new Date().toISOString()).split(\"T\")[1].split(\".\")[0])</script>";
                    indentAmount += 87;
                    linePos += std::string("@loadUTCtime").length();
                }
                else if(inLine.substr(linePos, 9) == "@loaddate")
                {
                    processedPage << "<script>document.write((new Date().toLocaleString()).split(\",\")[0])</script>";
                    indentAmount += 76;
                    linePos += std::string("@loaddate").length();
                }
                else if(inLine.substr(linePos, 12) == "@loadUTCdate")
                {
                    processedPage << "<script>document.write((new Date().toISOString()).split(\"T\")[0])</script>";
                    indentAmount += 73;
                    linePos += std::string("@loadUTCdate").length();
                }
                else if(inLine.substr(linePos, 10) == "@buildYYYY")
                {
                    processedPage << dateTimeInfo.currentYYYY();
                    indentAmount += dateTimeInfo.currentYYYY().length();
                    linePos += std::string("@buildYYYY").length();
                }
                else if(inLine.substr(linePos, 8) == "@buildYY")
                {
                    processedPage << dateTimeInfo.currentYY();
                    indentAmount += dateTimeInfo.currentYY().length();
                    linePos += std::string("@buildYY").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentYYYY")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.currentYYYY();
                    indentAmount += dateTimeInfo.currentYYYY().length();
                    linePos += std::string("@currentYYYY").length();
                }
                else if(inLine.substr(linePos, 10) == "@currentYY")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.currentYY();
                    indentAmount += dateTimeInfo.currentYY().length();
                    linePos += std::string("@currentYY").length();
                }
                else if(inLine.substr(linePos, 9) == "@loadYYYY")
                {
                    processedPage << "<script>document.write(new Date().getFullYear())</script>";
                    indentAmount += 57;
                    linePos += std::string("@loadYYYY").length();
                }
                else if(inLine.substr(linePos, 7) == "@loadYY")
                {
                    processedPage << "<script>document.write(new Date().getFullYear()%100)</script>";
                    indentAmount += 61;
                    linePos += std::string("@loadYY").length();
                }
                else if(inLine.substr(linePos, 8) == "@buildOS")
                {
                    processedPage << dateTimeInfo.currentOS();
                    indentAmount += dateTimeInfo.currentOS().length();
                    linePos += std::string("@buildOS").length();
                }
                else if(inLine.substr(linePos, 10) == "@currentOS")
                { //this is left for backwards compatibility
                    processedPage << dateTimeInfo.currentOS();
                    indentAmount += dateTimeInfo.currentOS().length();
                    linePos += std::string("@currentOS").length();
                } //I'm not sure how to do loadOS sorry!
                else if(inLine.substr(linePos, 16) == "@faviconinclude(") //checks for favicon include
                {
                    linePos+=std::string("@faviconinclude(").length();
                    std::string faviconPathStr;

                    if(read_path(faviconPathStr, linePos, inLine, readPath, lineNo, "@faviconinclude()", os) > 0)
                        return 1;

                    //warns user if favicon file doesn't exist
                    if(!std::ifstream(faviconPathStr.c_str()))
                    {
                        os_mtx.lock();
                        os << "warning: " << readPath << ": line " << lineNo << ": favicon file " << faviconPathStr << " does not exist" << std::endl;
                        os_mtx.unlock();
                    }

                    Path faviconPath;
                    faviconPath.set_file_path_from(faviconPathStr);

                    Path pathToFavicon(pathBetween(pageToBuild.pagePath.dir, faviconPath.dir), faviconPath.file);

                    std::string faviconInclude = "<link rel='icon' type='image/png' href='";
                    faviconInclude += pathToFavicon.str();
                    faviconInclude += "'>";

                    processedPage << faviconInclude;
                    indentAmount += faviconInclude.length();
                }
                else if(inLine.substr(linePos, 12) == "@cssinclude(") //checks for css includes
                {
                    linePos+=std::string("@cssinclude(").length();
                    std::string cssPathStr;

                    if(read_path(cssPathStr, linePos, inLine, readPath, lineNo, "@cssinclude()", os) > 0)
                        return 1;

                    //warns user if css file doesn't exist
                    if(!std::ifstream(cssPathStr.c_str()))
                    {
                        os_mtx.lock();
                        os << "warning: " << readPath << ": line " << lineNo << ": css file " << cssPathStr << " does not exist" << std::endl;
                        os_mtx.unlock();
                    }

                    Path cssPath;
                    cssPath.set_file_path_from(cssPathStr);

                    Path pathToCSSFile(pathBetween(pageToBuild.pagePath.dir, cssPath.dir), cssPath.file);

                    std::string cssInclude = "<link rel='stylesheet' type='text/css' href='";
                    cssInclude += pathToCSSFile.str();
                    cssInclude += "'>";

                    processedPage << cssInclude;
                    indentAmount += cssInclude.length();
                }
                else if(inLine.substr(linePos, 12) == "@imginclude(") //checks for img includes
                {
                    linePos+=std::string("@imginclude(").length();
                    std::string imgPathStr;

                    if(read_path(imgPathStr, linePos, inLine, readPath, lineNo, "@imginclude()", os) > 0)
                        return 1;

                    //warns user if img file doesn't exist
                    if(!std::ifstream(imgPathStr.c_str()))
                    {
                        os_mtx.lock();
                        os << "warning: " << readPath << ": line " << lineNo << ": img file " << imgPathStr << " does not exist" << std::endl;
                        os_mtx.unlock();
                    }

                    Path imgPath;
                    imgPath.set_file_path_from(imgPathStr);

                    Path pathToIMGFile(pathBetween(pageToBuild.pagePath.dir, imgPath.dir), imgPath.file);

                    std::string imgInclude = "<img src=\"" + pathToIMGFile.str() + "\">";

                    processedPage << imgInclude;
                    indentAmount += imgInclude.length();
                }
                else if(inLine.substr(linePos, 11) == "@jsinclude(") //checks for js includes
                {
                    linePos+=std::string("@jsinclude(").length();
                    std::string jsPathStr;

                    if(read_path(jsPathStr, linePos, inLine, readPath, lineNo, "@jsinclude()", os) > 0)
                        return 1;

                    //warns user if js file doesn't exist
                    if(!std::ifstream(jsPathStr.c_str()))
                    {
                        os_mtx.lock();
                        os << "warning: " << readPath << ": line " << lineNo << ": js file " << jsPathStr << " does not exist" << std::endl;
                        os_mtx.unlock();
                    }

                    Path jsPath;
                    jsPath.set_file_path_from(jsPathStr);

                    Path pathToJSFile(pathBetween(pageToBuild.pagePath.dir, jsPath.dir), jsPath.file);

                    //the type attribute is unnecessary for JavaScript resources.
                    //std::string jsInclude="<script type='text/javascript' src='";
                    std::string jsInclude="<script src='";
                    jsInclude += pathToJSFile.str();
                    jsInclude+="'></script>";

                    processedPage << jsInclude;
                    indentAmount += jsInclude.length();
                }
                else
                {
                    processedPage << '@';
                    indentAmount += " ";
                    linePos++;
                }
            }
            else //regular character
            {
                if(inLine[linePos] == '\t')
                    indentAmount += '\t';
                else
                    indentAmount += ' ';
                processedPage << inLine[linePos++];
            }
        }
    }

    if(codeBlockDepth > baseCodeBlockDepth)
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << openCodeLineNo << ": <pre*> open tag has no following </pre> close tag." << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    ifs.close();

    return 0;
}

int PageBuilder::read_path(std::string &pathRead, size_t &linePos, const std::string &inLine, const Path &readPath, const int &lineNo, const std::string &callType, std::ostream& os)
{
    pathRead = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos == inLine.size())
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //throws error if no path provided
    if(inLine[linePos] == ')')
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": no path provided inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
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
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == '\'')
                break;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else if(inLine[linePos] == '"')
    {
        ++linePos;
        for(;; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == '"')
                break;
            pathRead += inLine[linePos];
        }
        ++linePos;
    }
    else
    {
        for(; inLine[linePos] != ')'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
            {
                for(;linePos < inLine.size(); ++linePos)
                {
                    if(inLine[linePos] == ')')
                    {
                        ++linePos;
                        return 0;
                    }
                    else if(inLine[linePos] != ' ' && inLine[linePos] != '\t')
                    {
                        os_mtx.lock();
                        os << "error: " << readPath << ": line " << lineNo << ": unquoted path inside " << callType << " call contains whitespace" << std::endl << std::endl;
                        os_mtx.unlock();
                        return 1;
                    }
                }

                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
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
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //throws error if the path is invalid
    if(inLine[linePos] != ')')
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid path inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    ++linePos;

    return 0;
}

int PageBuilder::read_sys_call(std::string &sys_call, size_t &linePos, const std::string &inLine, const Path &readPath, const int &lineNo, const std::string &callType, std::ostream& os)
{
    sys_call = "";

    //skips over leading whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if either no closing bracket or a newline
    if(linePos == inLine.size())
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //throws error if no system call provided
    if(inLine[linePos] == ')')
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": no system call provided inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //reads the input system call
    if(inLine[linePos] == '\'')
    {
        ++linePos;
        for(;; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing single quote or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == '\'')
                break;
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
        for(;; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing double quote \" or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == '"')
                break;
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
        for(; inLine[linePos] != ')'; ++linePos)
        {
            if(linePos == inLine.size())
            {
                os_mtx.lock();
                os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
                os_mtx.unlock();
                return 1;
            }
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '\'')
                linePos++;
            else if(inLine[linePos] == '\\' && linePos+1 < inLine.size() && inLine[linePos+1] == '"')
                linePos++;
            sys_call += inLine[linePos];
        }

        //could strip trailing whitespace here
    }

    //skips over hopefully trailing whitespace
    while(linePos < inLine.size() && (inLine[linePos] == ' ' || inLine[linePos] == '\t'))
        ++linePos;

    //throws error if new line is between the system call and close bracket
    if(linePos == inLine.size())
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": system call has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    //throws error if the system call is invalid (eg. has text after quoted system call)
    if(inLine[linePos] != ')')
    {
        os_mtx.lock();
        os << "error: " << readPath << ": line " << lineNo << ": invalid system call inside " << callType << " call" << std::endl << std::endl;
        os_mtx.unlock();
        return 1;
    }

    ++linePos;

    return 0;
}
