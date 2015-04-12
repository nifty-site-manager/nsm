#ifndef READ_AND_PROCESS_H_
#define READ_AND_PROCESS_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "DateTimeInfo.h"

//reads file whilst writing processed version to ofs
int read_and_process(const Location &readLocation,
    std::set<Location> antiDepsOfReadLocation, //these are the files that have readLoc as a dep
    const Location &contentLocation,
    const Title &pageTitle,
    const DateTimeInfo &dateTimeInfo,
    std::string &indentAmount,
    int &codeBlockDepth,
    int &htmlCommentDepth,
    bool &contentInserted,
    std::stringstream &processedPage,
    std::set<Location> &pageDeps)
{
    //adds read location to anti dependencies of read location
    antiDepsOfReadLocation.insert(readLocation);

    std::ifstream ifs(STR(readLocation));
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
                        std::cout << "error: " << readLocation << ": line " << lineNo << ": </pre> close tag has no preceding <pre*> open tag." << std::endl << std::endl;
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
                else if(inLine.substr(linePos, 14) == "@insertcontent")
                {
                    contentInserted = 1;
                    std::string replaceText = "@insertfromfile(" + STR(contentLocation) + ")";
                    inLine.replace(linePos, 12, replaceText);
                }
                else if(inLine.substr(linePos, 16) == "@insertfromfile(")
                {
                    int posOffset=std::string("@insertfromfile(").length();
                    std::string insertLocation="";

                    for(; inLine[linePos+posOffset] != ')'; posOffset++)
                        if(inLine[linePos+posOffset] != '"' && inLine[linePos+posOffset] != '\'')
                            insertLocation += inLine[linePos+posOffset];
                    pageDeps.insert(insertLocation);
                    posOffset++;

                    //ensures insert file exists
                    if(!std::ifstream(insertLocation))
                    {
                        std::cout << "error: " << readLocation << ": line " << lineNo << ": file insertion failed as " << insertLocation << " does not exist" << std::endl;
                        return 1;
                    }
                    //ensures insert file isn't an anti dep of read location
                    if(antiDepsOfReadLocation.count(insertLocation))
                    {
                        std::cout << "error: " << readLocation << ": line " << lineNo << ": insertion of " << insertLocation << " would result in an insertion loop" << std::endl;
                        return 1;
                    }

                    //adds insert file
                    if(read_and_process(insertLocation, antiDepsOfReadLocation, contentLocation, pageTitle, dateTimeInfo, indentAmount, codeBlockDepth, htmlCommentDepth, contentInserted, processedPage, pageDeps) > 0)
                        return 1;
                    //indent amount updated inside read_and_process
                    linePos += posOffset;
                }
                else if(inLine.substr(linePos, 10) == "@pagetitle")
                {
                    processedPage << pageTitle;
                    indentAmount+=STR(pageTitle).length();
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
        std::cout << "error: " << readLocation << ": line " << openCodeLineNo << ": <pre*> open tag has no following </pre> close tag." << std::endl << std::endl;
        return 1;
    }

    ifs.close();

    return 0;
}


#endif //READ_AND_PROCESS_H_
