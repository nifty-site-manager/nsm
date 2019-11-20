#include "GitInfo.h"

bool is_git_configured()
{
    //checks that git is configured
    std::string str;
    int ret_val = system("git config --global user.email > .1223fsf23.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_configured(): system('git config --global user.email > .1223fsf23.txt') failed in " << quote(get_pwd()) << std::endl;
        Path("./", ".1223fsf23.txt").removePath();
        return 0;
    }
    std::ifstream ifs(".1223fsf23.txt");
    ifs >> str;
    ifs.close();
    Path("./", ".1223fsf23.txt").removePath();
    if(str == "")
    {
        std::cout << "*** Please tell me who you are." << std::endl;
        std::cout << std::endl;
        std::cout << "Run" << std::endl;
        std::cout << std::endl;
        std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
        std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
        std::cout << std::endl;
        std::cout << "to set your account's default identity." << std::endl;

        return 0;
    }
    ret_val = system("git config --global user.name > .1223fsf23.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_configured(): system('git config --global user.name > .1223fsf23.txt') failed in " << quote(get_pwd()) << std::endl;
        Path("./", ".1223fsf23.txt").removePath();
        return 0;
    }
    ifs.open(".1223fsf23.txt");
    ifs >> str;
    ifs.close();
    Path("./", ".1223fsf23.txt").removePath();
    if(str == "")
    {
        std::cout << "*** Please tell me who you are." << std::endl;
        std::cout << std::endl;
        std::cout << "Run" << std::endl;
        std::cout << std::endl;
        std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
        std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
        std::cout << std::endl;
        std::cout << "to set your account's default identity." << std::endl;

        return 0;
    }

    return 1;
}

bool is_git_remote_set()
{
    //checks that remote git url is set
    int ret_val = system("git config --get remote.origin.url > .txt65232g42f.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_remote_set(): system('git config --get remote.origin.url > .txt65232g42f.txt') failed in " << quote(get_pwd()) << std::endl;
        Path("./", ".txt65232g42f.txt").removePath();
        return 0;
    }
    std::ifstream ifs(".txt65232g42f.txt");
    std::string str="";
    ifs >> str;
    ifs.close();
    Path("./", ".txt65232g42f.txt").removePath();
    if(str == "")
    {
        std::cout << "error: no remote git url set" << std::endl;
        return 0;
    }

    return 1;
}

//get present git branch
std::string get_pb()
{
    std::string branch = "";

    int ret_val = system("git status > .f242tgg43.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: get_pb(): system('git status > .f242tgg43.txt') failed in " << quote(get_pwd()) << std::endl;
        Path("./", ".f242tgg43.txt").removePath();
        return "##error##";
    }

    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
        if(branch == "branch")
            break;
    if(branch == "branch")
        ifs >> branch;

    ifs.close();
    Path("./", ".f242tgg43.txt").removePath();

    return branch;
}

//get present git remote
std::string get_remote()
{
    std::string remote = "";

    int ret_val = system("git remote > .f242tgg43.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: get_remote(): system('git remote > .f242tgg43.txt') failed in " << quote(get_pwd()) << std::endl;
        Path("./", ".f242tgg43.txt").removePath();
        return "##error##";
    }

    std::ifstream ifs(".f242tgg43.txt");

    ifs >> remote;

    ifs.close();
    Path("./", ".f242tgg43.txt").removePath();

    return remote;
}

//get set of git branches
std::set<std::string> get_git_branches()
{
    std::set<std::string> branches;
    std::string branch = "";

    int ret_val = system("git show-ref > .f242tgg43.txt");
    ret_val = ret_val + 1; //gets rid of annoying warning
    //can't handle error here because returns error when there's no branches

    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
    {
        ifs >> branch;
        int pos = branch.find_last_of('/') + 1;
        branch = branch.substr(pos, branch.size() - pos);
        if(branch != "*" && branch != "->" && branch != "HEAD")
            branches.insert(branch);
    }

    ifs.close();

    Path("./", ".f242tgg43.txt").removePath();

    return branches;
}
