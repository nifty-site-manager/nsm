#include "SiteInfo.h"
#include <unistd.h>

std::string get_pwd()
{
    char * pwd_char = getcwd(NULL, 0);
    std::string pwd = pwd_char;
    free(pwd_char);
    return pwd;
}

void unrecognisedCommand(const std::string from, const std::string cmd)
{
    std::cout << "error: " << from << " does not recognise the command '" << cmd << "'" << std::endl;
}

bool parError(int noParams, char* argv[], const std::string &expectedNo)
{
        std::cout << "error: " << noParams << " is more than the " << expectedNo << " parameters expected" << std::endl;
        std::cout << "parameters given:";
        for(int p=1; p<=noParams; p++)
            std::cout << " " << argv[p];
        std::cout << std::endl;
        return 1;
}

int main(int argc, char* argv[])
{
    int noParams = argc-1;

    if(noParams == 0)
    {
        std::cout << "no commands given, nothing to do" << std::endl;
        return 0;
    }

    std::string cmd = argv[1];

    if(cmd == "commands")
    {
        std::cout << "------------------- available commands -------------------" << std::endl;
        std::cout << "nsm commands      | lists all nsm commands" << std::endl;
        std::cout << "nsm config        | lists config settings" << std::endl;
        std::cout << "nsm init          | input: (site-name)" << std::endl;
        std::cout << "nsm init-cb       | input: (site-name)" << std::endl;
        std::cout << "nsm status        | lists updated and problem pages" << std::endl;
        std::cout << "nsm info          | input: page-name-1 .. page-name-k" << std::endl;
        std::cout << "nsm info-all      | lists tracked pages" << std::endl;
        std::cout << "nsm info-names    | lists tracked page names" << std::endl;
        std::cout << "nsm track         | input: page-name (page-title) (template-path)" << std::endl;
        std::cout << "nsm untrack       | input: page-name" << std::endl;
        std::cout << "nsm rm            | input: page-name" << std::endl;
        std::cout << "nsm mv            | input: old-name new-name" << std::endl;
        std::cout << "nsm cp            | input: tracked-name new-name" << std::endl;
        std::cout << "nsm build         | input: page-name-1 .. page-name-k" << std::endl;
        std::cout << "nsm build-updated | builds updated pages" << std::endl;
        std::cout << "nsm build-all     | builds all tracked pages " << std::endl;
        std::cout << "nsm new-title     | input: page-name new-title" << std::endl;
        std::cout << "nsm new-template  | input: page-name template-path" << std::endl;
        std::cout << "----------------------------------------------------------" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site in " << get_pwd() << "/" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }

    if(cmd == "init-null")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site in " << get_pwd() << "/" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        pagesListPath = Path("template/", "page.template");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("template/page.template");
        ofs << "<html>" << std::endl;
        ofs << "    <head>" << std::endl;
        ofs << "        @input(template/head.content)" << std::endl;
        ofs << "    </head>" << std::endl;
        ofs << std::endl;
        ofs << "    <body>" << std::endl;
        ofs << "        @input(template/menu.content)" << std::endl;
        ofs << std::endl;
        ofs << "        @inputcontent" << std::endl;
        ofs << "    </body>" << std::endl;
        ofs << "</html>" << std::endl;
        ofs << std::endl;
        ofs.close();

        ofs.open("template/menu.content");
        ofs << "<nav>" << std::endl;
        ofs << "    <ul class=\"drop_menu\">" << std::endl;
        ofs << "        <li class=\"dm_li\"><a href=\"@pathto(index)\">index</a></li>" << std::endl;
        ofs << "    </ul>" << std::endl;
        ofs << "</nav>" << std::endl;
        ofs.close();

        ofs.open("template/head.content");
        if(noParams == 1)
            ofs << "<title>basic site - @pagetitle</title>" << std::endl;
        else if(noParams > 1)
            ofs << "<title>" << argv[2] << " - @pagetitle</title>" << std::endl;
        ofs << std::endl;
        ofs << "@cssinclude(site/css/pagestyle.css)" << std::endl;
        ofs << "@cssinclude(site/css/dropmenu.css)" << std::endl;
        ofs << std::endl;
        ofs.close();

        pagesListPath = Path("site/css/", "dropmenu.css");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("site/css/dropmenu.css");
        ofs << "/*" << std::endl;
        ofs << "    Style for drop menus" << std::endl;
        ofs << "*/" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    unordered menu list" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "nav" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    max-width: 80% !important;" << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    background:#C8C8C8;" << std::endl;
        ofs << std::endl;
        ofs << "    border-style: solid;" << std::endl;
        ofs << "    border-width: 1px;" << std::endl;
        ofs << "    border-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 15px;" << std::endl;
        ofs << "    margin-top: 15px;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left:15px !important;" << std::endl;
        ofs << "    padding-right:15px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    max-width: 700px;" << std::endl;
        ofs << std::endl;
        ofs << "    /*-moz-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    -webkit-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);*/" << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-radius: 5px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    submenu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*hides and styles submenus*/" << std::endl;
        ofs << "ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: block;" << std::endl;
        ofs << "    visibility: hidden;" << std::endl;
        ofs << "    position: absolute;" << std::endl;
        ofs << "    opacity: 0;" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << "    padding-left:0px;" << std::endl;
        ofs << "    background:#888888;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*positions and reveals submenu when hovering drop menu list item*/" << std::endl;
        ofs << "li.dm_li:hover ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:absolute;" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    visibility: visible;" << std::endl;
        ofs << "    opacity: 1.0;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    links in the menu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles links for menu list items*/" << std::endl;
        ofs << "ul.drop_menu a" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    cursor: pointer;" << std::endl;
        ofs << "    text-decoration:none;" << std::endl;
        ofs << std::endl;
        ofs << "    outline: 0;" << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    transition-property: color;" << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*changes link text colour when hovering*/" << std::endl;
        ofs << "ul.drop_menu a:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    color:#D8D8D8;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#C8C8C8 !important;" << std::endl;
        ofs << "    float: left;" << std::endl;
        ofs << "    white-space:nowrap;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#888888 !important;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles text*/" << std::endl;
        ofs << "ul.drop_menu .dm_text" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    -webkit-user-select: none; /* Chrome/Safari */" << std::endl;
        ofs << "    -moz-user-select: none; /* Firefox */" << std::endl;
        ofs << "    -ms-user-select: none; /* IE10+ */" << std::endl;
        ofs << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu .dm_text:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*color:#D8D8D8;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    just menu list items" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles menu list items*/" << std::endl;
        ofs << "li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*padding-top: 4px;" << std::endl;
        ofs << "    padding-bottom: 4px;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles menu list items when hovering*/" << std::endl;
        ofs << "li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:relative;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        ofs.open("site/css/pagestyle.css");
        ofs << "html" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background: #484848;" << std::endl;
        ofs << "    overflow-y: scroll;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "body" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#E8E8E8;" << std::endl;
        ofs << std::endl;
        ofs << "    display: table;" << std::endl;
        ofs << std::endl;
        ofs << "    width: 80%;" << std::endl;
        ofs << "    max-width: 800px !important;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 40px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 15px;" << std::endl;
        ofs << "    padding-right: 15px;" << std::endl;
        ofs << "    padding-top: 1px;" << std::endl;
        ofs << "    padding-bottom: 10px;" << std::endl;
        ofs << std::endl;
        ofs << "    word-wrap:break-word;" << std::endl;
        ofs << std::endl;
        ofs << "    font-family:'Helvetica Neue',Helvetica,Arial,sans-serif;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "footer" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    text-align: center;" << std::endl;
        ofs << "    margin-top:20px;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        system("nsm track index");
        system("nsm build-updated");

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }

    if(cmd == "init-dev")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site in " << get_pwd() << "/" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        pagesListPath = Path("template/", "page.template");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("template/page.template");
        ofs << "<html>" << std::endl;
        ofs << "    <head>" << std::endl;
        ofs << "        @input(template/head.content)" << std::endl;
        ofs << "    </head>" << std::endl;
        ofs << std::endl;
        ofs << "    <body>" << std::endl;
        ofs << "        @input(template/menu.content)" << std::endl;
        ofs << std::endl;
        ofs << "        @inputcontent" << std::endl;
        ofs << std::endl;
        ofs << "        @input(template/footer.content)" << std::endl;
        ofs << std::endl;
        ofs << "        @jsinclude(site/js/process_pre_tags.js)" << std::endl;
        ofs << "    </body>" << std::endl;
        ofs << "</html>" << std::endl;
        ofs << std::endl;
        ofs.close();

        ofs.open("template/menu.content");
        ofs << "<nav>" << std::endl;
        ofs << "    <ul class=\"drop_menu\">" << std::endl;
        ofs << "        <li class=\"dm_li\"><a href=\"@pathto(index)\">index</a></li>" << std::endl;
        ofs << "    </ul>" << std::endl;
        ofs << "</nav>" << std::endl;
        ofs.close();

        ofs.open("template/head.content");
        if(noParams == 1)
            ofs << "<title>basic site - @pagetitle</title>" << std::endl;
        else if(noParams > 1)
            ofs << "<title>" << argv[2] << " - @pagetitle</title>" << std::endl;
        ofs << std::endl;
        ofs << "@cssinclude(site/css/pagestyle.css)" << std::endl;
        ofs << "@cssinclude(site/css/dropmenu.css)" << std::endl;
        ofs << std::endl;
        ofs << "<!-- uses MathJax CDC for LaTeX support" << std::endl;
        ofs << "     see http://docs.mathjax.org/en/latest/start.html for more info -->" << std::endl;
        ofs << "<script type=\"text/x-mathjax-config\">" << std::endl;
        ofs << "    MathJax.Hub.Config" << std::endl;
        ofs << "    ({" << std::endl;
        ofs << "        extensions: [\"tex2jax.js\"]," << std::endl;
        ofs << "        jax: [\"input/TeX\", \"output/HTML-CSS\"]," << std::endl;
        ofs << "        tex2jax:" << std::endl;
        ofs << "        {" << std::endl;
        ofs << "            inlineMath: [['$','$'], ['\\\\(','\\\\)']]," << std::endl;
        ofs << "            displayMath: [ ['$$','$$'], [\"\\\\[\",\"\\\\]\"] ]," << std::endl;
        ofs << "            processEscapes: true" << std::endl;
        ofs << "        }," << std::endl;
        ofs << "        \"HTML-CSS\": { availableFonts: [\"MathJax TeX\"] }" << std::endl;
        ofs << "    });" << std::endl;
        ofs << "</script>" << std::endl;
        ofs << "<script type=\"text/javascript\"" << std::endl;
        ofs << "        src=\"https://cdn.mathjax.org/mathjax/latest/MathJax.js\">" << std::endl;
        ofs << "</script>" << std::endl;
        ofs.close();

        ofs.open("template/footer.content");
        ofs << "<footer>" << std::endl;
        ofs << "    <hr>" << std::endl;
        ofs << "    <p>" << std::endl;
        ofs << "        Page last built on @currentdate at @currenttime (@timezone)." << std::endl;
        ofs << "    </p>" << std::endl;
        ofs << "</footer>" << std::endl;
        ofs.close();

        pagesListPath = Path("site/js/", "process_pre_tags.js");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("site/js/process_pre_tags.js");
        ofs << "/*" << std::endl;
        ofs << "    Has browser ignore leading tabs for <pre> code blocks." << std::endl;
        ofs << "    modified from: http://stackoverflow.com/a/8505182/4525897" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "var pre_elements = document.getElementsByTagName('pre');" << std::endl;
        ofs << "for (var i = 0; i < pre_elements.length; i++)" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    pre_elements = document.getElementsByTagName('pre');" << std::endl;
        ofs << "    var content = pre_elements[i].innerHTML;" << std::endl;
        ofs << "    var tabs_to_remove = '';" << std::endl;
        ofs << "    while (content.indexOf('\t') == '0')" << std::endl;
        ofs << "    {" << std::endl;
        ofs << "        tabs_to_remove += '\t';" << std::endl;
        ofs << "        content = content.substring(1);" << std::endl;
        ofs << "    }" << std::endl;
        ofs << "    var re = new RegExp('\n' + tabs_to_remove, 'g');" << std::endl;
        ofs << "    content = content.replace(re, '\n');" << std::endl;
        ofs << "    if(content.lastIndexOf(\"\n\")>0)" << std::endl;
        ofs << "        content = content.substring(0, content.lastIndexOf(\"\n\"));" << std::endl;
        ofs << "    pre_elements[i].outerHTML = '<pre class=\"' + pre_elements[i].className + '\">' + content + '</pre>';" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        pagesListPath = Path("site/css/", "dropmenu.css");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("site/css/dropmenu.css");
        ofs << "/*" << std::endl;
        ofs << "    Style for drop menus" << std::endl;
        ofs << "*/" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    unordered menu list" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "nav" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    max-width: 80% !important;" << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    background:#C8C8C8;" << std::endl;
        ofs << std::endl;
        ofs << "    border-style: solid;" << std::endl;
        ofs << "    border-width: 1px;" << std::endl;
        ofs << "    border-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 15px;" << std::endl;
        ofs << "    margin-top: 15px;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left:15px !important;" << std::endl;
        ofs << "    padding-right:15px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    max-width: 700px;" << std::endl;
        ofs << std::endl;
        ofs << "    /*-moz-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    -webkit-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);*/" << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-radius: 5px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    submenu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*hides and styles submenus*/" << std::endl;
        ofs << "ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: block;" << std::endl;
        ofs << "    visibility: hidden;" << std::endl;
        ofs << "    position: absolute;" << std::endl;
        ofs << "    opacity: 0;" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << "    padding-left:0px;" << std::endl;
        ofs << "    background:#888888;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*positions and reveals submenu when hovering drop menu list item*/" << std::endl;
        ofs << "li.dm_li:hover ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:absolute;" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    visibility: visible;" << std::endl;
        ofs << "    opacity: 1.0;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    links in the menu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles links for menu list items*/" << std::endl;
        ofs << "ul.drop_menu a" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    cursor: pointer;" << std::endl;
        ofs << "    text-decoration:none;" << std::endl;
        ofs << std::endl;
        ofs << "    outline: 0;" << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    transition-property: color;" << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*changes link text colour when hovering*/" << std::endl;
        ofs << "ul.drop_menu a:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    color:#D8D8D8;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#C8C8C8 !important;" << std::endl;
        ofs << "    float: left;" << std::endl;
        ofs << "    white-space:nowrap;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#888888 !important;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles text*/" << std::endl;
        ofs << "ul.drop_menu .dm_text" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    -webkit-user-select: none; /* Chrome/Safari */" << std::endl;
        ofs << "    -moz-user-select: none; /* Firefox */" << std::endl;
        ofs << "    -ms-user-select: none; /* IE10+ */" << std::endl;
        ofs << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu .dm_text:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*color:#D8D8D8;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    just menu list items" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles menu list items*/" << std::endl;
        ofs << "li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*padding-top: 4px;" << std::endl;
        ofs << "    padding-bottom: 4px;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles menu list items when hovering*/" << std::endl;
        ofs << "li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:relative;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        ofs.open("site/css/pagestyle.css");
        ofs << "html" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background: #484848;" << std::endl;
        ofs << "    overflow-y: scroll;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "body" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#E8E8E8;" << std::endl;
        ofs << std::endl;
        ofs << "    display: table;" << std::endl;
        ofs << std::endl;
        ofs << "    width: 80%;" << std::endl;
        ofs << "    max-width: 800px !important;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 40px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 15px;" << std::endl;
        ofs << "    padding-right: 15px;" << std::endl;
        ofs << "    padding-top: 1px;" << std::endl;
        ofs << "    padding-bottom: 10px;" << std::endl;
        ofs << std::endl;
        ofs << "    word-wrap:break-word;" << std::endl;
        ofs << std::endl;
        ofs << "    font-family:'Helvetica Neue',Helvetica,Arial,sans-serif;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "footer" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    text-align: center;" << std::endl;
        ofs << "    margin-top:20px;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        ofs.open("site/css/pre.css");
        ofs << "/*" << std::endl;
        ofs << "    see http://google-code-prettify.googlecode.com/svn/trunk/README.html" << std::endl;
        ofs << "    for the prettyprint readme." << std::endl;
        ofs << "*/" << std::endl;
        ofs << std::endl;
        ofs << "pre ol.linenums > li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*lists all line numbers*/" << std::endl;
        ofs << "    list-style-type: decimal;" << std::endl;
        ofs << std::endl;
        ofs << "    /*linenum font color*/" << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    border-left-style: solid;" << std::endl;
        ofs << "    border-left-width: 1px;" << std::endl;
        ofs << "    border-left-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 5px !important;" << std::endl;
        ofs << "    padding-top: 1px;" << std::endl;
        ofs << std::endl;
        ofs << "    /*sets background color for every second line*/" << std::endl;
        ofs << "    background: #C8C8C8;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.scroll" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    max-height: 400px;" << std::endl;
        ofs << "    overflow-y: scroll;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.inline" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: inline-block !important;" << std::endl;
        ofs << std::endl;
        ofs << "    white-space: pre;       /* css-3 */" << std::endl;
        ofs << "    -moz-only-whitespace: -moz-pre-wrap !important;  /* Mozilla, since 1999 */" << std::endl;
        ofs << "    white-space: -pre;      /* Opera 4-6 */" << std::endl;
        ofs << "    white-space: -o-pre;    /* Opera 7 */" << std::endl;
        ofs << "    word-wrap: pre;       /* Internet Explorer 5.5+ */" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.rounded" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    border-radius: 5px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: block;" << std::endl;
        ofs << "    text-align:left !important;" << std::endl;
        ofs << "    background: #C8C8C8;" << std::endl;
        ofs << std::endl;
        ofs << "    /*picks the first available font and resizes*/" << std::endl;
        ofs << "    font-family: DejaVu Sans Mono, Liberation Mono, Consolas, Ubuntu Mono, Courier New;" << std::endl;
        ofs << "    font-size: 85%;" << std::endl;
        ofs << "    tab-size: 4;" << std::endl;
        ofs << std::endl;
        ofs << "    max-width: 90%;" << std::endl;
        ofs << std::endl;
        ofs << "    white-space: pre-wrap;       /* css-3 */" << std::endl;
        ofs << "    white-space: -moz-pre-wrap;  /* Mozilla, since 1999 */" << std::endl;
        ofs << "    white-space: -pre-wrap;      /* Opera 4-6 */" << std::endl;
        ofs << "    white-space: -o-pre-wrap;    /* Opera 7 */" << std::endl;
        ofs << "    word-wrap: break-word;       /* Internet Explorer 5.5+ */" << std::endl;
        ofs << std::endl;
        ofs << "    border-style: solid;" << std::endl;
        ofs << "    border-width: 1px;" << std::endl;
        ofs << "    border-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-top: 1px;" << std::endl;
        ofs << "    margin-bottom: 1px;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 5px !important;" << std::endl;
        ofs << "    padding-right:5px !important;" << std::endl;
        ofs << "    padding-top:0 !important;" << std::endl;
        ofs << "    padding-bottom:0 !important;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        system("nsm track index");
        system("nsm build-updated");

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }

    if(cmd == "init-simple")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site in " << get_pwd() << "/" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        pagesListPath = Path("template/", "page.template");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("template/page.template");
        ofs << "<html>" << std::endl;
        ofs << "    <head>" << std::endl;
        ofs << "        @input(template/head.content)" << std::endl;
        ofs << "    </head>" << std::endl;
        ofs << std::endl;
        ofs << "    <body>" << std::endl;
        ofs << "        @input(template/menu.content)" << std::endl;
        ofs << std::endl;
        ofs << "        @inputcontent" << std::endl;
        ofs << std::endl;
        ofs << "        @input(template/footer.content)" << std::endl;
        ofs << std::endl;
        ofs << "        @jsinclude(site/js/process_pre_tags.js)" << std::endl;
        ofs << "    </body>" << std::endl;
        ofs << "</html>" << std::endl;
        ofs << std::endl;
        ofs.close();

        ofs.open("template/menu.content");
        ofs << "<nav>" << std::endl;
        ofs << "    <ul class=\"drop_menu\">" << std::endl;
        ofs << "        <li class=\"dm_li\"><a href=\"@pathto(index)\">home</a></li>" << std::endl;
        ofs << "        <li class=\"dm_li\"><a href=\"@pathto(about)\">about</a></li>" << std::endl;
        ofs << "    </ul>" << std::endl;
        ofs << "</nav>" << std::endl;
        ofs.close();

        ofs.open("template/head.content");
        if(noParams == 1)
            ofs << "<title>basic site - @pagetitle</title>" << std::endl;
        else if(noParams > 1)
            ofs << "<title>" << argv[2] << " - @pagetitle</title>" << std::endl;
        ofs << std::endl;
        ofs << "@cssinclude(site/css/pagestyle.css)" << std::endl;
        ofs << "@cssinclude(site/css/dropmenu.css)" << std::endl;
        ofs << std::endl;
        ofs << "<!-- uses MathJax CDC for LaTeX support" << std::endl;
        ofs << "     see http://docs.mathjax.org/en/latest/start.html for more info -->" << std::endl;
        ofs << "<script type=\"text/x-mathjax-config\">" << std::endl;
        ofs << "    MathJax.Hub.Config" << std::endl;
        ofs << "    ({" << std::endl;
        ofs << "        extensions: [\"tex2jax.js\"]," << std::endl;
        ofs << "        jax: [\"input/TeX\", \"output/HTML-CSS\"]," << std::endl;
        ofs << "        tex2jax:" << std::endl;
        ofs << "        {" << std::endl;
        ofs << "            inlineMath: [['$','$'], ['\\(','\\)']]," << std::endl;
        ofs << "            displayMath: [ ['$$','$$'], [\"\\[\",\"\\]\"] ]," << std::endl;
        ofs << "            processEscapes: true" << std::endl;
        ofs << "        }," << std::endl;
        ofs << "        \"HTML-CSS\": { availableFonts: [\"MathJax TeX\"] }" << std::endl;
        ofs << "    });" << std::endl;
        ofs << "</script>" << std::endl;
        ofs << "<script type=\"text/javascript\"" << std::endl;
        ofs << "        src=\"https://cdn.mathjax.org/mathjax/latest/MathJax.js\">" << std::endl;
        ofs << "</script>" << std::endl;
        ofs.close();

        ofs.open("template/footer.content");
        ofs << "<footer>" << std::endl;
        ofs << "    <hr>" << std::endl;
        ofs << "    <p>" << std::endl;
        ofs << "        Page last built on @currentdate at @currenttime (@timezone)." << std::endl;
        ofs << "    </p>" << std::endl;
        ofs << "</footer>" << std::endl;
        ofs.close();

        pagesListPath = Path("site/js/", "process_pre_tags.js");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("site/js/process_pre_tags.js");
        ofs << "/*" << std::endl;
        ofs << "    Has browser ignore leading tabs for <pre> code blocks." << std::endl;
        ofs << "    modified from: http://stackoverflow.com/a/8505182/4525897" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "var pre_elements = document.getElementsByTagName('pre');" << std::endl;
        ofs << "for (var i = 0; i < pre_elements.length; i++)" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    pre_elements = document.getElementsByTagName('pre');" << std::endl;
        ofs << "    var content = pre_elements[i].innerHTML;" << std::endl;
        ofs << "    var tabs_to_remove = '';" << std::endl;
        ofs << "    while (content.indexOf('\t') == '0')" << std::endl;
        ofs << "    {" << std::endl;
        ofs << "        tabs_to_remove += '\t';" << std::endl;
        ofs << "        content = content.substring(1);" << std::endl;
        ofs << "    }" << std::endl;
        ofs << "    var re = new RegExp('\n' + tabs_to_remove, 'g');" << std::endl;
        ofs << "    content = content.replace(re, '\n');" << std::endl;
        ofs << "    if(content.lastIndexOf(\"\n\")>0)" << std::endl;
        ofs << "        content = content.substring(0, content.lastIndexOf(\"\n\"));" << std::endl;
        ofs << "    pre_elements[i].outerHTML = '<pre class=\"' + pre_elements[i].className + '\">' + content + '</pre>';" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        pagesListPath = Path("site/css/", "dropmenu.css");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("site/css/dropmenu.css");
        ofs << "/*" << std::endl;
        ofs << "    Style for drop menus" << std::endl;
        ofs << "*/" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    unordered menu list" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "nav" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    max-width: 80% !important;" << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << std::endl;
        ofs << "    display:table;" << std::endl;
        ofs << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    background:#C8C8C8;" << std::endl;
        ofs << std::endl;
        ofs << "    border-style: solid;" << std::endl;
        ofs << "    border-width: 1px;" << std::endl;
        ofs << "    border-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 15px;" << std::endl;
        ofs << "    margin-top: 15px;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left:15px !important;" << std::endl;
        ofs << "    padding-right:15px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    max-width: 700px;" << std::endl;
        ofs << std::endl;
        ofs << "    /*-moz-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    -webkit-box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);" << std::endl;
        ofs << "    box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.9);*/" << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-radius: 5px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    submenu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*hides and styles submenus*/" << std::endl;
        ofs << "ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: block;" << std::endl;
        ofs << "    visibility: hidden;" << std::endl;
        ofs << "    position: absolute;" << std::endl;
        ofs << "    opacity: 0;" << std::endl;
        ofs << "    list-style-type:none;" << std::endl;
        ofs << "    padding-left:0px;" << std::endl;
        ofs << "    background:#888888;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*positions and reveals submenu when hovering drop menu list item*/" << std::endl;
        ofs << "li.dm_li:hover ul.drop_submenu" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:absolute;" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    visibility: visible;" << std::endl;
        ofs << "    opacity: 1.0;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    links in the menu" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles links for menu list items*/" << std::endl;
        ofs << "ul.drop_menu a" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display:block;" << std::endl;
        ofs << "    cursor: pointer;" << std::endl;
        ofs << "    text-decoration:none;" << std::endl;
        ofs << std::endl;
        ofs << "    outline: 0;" << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << std::endl;
        ofs << "    transition-property: color;" << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*changes link text colour when hovering*/" << std::endl;
        ofs << "ul.drop_menu a:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    color:#D8D8D8;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#C8C8C8 !important;" << std::endl;
        ofs << "    float: left;" << std::endl;
        ofs << "    white-space:nowrap;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.2s;" << std::endl;
        ofs << "    -ms-transition:0.2s;" << std::endl;
        ofs << "    -moz-transition:0.2s;" << std::endl;
        ofs << "    -webkit-transition:0.2s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.2s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#888888 !important;" << std::endl;
        ofs << std::endl;
        ofs << "    -o-transition:0.5s;" << std::endl;
        ofs << "    -ms-transition:0.5s;" << std::endl;
        ofs << "    -moz-transition:0.5s;" << std::endl;
        ofs << "    -webkit-transition:0.5s;" << std::endl;
        ofs << "    /* ...and now for the proper property */" << std::endl;
        ofs << "    transition:0.5s;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles text*/" << std::endl;
        ofs << "ul.drop_menu .dm_text" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    padding-left: 8px;" << std::endl;
        ofs << "    padding-right: 8px;" << std::endl;
        ofs << "    cursor: default;" << std::endl;
        ofs << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    -webkit-user-select: none; /* Chrome/Safari */" << std::endl;
        ofs << "    -moz-user-select: none; /* Firefox */" << std::endl;
        ofs << "    -ms-user-select: none; /* IE10+ */" << std::endl;
        ofs << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "ul.drop_menu .dm_text:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*color:#D8D8D8;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*" << std::endl;
        ofs << "    just menu list items" << std::endl;
        ofs << "*/" << std::endl;
        ofs << "/*styles menu list items*/" << std::endl;
        ofs << "li.dm_li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*padding-top: 4px;" << std::endl;
        ofs << "    padding-bottom: 4px;*/" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "/*styles menu list items when hovering*/" << std::endl;
        ofs << "li.dm_li:hover" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    position:relative;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        ofs.open("site/css/pagestyle.css");
        ofs << "html" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background: #484848;" << std::endl;
        ofs << "    overflow-y: scroll;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "body" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    background:#E8E8E8;" << std::endl;
        ofs << std::endl;
        ofs << "    display: table;" << std::endl;
        ofs << std::endl;
        ofs << "    width: 80%;" << std::endl;
        ofs << "    max-width: 800px !important;" << std::endl;
        ofs << "    margin: 0 auto;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-bottom: 40px !important;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 15px;" << std::endl;
        ofs << "    padding-right: 15px;" << std::endl;
        ofs << "    padding-top: 1px;" << std::endl;
        ofs << "    padding-bottom: 10px;" << std::endl;
        ofs << std::endl;
        ofs << "    word-wrap:break-word;" << std::endl;
        ofs << std::endl;
        ofs << "    font-family:'Helvetica Neue',Helvetica,Arial,sans-serif;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #202020;" << std::endl;
        ofs << "    border-bottom-left-radius: 8px;" << std::endl;
        ofs << "    border-bottom-right-radius: 8px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "footer" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    text-align: center;" << std::endl;
        ofs << "    margin-top:20px;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        ofs.open("site/css/pre.css");
        ofs << "/*" << std::endl;
        ofs << "    see http://google-code-prettify.googlecode.com/svn/trunk/README.html" << std::endl;
        ofs << "    for the prettyprint readme." << std::endl;
        ofs << "*/" << std::endl;
        ofs << std::endl;
        ofs << "pre ol.linenums > li" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    /*lists all line numbers*/" << std::endl;
        ofs << "    list-style-type: decimal;" << std::endl;
        ofs << std::endl;
        ofs << "    /*linenum font color*/" << std::endl;
        ofs << "    color:#484848;" << std::endl;
        ofs << std::endl;
        ofs << "    border-left-style: solid;" << std::endl;
        ofs << "    border-left-width: 1px;" << std::endl;
        ofs << "    border-left-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 5px !important;" << std::endl;
        ofs << "    padding-top: 1px;" << std::endl;
        ofs << std::endl;
        ofs << "    /*sets background color for every second line*/" << std::endl;
        ofs << "    background: #C8C8C8;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.scroll" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    max-height: 400px;" << std::endl;
        ofs << "    overflow-y: scroll;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.inline" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: inline-block !important;" << std::endl;
        ofs << std::endl;
        ofs << "    white-space: pre;       /* css-3 */" << std::endl;
        ofs << "    -moz-only-whitespace: -moz-pre-wrap !important;  /* Mozilla, since 1999 */" << std::endl;
        ofs << "    white-space: -pre;      /* Opera 4-6 */" << std::endl;
        ofs << "    white-space: -o-pre;    /* Opera 7 */" << std::endl;
        ofs << "    word-wrap: pre;       /* Internet Explorer 5.5+ */" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre.rounded" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    border-radius: 5px;" << std::endl;
        ofs << "}" << std::endl;
        ofs << std::endl;
        ofs << "pre" << std::endl;
        ofs << "{" << std::endl;
        ofs << "    display: block;" << std::endl;
        ofs << "    text-align:left !important;" << std::endl;
        ofs << "    background: #C8C8C8;" << std::endl;
        ofs << std::endl;
        ofs << "    /*picks the first available font and resizes*/" << std::endl;
        ofs << "    font-family: DejaVu Sans Mono, Liberation Mono, Consolas, Ubuntu Mono, Courier New;" << std::endl;
        ofs << "    font-size: 85%;" << std::endl;
        ofs << "    tab-size: 4;" << std::endl;
        ofs << std::endl;
        ofs << "    max-width: 90%;" << std::endl;
        ofs << std::endl;
        ofs << "    white-space: pre-wrap;       /* css-3 */" << std::endl;
        ofs << "    white-space: -moz-pre-wrap;  /* Mozilla, since 1999 */" << std::endl;
        ofs << "    white-space: -pre-wrap;      /* Opera 4-6 */" << std::endl;
        ofs << "    white-space: -o-pre-wrap;    /* Opera 7 */" << std::endl;
        ofs << "    word-wrap: break-word;       /* Internet Explorer 5.5+ */" << std::endl;
        ofs << std::endl;
        ofs << "    border-style: solid;" << std::endl;
        ofs << "    border-width: 1px;" << std::endl;
        ofs << "    border-color: #888888;" << std::endl;
        ofs << std::endl;
        ofs << "    margin-top: 1px;" << std::endl;
        ofs << "    margin-bottom: 1px;" << std::endl;
        ofs << std::endl;
        ofs << "    padding-left: 5px !important;" << std::endl;
        ofs << "    padding-right:5px !important;" << std::endl;
        ofs << "    padding-top:0 !important;" << std::endl;
        ofs << "    padding-bottom:0 !important;" << std::endl;
        ofs << std::endl;
        ofs << "    -moz-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    -webkit-box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "    box-shadow: 3px 3px 5px #484848;" << std::endl;
        ofs << "}" << std::endl;
        ofs.close();

        system("nsm track index home");
        system("nsm track about");
        system("nsm build-updated");

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }

    //ensures nsm is managing a site from this directory or one of the parent directories
    std::string parentDir = "../",
        rootDir = "/",
        owd = get_pwd(),
        pwd = get_pwd(),
        prevPwd;

    while(!std::ifstream(".siteinfo/pages.list") && !std::ifstream(".siteinfo/nsm.config"))
    {
        //sets old pwd
        prevPwd = pwd;

        //changes to parent directory
        chdir(parentDir.c_str());

        //gets new pwd
        pwd = get_pwd();

        //checks we haven't made it to root directory or stuck at same dir
        if(pwd == rootDir || pwd == prevPwd)
        {
            std::cout << "nsm is not managing a site from " << owd << " (or any accessible parent directories)" << std::endl;
            return 1;
        }
    }

    //ensures both pages.list and nsm.config exist
    if(!std::ifstream(".siteinfo/pages.list"))
    {
        std::cout << "error: " << get_pwd() << "/.siteinfo/pages.list is missing" << std::endl;
        return 1;
    }

    if(!std::ifstream(".siteinfo/nsm.config"))
    {
        std::cout << "error: " << get_pwd() << "/.siteinfo/nsm.config is missing" << std::endl;
        return 1;
    }

    //opens up site information
    SiteInfo site;
    if(site.open() > 0)
        return 1;

    if(cmd == "config")
    {
        std::cout << "contentDir: " << site.contentDir << std::endl;
        std::cout << "contentExt: " << site.contentExt << std::endl;
        std::cout << "siteDir: " << site.siteDir << std::endl;
        std::cout << "pageExt: " << site.pageExt << std::endl;
        std::cout << "defaultTemplate: " << site.defaultTemplate << std::endl;

        return 0;
    }
    if(cmd == "status")
    {
        //ensures correct number of parameters given
        if(noParams != 1)
            return parError(noParams, argv, "1");

        return site.status();
    }
    else if(cmd == "info")
    {
        //ensures correct number of parameters given
        if(noParams <= 1)
            return parError(noParams, argv, ">1");

        std::vector<Name> pageNames;
        Name pageName;
        for(int p=2; p<argc; p++)
            pageNames.push_back(argv[p]);
        return site.info(pageNames);
    }
    else if(cmd == "info-all")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        return site.info_all();
    }
    else if(cmd == "info-names")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        return site.info_names();
    }
    else if(cmd == "track")
    {
        //ensures correct number of parameters given
        if(noParams < 2 || noParams > 4)
            return parError(noParams, argv, "2-4");

        Name newPageName = argv[2];
        Title newPageTitle;
        if(noParams >= 3)
            newPageTitle = argv[3];
        else
            newPageTitle = get_title(newPageName);

        Path newTemplatePath;
        if(noParams == 4)
            newTemplatePath.set_file_path_from(argv[4]);
        else
            newTemplatePath = site.defaultTemplate;

        return site.track(newPageName, newPageTitle, newTemplatePath);
    }
    else if(cmd == "untrack")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        Name pageNameToUntrack = argv[2];

        return site.untrack(pageNameToUntrack);
    }
    else if(cmd == "rm")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        Name pageNameToRemove = argv[2];

        return site.rm(pageNameToRemove);
    }
    else if(cmd == "mv")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Name oldPageName = argv[2],
             newPageName = argv[3];

        return site.mv(oldPageName, newPageName);
    }
    else if(cmd == "cp")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Name trackedPageName = argv[2],
             newPageName = argv[3];

        return site.cp(trackedPageName, newPageName);
    }
    else if(cmd == "new-title")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Name pageName = argv[2];
        Title newTitle;
        newTitle.str = argv[3];

        return site.new_title(pageName, newTitle);
    }
    else if(cmd == "new-template")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Name pageName = argv[2];
        Path newTemplatePath;
        newTemplatePath.set_file_path_from(argv[3]);

        return site.new_template(pageName, newTemplatePath);
    }
    else if(cmd == "build-updated")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        return site.build_updated();
    }
    else if(cmd == "build")
    {
        //ensures correct number of parameters given
        if(noParams <= 1)
            return parError(noParams, argv, ">1");

        std::vector<Name> pageNamesToBuild;
        for(int p=2; p<argc; p++)
        {
            pageNamesToBuild.push_back(argv[p]);
        }
        return site.build(pageNamesToBuild);
    }
    else if(cmd == "build-all")
    {
        //ensures correct number of parameters given
        if(noParams != 1)
            return parError(noParams, argv, "1");

        return site.build_all();
    }
    else
    {
        unrecognisedCommand("nsm", cmd);
    }

    return 0;
}
