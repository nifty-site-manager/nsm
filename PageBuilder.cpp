#include "PageBuilder.h"

int PageBuilder::build(const PageInfo &PageToBuild)
{
    pageToBuild = PageToBuild;

    std::cout << std::endl;

    //ensures content and template files exist
    if(!std::ifstream(pageToBuild.contentPath.str))
    {
        std::cout << "error: cannot build " << pageToBuild.pagePath << " as content file " << pageToBuild.contentPath << " does not exist" << std::endl;
        return 1;
    }
    if(!std::ifstream(pageToBuild.templatePath.str))
    {
        std::cout << "error: cannot build " << pageToBuild.pagePath << " as template file " << pageToBuild.templatePath << " does not exist." << std::endl;
        return 1;
    }

    std::cout << "building page " << pageToBuild.pagePath << std::endl;

    //makes sure variables are at default values
    //DateTimeInfo dateTimeInfo;
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

    //ensures @inputcontent was found inside template tree
    if(!contentAdded)
    {
        std::cout << "error: @inputcontent not found within template file " << pageToBuild.templatePath << " or any of its dependencies, content from " << pageToBuild.contentPath << " has not been inserted" << std::endl;
        return 1;
    }

    std::string systemCall;

    //makes sure we can write to page file
    systemCall = "if [ -f " + pageToBuild.pagePath.str + " ]; then chmod +w " + pageToBuild.pagePath.str + "; fi";
    system(systemCall.c_str());

    std::ofstream pageStream(pageToBuild.pagePath.str);
    pageStream << processedPage.str();
    pageStream.close();

    //makes sure user can't edit page file
    systemCall = "if [ -f " + pageToBuild.pagePath.str + " ]; then chmod -w " + pageToBuild.pagePath.str + "; fi";
    system(systemCall.c_str());

    //gets path for storing page information
    Path pageInfoPath = pageToBuild.pagePath.getInfoPath();

    //makes sure we can write to info file_
    systemCall = "if [ -f " + pageInfoPath.str + " ]; then chmod +w " + pageInfoPath.str + "; fi";
    system(systemCall.c_str());

    //writes dependencies to page info file
    std::ofstream infoStream(pageInfoPath.str);
    infoStream << this->pageToBuild << std::endl;
    for(auto pageDep=pageDeps.begin(); pageDep != pageDeps.end(); pageDep++)
        infoStream << *pageDep << std::endl;
    infoStream.close();

    //makes sure user can't edit info file
    systemCall = "if [ -f " + pageInfoPath.str + " ]; then chmod -w " + pageInfoPath.str + "; fi";
    system(systemCall.c_str());

    std::cout << "page build successful" << std::endl;

    return 0;
};

//reads file whilst writing processed version to ofs
int PageBuilder::read_and_process(const Path &readPath, std::set<Path> antiDepsOfReadPath)
{
    //adds read path to anti dependencies of read path
    antiDepsOfReadPath.insert(readPath);

    std::ifstream ifs(readPath.str);
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
                    case '&':
                        processedPage << "&amp;";
                        linePos++;
                        indentAmount += std::string(5, ' ');
                        break;
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
                    std::string replaceText = "@input(" + pageToBuild.contentPath.str + ")";
                    inLine.replace(linePos, 13, replaceText);
                }
                else if(inLine.substr(linePos, 7) == "@input(")
                {
                    int posOffset=std::string("@input(").length();
                    std::string inputPath="";

                    for(; inLine[linePos+posOffset] != ')'; posOffset++)
                        if(inLine[linePos+posOffset] != '"' && inLine[linePos+posOffset] != '\'')
                            inputPath += inLine[linePos+posOffset];
                    pageDeps.insert(inputPath);
                    posOffset++;

                    //ensures insert file exists
                    if(!std::ifstream(inputPath))
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
                    linePos += posOffset;
                }
                else if(inLine.substr(linePos, 10) == "@pagetitle")
                {
                    processedPage << pageToBuild.pageTitle;
                    indentAmount+=pageToBuild.pageTitle.str.length();
                    linePos+=std::string("@pagetitle").length();
                }
                else if(inLine.substr(linePos, 12) == "@currenttime")
                {
                    processedPage << dateTimeInfo.cTime;
                    indentAmount+=dateTimeInfo.cTime.length();
                    linePos+=std::string("@currenttime").length();
                }
                else if(inLine.substr(linePos, 12) == "@currentdate")
                {
                    processedPage << dateTimeInfo.cDate;
                    indentAmount+=dateTimeInfo.cDate.length();
                    linePos+=std::string("@currentdate").length();
                }
                else if(inLine.substr(linePos, 9) == "@timezone")
                {
                    processedPage << dateTimeInfo.cTimezone;
                    indentAmount+=dateTimeInfo.cTimezone.length();
                    linePos+=std::string("@timezone").length();
                }
                else if(inLine.substr(linePos, 15) == "@includefavicon") //checks for favicon include
                {
                    int posOffset=std::string("@includefavicon(").length();
                    std::string faviconInclude="<link rel='icon' type='image/png' href='";

                    for(; inLine[linePos+posOffset] != ')'; posOffset++)
                        if(inLine[linePos+posOffset] != '"' && inLine[linePos+posOffset] != '\'')
                            faviconInclude += inLine[linePos+posOffset];
                    posOffset++;

                    faviconInclude+="'>";

                    processedPage << faviconInclude;
                    indentAmount += faviconInclude.length();
                    linePos += posOffset;
                }
                else if(inLine.substr(linePos, 16) == "@includecssfile(") //checks for css includes
                {
                    int posOffset=std::string("@includecssfile(").length();
                    std::string cssInclude="<link rel='stylesheet' type='text/css' href='";

                    for(; inLine[linePos+posOffset] != ')'; posOffset++)
                        if(inLine[linePos+posOffset] != '"' && inLine[linePos+posOffset] != '\'')
                            cssInclude += inLine[linePos+posOffset];
                    posOffset++;

                    cssInclude+="'>";

                    processedPage << cssInclude;
                    indentAmount += cssInclude.length();
                    linePos += posOffset;
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
