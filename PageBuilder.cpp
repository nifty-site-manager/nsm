#include "PageBuilder.h"

PageBuilder::PageBuilder(const std::set<PageInfo> &Pages)
{
    pages = Pages;
}

int PageBuilder::build(const PageInfo &PageToBuild)
{
    pageToBuild = PageToBuild;

    std::cout << std::endl;

    //ensures content and template files exist
    if(!std::ifstream(pageToBuild.contentPath.str()))
    {
        std::cout << "error: cannot build " << pageToBuild.pagePath << " as content file " << pageToBuild.contentPath << " does not exist" << std::endl;
        return 1;
    }
    if(!std::ifstream(pageToBuild.templatePath.str()))
    {
        std::cout << "error: cannot build " << pageToBuild.pagePath << " as template file " << pageToBuild.templatePath << " does not exist." << std::endl;
        return 1;
    }

    std::cout << "building page " << pageToBuild.pagePath << std::endl;

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
    if(read_and_process(pageToBuild.templatePath, antiDepsOfReadPath) > 0)
        return 1;

    //ensures @inputcontent was found inside template dag
    if(!contentAdded)
    {
        std::cout << "error: @inputcontent not found within template file " << pageToBuild.templatePath << " or any of its dependencies, content from " << pageToBuild.contentPath << " has not been inserted" << std::endl;
        return 1;
    }

    //makes sure page file exists
    pageToBuild.pagePath.ensurePathExists();

    //makes sure we can write to page file
    chmod(pageToBuild.pagePath.str().c_str(), 0644);

    //writes processed page to page file
    std::ofstream pageStream(pageToBuild.pagePath.str());
    pageStream << processedPage.str();
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

    std::cout << "page build successful" << std::endl;

    return 0;
}

