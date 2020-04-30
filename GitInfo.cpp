#include "GitInfo.h"

bool is_git_configured()
{
    //checks that git is configured
    std::string str;
    int ret_val = system("git config --global user.email > .nsm-git-email.txt");
    if(ret_val)
    {
        start_err(std::cout) << "is_git_configured(): system('git config --global user.email > .nsm-git-email.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_file(Path("./", ".nsm-git-email.txt"));
        return 0;
    }
    std::ifstream ifs(".nsm-git-email.txt");
    ifs >> str;
    ifs.close();
    remove_file(Path("./", ".nsm-git-email.txt"));
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
        start_err(std::cout) << "is_git_configured(): system('git config --global user.name > .nsm-git-username.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_file(Path("./", ".nsm-git-username.txt"));
        return 0;
    }
    ifs.open(".nsm-git-username.txt");
    ifs >> str;
    ifs.close();
    remove_file(Path("./", ".nsm-git-username.txt"));
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
        start_err(std::cout) << "is_git_remote_set(): system('git config --get remote.origin.url > .nsm-git-remote-url.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_file(Path("./", ".nsm-git-remote-url.txt"));
        return 0;
    }
    std::ifstream ifs(".nsm-git-remote-url.txt");
    std::string str="";
    ifs >> str;
    ifs.close();
    remove_file(Path("./", ".nsm-git-remote-url.txt"));
    if(str == "")
    {
        start_err(std::cout) << "no remote git url set" << std::endl;
        return 0;
    }

    return 1;
}

//get present git branch
int get_pb(std::string& branch)
{
    int ret_val = system("git symbolic-ref HEAD --short > .nsm-present-branch.txt");
    if(ret_val)
    {
        start_err(std::cout) << "get_pb(): system('git symbolic-ref HEAD --short > .nsm-present-branch.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_file(Path("./", ".nsm-present-branch.txt"));
        return ret_val;
    }

    std::ifstream ifs(".nsm-present-branch.txt");
    ifs >> branch;
    ifs.close();
    remove_file(Path("./", ".nsm-present-branch.txt"));

    return 0;
}

//get present git remote
int get_remote(std::string& remote)
{
    int ret_val = system("git remote > .nsm-git-remote.txt");
    if(ret_val)
    {
        start_err(std::cout) << "get_remote(): system('git remote > .nsm-git-remote.txt') failed in " << quote(get_pwd()) << std::endl;
        remove_file(Path("./", ".nsm-git-remote.txt"));
        return ret_val;
    }

    std::ifstream ifs(".nsm-git-remote.txt");

    ifs >> remote;

    ifs.close();
    remove_file(Path("./", ".nsm-git-remote.txt"));

    return 0;
}

//get set of git branches
int get_git_branches(std::set<std::string>& branches)
{
    branches.clear();
    std::string branch;

    int ret_val = system("git show-ref > .nsm-git-branches.txt");
    if(ret_val)
    {
        start_err(std::cout) << "system(\"git show-ref > .nsm-git-branches.txt\") failed" << std::endl;
        return 1;
    }

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

    remove_file(Path("./", ".nsm-git-branches.txt"));

    return 0;
}
