#include "PageBuilder.h"

PageBuilder::PageBuilder(const std::set<PageInfo> &Pages)
{
    pages = Pages;
}

int PageBuilder::build(const PageInfo &PageToBuild, std::ostream& os)
{
    pageToBuild = PageToBuild;

    //os << std::endl;

    //ensures content and template files exist
    if(!std::ifstream(pageToBuild.contentPath.str()))
    {
        os << "error: cannot build " << pageToBuild.pagePath << " as content file " << pageToBuild.contentPath << " does not exist" << std::endl;
        return 1;
    }
    if(!std::ifstream(pageToBuild.templatePath.str()))
    {
        os << "error: cannot build " << pageToBuild.pagePath << " as template file " << pageToBuild.templatePath << " does not exist." << std::endl;
        return 1;
    }

    //os << "building page " << pageToBuild.pagePath << std::endl;

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
    if(read_and_process(pageToBuild.templatePath, antiDepsOfReadPath, os) > 0)
        return 1;

    //ensures @inputcontent was found inside template dag
    if(!contentAdded)
    {
        os << "error: @inputcontent not found within template file " << pageToBuild.templatePath << " or any of its dependencies, content from " << pageToBuild.contentPath << " has not been inserted" << std::endl;
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

    //os << "page build successful" << std::endl;

    return 0;
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
                        os << "error: " << readPath << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl << std::endl;
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
                    std::string replaceText = "@input(" + quote(pageToBuild.contentPath.str()) + ")";
                    inLine.replace(linePos, 13, replaceText);
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
                        os << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " failed as path does not exist" << std::endl;
                        return 1;
                    }
                    //ensures insert file isn't an anti dep of read path
                    if(antiDepsOfReadPath.count(inputPath))
                    {
                        os << "error: " << readPath << ": line " << lineNo << ": inputting file " << inputPath << " would result in an input loop" << std::endl;
                        return 1;
                    }

                    //adds insert file
                    if(read_and_process(inputPath, antiDepsOfReadPath, os) > 0)
                        return 1;
                    //indent amount updated inside read_and_process
                }
                else if(inLine.substr(linePos, 8) == "@pathto(")
                {
                    linePos+=std::string("@pathto(").length();
                    Name targetPageName;

                    if(read_path(targetPageName, linePos, inLine, readPath, lineNo, "@pathto()", os) > 0)
                        return 1;

                    //throws error if target targetPageName isn't being tracked by nsm
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        os << "error: " << readPath << ": line " << lineNo << ": @pathto(" << targetPageName << ") failed, nsm not tracking " << targetPageName << std::endl;
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

                    //throws error if target targetPageName isn't being tracked by nsm
                    PageInfo targetPageInfo;
                    targetPageInfo.pageName = targetPageName;
                    if(!pages.count(targetPageInfo))
                    {
                        os << "error: " << readPath << ": line " << lineNo << ": @pathtopage(" << targetPageName << ") failed, nsm not tracking " << targetPageName << std::endl;
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
                        os << "error: " << readPath << ": line " << lineNo << ": file " << targetFilePath << " does not exist" << std::endl;
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
                        os << "warning: " << readPath << ": line " << lineNo << ": favicon file " << faviconPathStr << " does not exist" << std::endl;
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
                        os << "warning: " << readPath << ": line " << lineNo << ": css file " << cssPathStr << " does not exist" << std::endl;
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
                        os << "warning: " << readPath << ": line " << lineNo << ": img file " << imgPathStr << " does not exist" << std::endl;
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
                        os << "warning: " << readPath << ": line " << lineNo << ": js file " << jsPathStr << " does not exist" << std::endl;

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
        os << "error: " << readPath << ": line " << openCodeLineNo << ": <pre*> open tag has no following </pre> close tag." << std::endl << std::endl;
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
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
        return 1;
    }

    //throws error if no path provided
    if(inLine[linePos] == ')')
    {
        os << "error: " << readPath << ": line " << lineNo << ": no path provided inside " << callType << " call" << std::endl << std::endl;
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
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing single quote or newline inside " << callType << " call" << std::endl << std::endl;
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
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing double quote \" or newline inside " << callType << " call" << std::endl << std::endl;
                return 1;
            }
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
                os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
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
                        os << "error: " << readPath << ": line " << lineNo << ": unquoted path inside " << callType << " call contains whitespace" << std::endl << std::endl;
                        return 1;
                    }
                }

                os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside " << callType << " call" << std::endl << std::endl;
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
        os << "error: " << readPath << ": line " << lineNo << ": path has no closing bracket ) or newline inside  " << callType << " call" << std::endl << std::endl;
        return 1;
    }

    //throws error if the path is invalid
    if(inLine[linePos] != ')')
    {
        os << "error: " << readPath << ": line " << lineNo << ": invalid path inside " << callType << " call" << std::endl << std::endl;
        return 1;
    }

    ++linePos;

    return 0;
}