//reads file whilst writing processed version to ofs
int PageBuilder::read_and_process(const Path &readPath, std::set<Path> antiDepsOfReadPath)
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
                    /*case '&':
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
                        std::cout << "error: " << readPath << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl << std::endl;
                        return 1;
                    }
                }

                //checks whether to escape <
                if(codeBlockDepth > 0)
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
                    std::string replaceText = "@input(" + pageToBuild.contentPath.str() + ")";
                    inLine.replace(linePos, 13, replaceText);
                }
                else if(inLine.substr(linePos, 7) == "@input(")
                {
                    linePos+=std::string("@input(").length();
                    std::string inputPathStr="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @input() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @input() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @input() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @input() call" << std::endl << std::endl;
                                return 1;
                            }
                            inputPathStr += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @input() call" << std::endl << std::endl;
                                return 1;
                            }
                            inputPathStr += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @input() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @input() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            inputPathStr += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @input() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @input() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @input() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    Path inputPath;
                    inputPath.set_file_path_from(inputPathStr);
                    pageDeps.insert(inputPath);

                    //ensures insert file exists
                    if(!std::ifstream(inputPathStr))
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                        return 1;
                    }
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " would result in an input loop" << std::endl;
                        return 1;
                    }

                    //adds insert file
                    if(read_and_process(inputPath, antiDepsOfReadPath) > 0)
                        return 1;
                    //indent amount updated inside read_and_process
                }
                else if(inLine.substr(linePos, 8) == "@pathto(")
                {
                    linePos+=std::string("@pathto(").length();
                    Name targetPageName="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathto() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @pathto() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @pathto() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @pathto() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @pathto() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathto() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @pathto() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @pathto() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @pathto() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @pathto() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //throws error if target targetPageName isn't being tracked by nsm
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetPageName << ") failed, nsm not tracking " << targetPageName << std::endl;
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
                    Name targetPageName="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathtopage() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @pathtopage() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @pathtopage() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @pathtopage() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @pathtopage() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathtopage() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @pathtopage() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            targetPageName += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @pathtopage() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @pathtopage() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @pathtopage() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //throws error if target targetPageName isn't being tracked by nsm
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetPageName << ") failed, nsm not tracking " << targetPageName << std::endl;
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
                    Name targetFilePath="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathtofile() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @pathtofile() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @pathtofile() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @pathtofile() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetFilePath += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @pathtofile() call" << std::endl << std::endl;
                                return 1;
                            }
                            targetFilePath += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @pathtofile() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @pathtofile() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            targetFilePath += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @pathtofile() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @pathtofile() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @pathtofile() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //throws error if targetFilePath doesn't exist
                    if(!std::ifstream(targetFilePath.c_str()))
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
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
                else if(inLine.substr(linePos, 12) == "@currenttime")
                {
                    processedPage << dateTimeInfo.cTime;
                    indentAmount += dateTimeInfo.cTime.length();
                    linePos += std::string("@currenttime").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCtime")
                {
                    processedPage << dateTimeInfo.currentUTCTime();
                    indentAmount += dateTimeInfo.currentUTCTime().length();
                    linePos += std::string("@currentUTCtime").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentdate")
                {
                    processedPage << dateTimeInfo.cDate;
                    indentAmount += dateTimeInfo.cDate.length();
                    linePos += std::string("@currentdate").length();
                }
                else if(inLine.substr(linePos, 15) == "@currentUTCdate")
                {
                    processedPage << dateTimeInfo.currentUTCDate();
                    indentAmount += dateTimeInfo.currentUTCDate().length();
                    linePos += std::string("@currentUTCdate").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentYYYY")
                {
                    processedPage << dateTimeInfo.currentYY();
                    indentAmount += dateTimeInfo.cDate.length();
                    linePos += std::string("@currentYYYY").length();
                }
                else if(inLine.substr(linePos, 10) == "@currentYY")
                {
                    processedPage << dateTimeInfo.currentYY();
                    indentAmount += dateTimeInfo.cDate.length();
                    linePos += std::string("@currentYY").length();
                }
                else if(inLine.substr(linePos, 9) == "@timezone")
                {
                    processedPage << dateTimeInfo.cTimezone;
                    indentAmount += dateTimeInfo.cTimezone.length();
                    linePos += std::string("@timezone").length();
                }
                else if(inLine.substr(linePos, 16) == "@faviconinclude(") //checks for favicon include
                {
                    linePos+=std::string("@faviconinclude(").length();
                    std::string faviconPathStr="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @faviconinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @faviconinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @faviconinclude() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @faviconinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            faviconPathStr += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @faviconinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            faviconPathStr += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @faviconinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @faviconinclude() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            faviconPathStr += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @faviconinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @faviconinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @faviconinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //warns user if favicon file doesn't exist
                    if(!std::ifstream(faviconPathStr.c_str()))
                    {
                        std::cout << "warning: " << readPath << ": line " << lineNo << ": favicon file " << faviconPathStr << " does not exist" << std::endl;
                    }

                    Path faviconPath;
                    faviconPath.set_file_path_from(faviconPathStr);

                    Path pathToFavicon(pathBetween(pageToBuild.pagePath.dir, faviconPath.dir), faviconPath.file);

                    std::string faviconInclude="<link rel='icon' type='image/png' href='";
                    faviconInclude += pathToFavicon.str();
                    faviconInclude += "'>";

                    processedPage << faviconInclude;
                    indentAmount += faviconInclude.length();
                }
                else if(inLine.substr(linePos, 12) == "@cssinclude(") //checks for css includes
                {
                    linePos+=std::string("@cssinclude(").length();
                    std::string cssPathStr="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @cssinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @cssinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @cssinclude() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @cssinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            cssPathStr += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @cssinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            cssPathStr += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @cssinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @cssinclude() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            cssPathStr += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @cssinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @cssinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @cssinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //warns user if css file doesn't exist
                    if(!std::ifstream(cssPathStr.c_str()))
                    {
                        std::cout << "warning: " << readPath << ": line " << lineNo << ": css file " << cssPathStr << " does not exist" << std::endl;
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
                    std::string imgPathStr="";

                     //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @imginclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @imginclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @imginclude() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @imginclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            imgPathStr += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @imginclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            imgPathStr += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @imginclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @imginclude() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            imgPathStr += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @imginclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @imginclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @imginclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //warns user if img file doesn't exist
                    if(!std::ifstream(imgPathStr.c_str()))
                    {
                        std::cout << "warning: " << readPath << ": line " << lineNo << ": img file " << imgPathStr << " does not exist" << std::endl;
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
                    std::string jsPathStr="";

                    //throws error if either no closing bracket or a newline
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @jsinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if no path provided
                    if(inLine[linePos] == ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": no path provided inside @jsinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if leading whitespace before path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": leading whitespace before path inside @jsinclude() call" << std::endl << std::endl;
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside @jsinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            jsPathStr += inLine[linePos];
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
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside @jsinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            jsPathStr += inLine[linePos];
                        }
                        ++linePos;
                    }
                    else
                    {
                        for(; inLine[linePos] != ')'; ++linePos)
                        {
                            if(linePos == inLine.size())
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside @jsinclude() call" << std::endl << std::endl;
                                return 1;
                            }
                            else if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                            {
                                std::cout << "error: " << readPath << ": line " << lineNo << ": unquoted path inside @jsinclude() call contains whitespace" << std::endl << std::endl;
                                return 1;
                            }
                            jsPathStr += inLine[linePos];
                        }

                    }

                    //throws error if new line is between the path and close bracket
                    if(linePos == inLine.size())
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": newline character inside @jsinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if trailing whitespace after path
                    if(inLine[linePos] == ' ' || inLine[linePos] == '\t')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": trailing whitespace after path inside @jsinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    //throws error if the path is invalid
                    if(inLine[linePos] != ')')
                    {
                        std::cout << "error: " << readPath << ": line " << lineNo << ": invalid path inside @jsinclude() call" << std::endl << std::endl;
                        return 1;
                    }

                    ++linePos;

                    //warns user if js file doesn't exist
                    if(!std::ifstream(jsPathStr.c_str()))
                        std::cout << "warning: " << readPath << ": line " << lineNo << ": js file " << jsPathStr << " does not exist" << std::endl;

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
        std::cout << "error: " << readPath << ": line " << openCodeLineNo << ": <pre*> open tag has no following </pre> close tag." << std::endl << std::endl;
        return 1;
    }

    ifs.close();

    return 0;
}
