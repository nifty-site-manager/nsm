#include "GitInfo.h"

bool is_git_configured()
{
    //checks that git is configured
    std::string str;
    int ret_val = system("git config --global user.email > .nsm-git-email.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_configured(): system('git config --global user.email > .nsm-git-email.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_path(Path("./", ".nsm-git-email.txt"));
        return 0;
    }
    std::ifstream ifs(".nsm-git-email.txt");
    ifs >> str;
    ifs.close();
    remove_path(Path("./", ".nsm-git-email.txt"));
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
    ret_val = system("git config --global user.name > .nsm-git-username.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_configured(): system('git config --global user.name > .nsm-git-username.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_path(Path("./", ".nsm-git-username.txt"));
        return 0;
    }
    ifs.open(".nsm-git-username.txt");
    ifs >> str;
    ifs.close();
    remove_path(Path("./", ".nsm-git-username.txt"));
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
    int ret_val = system("git config --get remote.origin.url > .nsm-git-remote-url.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: is_git_remote_set(): system('git config --get remote.origin.url > .nsm-git-remote-url.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_path(Path("./", ".nsm-git-remote-url.txt"));
        return 0;
    }
    std::ifstream ifs(".nsm-git-remote-url.txt");
    std::string str="";
    ifs >> str;
    ifs.close();
    remove_path(Path("./", ".nsm-git-remote-url.txt"));
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
    std::string branch = "##not-found##";

    int ret_val = system("git status > .nsm-present-branch.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: get_pb(): system('git status > .nsm-present-branch.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_path(Path("./", ".nsm-present-branch.txt"));
        return "##error##";
    }

    std::ifstream ifs(".nsm-present-branch.txt");

    while(ifs >> branch)
        if(branch == "branch")
            break;
    if(branch == "branch")
        ifs >> branch;

    ifs.close();
    remove_path(Path("./", ".nsm-present-branch.txt"));

	//developer can choose whether to output error message if no branches found

    return branch;
}

//get present git remote
std::string get_remote()
{
    std::string remote = "##not-found##";

    int ret_val = system("git remote > .nsm-git-remote.txt");
    if(ret_val)
    {
        std::cout << "error: nsm.cpp: get_remote(): system('git remote > .nsm-git-remote.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_path(Path("./", ".nsm-git-remote.txt"));
        return "##error##";
    }

    std::ifstream ifs(".nsm-git-remote.txt");

    ifs >> remote;

    ifs.close();
    remove_path(Path("./", ".nsm-git-remote.txt"));

	//developer can choose whether to output error message if no branches found

    return remote;
}

//get set of git branches
std::set<std::string> get_git_branches()
{
    std::set<std::string> branches;
    std::string branch = "";

    int ret_val = system("git show-ref > .nsm-git-branches.txt");
    ret_val = ret_val + 1; //gets rid of annoying warning
    //can't handle error here because returns error when there's no branches

    std::ifstream ifs(".nsm-git-branches.txt");

    while(ifs >> branch)
    {
        ifs >> branch;
        int pos = branch.find_last_of('/') + 1;
        branch = branch.substr(pos, branch.size() - pos);
        if(branch != "*" && branch != "->" && branch != "HEAD")
            branches.insert(branch);
    }

    ifs.close();

    remove_path(Path("./", ".nsm-git-branches.txt"));

    return branches;
}
