/*
	Nift (aka nsm) is a cross-platform open source
	command-line website and project generator.

	Official Website: https://nift.dev
	GitHub: https://github.com/nifty-site-manager/nsm

	Copyright (c) 2015-present
	Creator: Nicholas Ham
	https://n-ham.com
*/

#include "GitInfo.h"
#include "ProjectInfo.h"

std::atomic<bool> auto_build;
std::mutex auto_build_mtx;

int read_build_auto_commands()
{
	char cmd;

	std::cout << "auto building project locally - 'q' or 's' to stop ('ctrl c' to kill)" << std::endl;

	while(1)
	{
		#if defined _WIN32 || defined _WIN64
			cmd = _getch();
		#else
			enable_raw_mode();
			cmd = getchar();
			disable_raw_mode();
		#endif

		if(cmd != 'q' && cmd != 's')
			std::cout << "unrecognised command - 'q' or 's' to stop ('ctrl c' to kill)" << std::endl;
		else
			break;
	}

	auto_build = 0;

	return 0;
}

int sleepTime = 300000; //default value: 0.3 seconds

int build_auto()
{
	std::ofstream ofs;

	while(auto_build)
	{
		ProjectInfo project;
		if(project.open(1))
		{
			start_err(std::cout) << "build_auto(): failed to open project, no longer serving" << std::endl;
			return 1;
		}

		Parser parser(&project.trackedAll, 
		          &auto_build_mtx, 
		          project.contentDir, 
		          project.outputDir, 
		          project.contentExt, 
		          project.outputExt, 
		          project.scriptExt, 
		          project.defaultTemplate, 
		          project.backupScripts, 
		          project.unixTextEditor, 
		          project.winTextEditor);

		ofs.open(".build-auto-log.txt");

		if(!parser.run_script(ofs, Path("", "pre-build" + project.scriptExt), project.backupScripts, 1))
			if(!project.build_updated(ofs, 0, 1, 1))
				parser.run_script(ofs, Path("", "post-build" + project.scriptExt), project.backupScripts, 1);

		ofs.close();

		usleep(sleepTime);
	}

	remove_file(Path("./", ".build-auto-log.txt"));

	return 0;
}

void unrecognisedCommand(const std::string& cmd)
{
	start_err(std::cout) << "Nift does not recognise the command " << quote(cmd) << std::endl;
}

bool parError(int noParams, const char* argv[], const std::string& expectedNo)
{
	start_err(std::cout) << noParams;
	if(noParams == 1)
		std::cout << " parameter";
	else
		std::cout << " parameters";
	std::cout << " is not the " << expectedNo;
	if(expectedNo == "1")
		std::cout << " parameter";
	else
		std::cout << " parameters";
	std::cout << " expected" << std::endl;
	std::cout << c_purple << "parameters" << c_white << ":";
	for(int p=1; p<=noParams; p++)
		std::cout << " " << argv[p];
	std::cout << std::endl;
	return 1;
}

void asciiNift()
{
	srand(time(NULL));
	int c = rand()%5,
	    T = 3;

	if(console_width() < 23 || console_height() < 12)
		return;
	else if(console_width() < 26)
		T = 2;
	else if(console_width() < 30)
		T = 3;
	else
		T = 4;

	int t = rand()%T;

	std::string asciiNift;

	if(t == 0)
	{
		asciiNift += "         _  _____     \n";
		asciiNift += "   ____ (_)/ __/ |    \n";
		asciiNift += "  |  _ \\ _ | |_| |_   \n";
		asciiNift += "  | | | | || _/  __)  \n";
		asciiNift += "  |_| | |_|| | | |_   \n";
		asciiNift += "      |/   |/  |___)  \n";
	}
	else if(t == 1)
	{
		asciiNift += "         _  _____     \n";
		asciiNift += "   ____ |_|/ __/ |    \n";
		asciiNift += "  |  _ \\ _ | |_| |__  \n";
		asciiNift += "  | | | | || _/  __/  \n";
		asciiNift += "  |_| | |_|| | | |__  \n";
		asciiNift += "      |/   |/  |___/  \n";
	}
	else if(t == 2)
	{
		asciiNift += "   ____ ____ ____ ____  \n";
		asciiNift += "  ||n |||i |||f |||t || \n";
		asciiNift += "  ||__|||__|||__|||__|| \n";
		asciiNift += "  |/__\\|/__\\|/__\\|/__\\| \n";
	}
	else if(t == 3)
	{
		asciiNift += "         _________________  \n";
		asciiNift += "  __________(_)  / __/_/ /_ \n";
		asciiNift += "  __/ __ \\ __ __/ /_ _  __/ \n";
		asciiNift += "  _  / / // / _  __/ / /_   \n";
		asciiNift += "  /_/ /_//_/  /_/    \\__/   \n";
	}

	if(c == 0)
		std::cout << c_gold << asciiNift;
	else if(c == 1)
		std::cout << c_green << asciiNift;
	else if(c == 2)
		std::cout << c_light_blue << asciiNift;
	else if(c == 3)
		std::cout << c_purple << asciiNift;
	else if(c == 4)
		std::cout << c_red << asciiNift;

	std::cout << c_white << std::endl;
}

int main(int argc, const char* argv[])
{
	Timer timer;
	timer.start();

	Path globalConfigPath(app_dir() + "/.nift/", "config.json");
	if(file_exists(app_dir() + "/.nift/nift.config") && upgrade_global_config())
		return 1;
	else if(!file_exists(globalConfigPath.str()))
		create_config_file(globalConfigPath, "", 1);

	int noParams = argc-1;
	int ret_val;

	if(noParams == 0)
	{
		std::cout << "no commands given, nothing to do" << std::endl;
		return 0;
	}

	std::string cmd = argv[1];
	if(cmd.size() && cmd[0] == '-')
	{
		for(size_t i=1; i<cmd.size(); ++i)
		{
			if(cmd[i] != '-')
			{
				cmd = cmd.substr(i, cmd.size()-i);
				break;
			}
		}
	}

	if(get_pwd() == "/")
	{
		start_err(std::cout) << "do not run Nift from the root directory!" << std::endl;
		return 1;
	}

	//Nift commands that can run from anywhere
	if(cmd == "version")
	{
		#if defined _WIN32 || defined _WIN64
			if(ProjectInfo().open_global_config(0))
				return 1;
		#endif

		std::cout << "Nift (aka nsm) (c)" << DateTimeInfo().currentYYYY() << " " << c_gold << "v" << NSM_VERSION << c_white << std::endl;

		return 0;
	}
	else if(cmd == "about" || cmd == "help")
	{
		#if defined _WIN32 || defined _WIN64
			if(ProjectInfo().open_global_config(0))
				return 1;
		#endif

		if(console_width() > 22 && console_height() > 12)
			asciiNift();

		std::cout << "Nift (aka nifty-site-manager or nsm) (c)" << DateTimeInfo().currentYYYY();
		std::cout << " is a cross-platform open source website and project generator" << std::endl;
		std::cout << "Official Website: " << c_blue << "https://nift.dev/" << c_white << std::endl;
		std::cout << "Source: " << c_blue << "https://github.com/nifty-site-manager/nsm" << c_white << std::endl;
		std::cout << "Installed: " << c_gold << "v" << NSM_VERSION << c_white << std::endl;
		std::cout << "enter `nsm commands` or `nift commands` for available commands" << std::endl;

		return 0;
	}
	else if(cmd == "commands" || cmd == "cmds")
	{
		std::cout << "+--------- available commands ---------------------------------+" << std::endl;
		std::cout << "| commands or cmds  | list all commands                        |" << std::endl;
		std::cout << "| config            | configure Nift                           |" << std::endl;
		std::cout << "| clone             | par: clone-url                           |" << std::endl;
		std::cout << "| diff              | par: file-path                           |" << std::endl;
		std::cout << "| pull              | pull remote changes locally              |" << std::endl;
		std::cout << "| bcp               | par: commit-message                      |" << std::endl;
		std::cout << "| mbcp              | par: dir-1 .. dir-k commit-message       |" << std::endl;
		std::cout << "| init              | start managing project - par: (out-ext)  |" << std::endl;
		std::cout << "| init-html         | start managing html website              |" << std::endl;
		std::cout << "| status            | list updated & problem files             |" << std::endl;
		std::cout << "| info              | par: name-1 .. name-k                    |" << std::endl;
		std::cout << "| info-all          | list watched dirs & tracked files        |" << std::endl;
		std::cout << "| info-names        | list tracked names                       |" << std::endl;
		std::cout << "| info-tracking     | list tracked files                       |" << std::endl;
		std::cout << "| info-watching     | list watched dirs                        |" << std::endl;
		std::cout << "| track             | par: name (title) (template)             |" << std::endl;
		std::cout << "| track-from-file   | par: file-path                           |" << std::endl;
		std::cout << "| track-dir         | par: dir (cont-ext) (template) (out-ext) |" << std::endl;
		std::cout << "| untrack           | par: name                                |" << std::endl;
		std::cout << "| untrack-from-file | par: file-path                           |" << std::endl;
		std::cout << "| untrack-dir       | par: dir-path (content-ext)              |" << std::endl;
		std::cout << "| rmv               | par: name                                |" << std::endl;
		std::cout << "| rmv-from-file     | par: file-path                           |" << std::endl;
		std::cout << "| rmv-dir           | par: dir-path (content-ext)              |" << std::endl;
		std::cout << "| mve               | par: old-name new-name                   |" << std::endl;
		std::cout << "| cpy               | par: tracked-name new-name               |" << std::endl;
		std::cout << "| run               | par: (lang-opt) script-path              |" << std::endl;
		std::cout << "| interp            | par: (lang-opt)                          |" << std::endl;
		std::cout << "| sh                | par: (lang-opt)                          |" << std::endl;
		std::cout << "| build-names       | par: name-1 .. name-k                    |" << std::endl;
		std::cout << "| build(-updated)   | build updated output files               |" << std::endl;
		std::cout << "| build-all         | build all tracked output files           |" << std::endl;
		std::cout << "| build-auto        | par: (sleep-sec)                         |" << std::endl;
		std::cout << "| browse            | browse page: (name or path)              |" << std::endl;
		std::cout << "| mve-output-dir    | par: dir-path                            |" << std::endl;
		std::cout << "| mve-cont-dir      | par: dir-path                            |" << std::endl;
		std::cout << "| new-title         | par: name new-title                      |" << std::endl;
		std::cout << "| new-template      | par: (name) template                     |" << std::endl;
		std::cout << "| new-cont-ext      | par: (name) content-ext                  |" << std::endl;
		std::cout << "| new-output-ext    | par: (name) output-ext                   |" << std::endl;
		std::cout << "| new-script-ext    | par: (name) script-ext                   |" << std::endl;
		std::cout << "| no-build-thrds    | par: (no-threads) [-n == n*cores]        |" << std::endl;
		std::cout << "| backup-scripts    | par: (option)                            |" << std::endl;
		std::cout << "| incr-mode         | par: (mode)                              |" << std::endl;
		std::cout << "| watch             | par: dir (cont-ext) (template) (out-ext) |" << std::endl;
		std::cout << "| unwatch           | par: dir (cont-ext)                      |" << std::endl;
		std::cout << "| edit or open      | par: name-1 .. name-k                    |" << std::endl;
		std::cout << "| medit or mopen    | par: dir-1 .. dir-k name                 |" << std::endl;
		std::cout << "| lolcat            | par: (options) used when no other lolcat |" << std::endl;
		std::cout << "|                   |                versions are installed    |" << std::endl;
		std::cout << "+--------------------------------------------------------------+" << std::endl;

		return 0;
	}

	if(cmd == "git" || cmd == "luarocks")
	{
		std::string sysStr = cmd;

		for(int p=2; p<=noParams; ++p)
			sysStr += " " + std::string(argv[p]);

		return system(sysStr.c_str());
	}

	if(cmd == "lolcat" || cmd == "lolcat-cc")
	{
		return lolmain(argc, argv);
	}

	//can delete init-html once we've updated homebrew test
	if(cmd == "init" || cmd == "init-html")
	{
		//ensures correct number of parameters given
		if(noParams != 1 && noParams != 2)
			return parError(noParams, argv, "1-2");

		if(file_exists(".nift/config.json") ||
		   file_exists(".nsm/nift.config"))
		{
			start_err(std::cout) << "Nift is already managing a project in " << get_pwd() << std::endl;
			return 1;
		}

		//sets up
		std::string outputExt;

		if(noParams == 1)
			outputExt = ".html";
		else if(noParams == 2)
		{
			if(std::string(argv[2]).size() == 0 || argv[2][0] != '.')
			{
				start_err(std::cout) << "project output extension should start with a fullstop" << std::endl;
				return 1;
			}

			outputExt = argv[2];
		}

		Path trackingListPath(".nift/", "tracked.json");
		//ensures tracking list file exists
		trackingListPath.ensureFileExists();

		#if defined _WIN32 || defined _WIN64
			ret_val = system("attrib +h .nift");

			//if handling error here need to check what happens in all of cmd prompt/power shell/git bash/cygwin/etc.
		#endif

		Path configPath(".nift/", "config.json");
		create_config_file(configPath, outputExt, 0);

		std::ofstream ofs(".nift/tracked.json");
		ofs << "{\"tracked\":[]}";
		ofs.close();

		if(outputExt == ".html")
		{
			Path templatePath("templates/", "template.html");
			create_default_html_template(templatePath);

			ProjectInfo project;
			if(project.open(1) > 0)
				return 1;

			Name name = "/";
			Title title("index");
			project.track(name, title, templatePath);

			name = "assets/css/style";
			title = get_title(name);
			project.track(name, title, Path("templates/", "template.css"), ".css", ".css");

			name = "assets/js/script";
			title = get_title(name);
			project.track(name, title, Path("templates/", "template.js"), ".js", ".js");


			project.build_all(std::cout, 0);
		}
		else
		{
			Path templatePath("templates/", "template" + outputExt);
			create_blank_template(templatePath);
		}

		std::cout << "initialised empty project in " << get_pwd() << std::endl;

		return 0;
	}
	else if(cmd == "config" && noParams > 1 && std::string(argv[2]) != "project")
	{
		std::string str = argv[2];

		//ensures correct number of parameters given
		if(noParams == 2)
		{
			if(str == "global")
			{
				ProjectInfo project;
				if(project.open_global_config(1) > 0)
					return 1;

				std::string editor;
				#if defined _WIN32 || defined _WIN64
					editor = project.winTextEditor;
				#else
					editor = project.unixTextEditor;
				#endif

				ret_val = std::system((editor + " " + app_dir() + "/.nift/config.json").c_str());

				if(ret_val)
					start_err(std::cout) << "config: system('" << editor << " " << app_dir() << "/.nift/config.json) failed" << std::endl;

				return ret_val;
			}
			else if(str != "project")
			{
				unrecognisedCommand(cmd + " " + argv[2]);
				return 1;	
			}
		}
		else if((noParams == 3 || noParams == 4) && str == "--global")
		{
			str = argv[3];
			if(str == "user.email")
			{
				if(noParams == 3)
				{
					ret_val = system("git config --global user.email");
					if(ret_val)
					{
						start_err(std::cout) << "config: system('git config --global user.email') failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}
				}
				else
				{
					std::string cmdStr = "git config --global user.email \"" + std::string(argv[4]) + "\"";
					ret_val = system(cmdStr.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "config: system(" << quote(cmdStr) << ") failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}
				}
			}
			else if(str == "user.name")
			{
				if(noParams == 3)
				{
					ret_val = system("git config --global user.name");
					if(ret_val)
					{
						start_err(std::cout) << "config: system('git config --global user.name') failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}
				}
				else
				{
					std::string cmdStr = "git config --global user.name \"" + std::string(argv[4]) + "\" --replace-all";
					ret_val = system(cmdStr.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "config: system(" << quote(cmdStr) << ") failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}
				}
			}
			else
			{
				std::cout << "you might have meant one of:" << std::endl;
				std::cout << "  nsm config --global user.email" << std::endl;
				std::cout << "  nsm config --global user.name" << std::endl;
				std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
				std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
				return parError(noParams, argv, "1, 3 or 4");
			}
		}
		else
		{
			unrecognisedCommand(cmd + " " + argv[2]);
			return 1;
		}
	}
	else if(cmd == "clone")
	{
		//ensures correct number of parameters given
		if(noParams != 2)
			return parError(noParams, argv, "2");

		if(!is_git_configured())
			return 1;

		std::string cloneURL = std::string(argv[2]);

		if(cloneURL.size() && cloneURL[0] == '?')
			cloneURL = cloneURL.substr(1, cloneURL.size()-1);

		std::string cloneCmnd = "git clone " + cloneURL;
		std::string dirName = "";
		std::string parDir = "../";

		if(cloneCmnd.find('/') == std::string::npos)
		{
			start_err(std::cout) << "no / found in provided clone url" << std::endl;
			return 1;
		}
		else if(cloneCmnd.find_last_of('/') > cloneCmnd.find_last_of('.'))
		{
			start_err(std::cout) << "clone url should contain .git after the last /" << std::endl;
			return 1;
		}

		for(auto i=cloneCmnd.find_last_of('/')+1; i < cloneCmnd.find_last_of('.'); ++i)
			dirName += cloneCmnd[i];

		if(dir_exists(dirName))
		{
			start_err(std::cout) << "destination path " << quote(dirName) << " already exists" << std::endl;
			return 1;
		}

		ret_val = system(cloneCmnd.c_str());
		if(ret_val)
		{
			start_err(std::cout) << "clone: system(" << quote(cloneCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
			return ret_val;
		}

		ret_val = chdir(dirName.c_str());
		if(ret_val)
		{
			start_err(std::cout) << "clone: failed to change directory to " << quote(dirName) << " from " << quote(get_pwd()) << std::endl;
			return ret_val;
		}

		std::string obranch;
		std::set<std::string> branches;

		if(get_pb(obranch))
		{
			start_err(std::cout) << "clone: get_pb() failed in repository root directory " << quote(get_pwd()) << std::endl;
			return 1;
		}

		if(obranch == "")
		{
			start_err(std::cout) << "clone: no branch found in repository root directory " << quote(get_pwd()) << std::endl;
			return 1;
		}

		if(get_git_branches(branches)) 
		{
			start_err(std::cout) << "clone: get_git_branches() failed in " << quote(get_pwd()) << std::endl;
			return 1;
		}
		else if(!branches.size()) 
		{
			std::cout << "no branches found, cloning finished" << std::endl;
			return 0;
		}

		//looks for root branch
		std::string checkoutCmnd;
		for(auto branch=branches.begin(); branch!=branches.end(); branch++)
		{
			checkoutCmnd = "git checkout " + *branch + " > /dev/null 2>&1 >nul 2>&1";
			ret_val = system(checkoutCmnd.c_str());
			if(file_exists("./nul"))
				remove_file(Path("./", "nul"));
			if(ret_val)
			{
				start_err(std::cout) << "clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			if(file_exists(".nift/config.json") ||
			   file_exists(".nsm/nift.config")) //found project root
			{
				if(file_exists(".nsm/nift.config") && upgrade_config_dir())
					return 1;

				ProjectInfo project;
				if(project.open(1))
					return 1;

				if(!branches.count(project.rootBranch))
				{
					start_err(std::cout) << "clone: rootBranch " << quote(project.rootBranch) << " is not present in the git repository" << std::endl;
					return 1;
				}
				else if(!branches.count(project.outputBranch))
				{
					start_err(std::cout) << "clone: outputBranch " << quote(project.outputBranch) << " is not present in the git repository" << std::endl;
					return 1;
				}

				if(project.outputBranch != project.rootBranch)
				{
					ret_val = chdir(parDir.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					std::mutex os_mtx;
					Path emptyPath("", "");
					ret_val = cpDir(dirName, ".temp-output-dir", -1, emptyPath, std::cout, 0, &os_mtx);
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to copy directory " << quote(dirName) << " to '.temp-output-dir/' from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					ret_val = chdir(".temp-output-dir");
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(".temp-output-dir") << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					ret_val = delDir(".nift/", -1, emptyPath, std::cout, 0, &os_mtx); //can delete this later
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to delete directory " << get_pwd() << "/.nift/" << std::endl;
						return 1;
					}

					ret_val = system("git checkout -- . > /dev/null 2>&1 >nul 2>&1");
					if(file_exists("./nul"))
						remove_file(Path("./", "nul"));
					if(ret_val)
					{
						start_err(std::cout) << "clone: system('git checkout -- .') failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					checkoutCmnd = "git checkout " + project.outputBranch + " > /dev/null 2>&1 >nul 2>&1";
					ret_val = system(checkoutCmnd.c_str());
					if(file_exists("./nul"))
						remove_file(Path("./", "nul"));
					if(ret_val)
					{
						start_err(std::cout) << "clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					ret_val = chdir((parDir + dirName).c_str());
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					ret_val = delDir(project.outputDir, -1, emptyPath, std::cout, 0, &os_mtx);
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to delete directory " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					ret_val = chdir(parDir.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					//moves project dir/branch in two steps here in case project dir/branch isn't located inside stage/root dir/branch
					rename(".temp-output-dir", (dirName + "/.temp-output-dir").c_str());

					ret_val = chdir(dirName.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					rename(".temp-output-dir", project.outputDir.c_str());

					ret_val = chdir(parDir.c_str());
					if(ret_val)
					{
						start_err(std::cout) << "clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
						return ret_val;
					}

					return 0;
				}
			}
		}
		//switches back to original branch
		checkoutCmnd = "git checkout " + obranch + " > /dev/null 2>&1 >nul 2>&1";
		ret_val = system(checkoutCmnd.c_str());
		if(file_exists("./nul"))
			remove_file(Path("./", "nul"));
		if(ret_val)
		{
			start_err(std::cout) << "clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
			return ret_val;
		}

		return 0;
	}
	else if(cmd == "interp" || cmd == "sh" || cmd == "shh")
	{
		std::cout << "Nift (aka nsm) " << c_gold << "v" << NSM_VERSION << c_white;
		std::cout << " (c)" << DateTimeInfo().currentYYYY();
		std::cout << " (" << c_blue << "https://nift.dev" << c_white << ")" << std::endl;

		//ensures correct number of parameters given
		if(noParams != 1 && noParams != 2)
			return parError(noParams, argv, "1-2");

		std::string param;
		Path path;

		std::set<TrackedInfo> trackedAll;
		std::mutex os_mtx;

		ProjectInfo project, globalInfo;
		if(file_exists(".nift/config.json"))
		{
			if(project.open_local_config(1))
				return 1;

			if(project.open_tracking(1))
				return 1;
		}
		else if(project.open_global_config(1))
				return 1;

		Parser parser(&project.trackedAll,
		              &os_mtx,
		              project.contentDir,
		              project.outputDir,
		              project.contentExt,
		              project.outputExt,
		              project.scriptExt,
		              project.defaultTemplate,
		              project.backupScripts,
		              project.unixTextEditor,
		              project.winTextEditor);

		std::string langStr = "f++";
		if(noParams == 2)
			langStr = argv[2];
		char langCh = 'f';

		if(langStr.find_first_of("xX") != std::string::npos)
			langCh = 'x';
		else if(langStr.find_first_of("nNmM") != std::string::npos)
			langCh = 'n';
		else if(langStr.find_first_of("fF") != std::string::npos)
			langCh = 'f';
		else if(langStr.find_first_of("lL") != std::string::npos)
			langCh = 'l';
		else
		{
			size_t pos = langStr.find_first_of("eEtTfFlL");

			if(pos == std::string::npos)
			{
				start_err(std::cout) << "run: cannot determine chosen language from " << quote(langStr) << ", ";
				std::cout << "valid options include '-n++', '-f++', '-lua', -exprtk'" << std::endl;
				return 1;
			}
			else if(langStr[pos] == 'f' || langStr[pos] == 'F')
				langCh = 'f';
			else if(langStr[pos] == 't' || langStr[pos] == 'T')
				langCh = 'n';
			else if(langStr[pos] == 'e' || langStr[pos] == 'E')
				langCh = 'x';
			else if(langStr[pos] == 'l' || langStr[pos] == 'L')
				langCh = 'l';
		}

		if(project.lolcatDefault)
		{
			parser.lolcatActive = parser.lolcatInit = 1;
			parser.lolcatCmd = project.lolcatCmd;
		}

		if(file_exists(project.execrc_path(argv[0], langCh).str()))
		{
			std::cout << "running " << argv[0] << "rc." << langCh << " file: ";
			std::cout << project.execrc_path(argv[0], langCh) << std::endl;
			std::vector<std::string> run_params;
			parser.run(project.execrc_path(argv[0], langCh).str(), langCh, run_params, std::cout);
		}
		else if(file_exists(project.execrc_path(argv[0], langStr).str()))
		{
			std::cout << "running " << argv[0] << "rc." << langCh << " file: ";
			std::cout << project.execrc_path(argv[0], langStr) << std::endl;
			std::vector<std::string> run_params;
			parser.run(project.execrc_path(argv[0], langStr).str(), langCh, run_params, std::cout);
		}

		if(file_exists(Path(".nift/", cmd + "." + langCh).str()))
		{
			std::cout << "runnning project " << cmd << "." << langCh << " file: ";
			std::cout << Path(".nift/", cmd + "." + langCh) << std::endl;
			std::vector<std::string> run_params;
			parser.run(Path(".nift/", cmd + "." + langCh).str(), langCh, run_params, std::cout);
		}
		else if(file_exists(Path(".nift/", cmd + "." + langStr).str()))
		{
			std::cout << "runnning project " << cmd << "." << langStr << " file: ";
			std::cout << Path(".nift/", cmd + "." + langStr) << std::endl;
			std::vector<std::string> run_params;
			parser.run(Path(".nift/", cmd + "." + langStr).str(), langCh, run_params, std::cout);
		}


		if(cmd == "interp")
			return parser.interpreter(langStr, langCh, std::cout);
		else
			return parser.shell(langStr, langCh, std::cout);
	}
	else if(cmd == "medit" || cmd == "mopen")
	{
		//ensures correct number of parameters given
		if(noParams < 3)
			return parError(noParams, argv, "3");

		ProjectInfo project;
		std::string editor, 
		            c_path,
		            name = argv[noParams],
		            paths_str;

		for(int p=2; p<noParams; ++p)
		{
			if(!dir_exists(argv[p]))
			{
				start_err(std::cout, std::string(argv[p])) << "directory does not exist" << std::endl;
				return 1;
			}

			if(chdir(argv[p]))
			{
				start_err(std::cout) << "failed to change directory from " << get_pwd() << " to " << argv[p] << std::endl;
				return 1;
			}

			if(project.open_local_config(0) || project.open_tracking(0))
				return 1;

			if(p == 2)
			{
				#if defined _WIN32 || defined _WIN64
					editor = project.winTextEditor;
				#else
					editor = project.unixTextEditor;
				#endif
			}

			if(!project.tracking(name))
			{
				start_err(std::cout) << "pagename " << name << " not tracked by project in " << argv[p] << std::endl;
				return 1;
			}
			
			paths_str += " " + std::string(argv[p]) + "/" + project.contentDir + name + project.contentExt;

			project.trackedAll.clear();

			if(chdir("../"))
			{
				start_err(std::cout) << "failed to change to parent directory from " << get_pwd() << std::endl;
				return 1;
			}
		}

		ret_val = std::system((editor + paths_str).c_str());

		if(ret_val)
			start_err(std::cout, std::string(argv[1])) << "system(\"" << editor << paths_str << "\") failed" << std::endl;

		return ret_val;
	}
	else if(cmd == "mbcp")
	{
		//ensures correct number of parameters given
		if(noParams < 3)
			return parError(noParams, argv, "3");

		ProjectInfo project;

		for(int p=2; p<noParams; ++p)
		{
			if(!dir_exists(argv[p]))
			{
				start_err(std::cout, std::string(argv[p])) << "directory does not exist" << std::endl;
				return 1;
			}

			if(chdir(argv[p]))
			{
				start_err(std::cout) << "failed to change directory from " << get_pwd() << " to " << argv[p] << std::endl;
				return 1;
			}

			project.trackedAll.clear();

			if(project.open_local_config(0) || project.open_tracking(0))
				return 1;

			if(chdir("../"))
			{
				start_err(std::cout) << "failed to change to parent directory from " << get_pwd() << std::endl;
				return 1;
			}
		}

		for(int p=2; p<noParams; ++p)
		{
			if(chdir(argv[p]))
			{
				start_err(std::cout) << "failed to change directory from " << get_pwd() << " to " << argv[p] << std::endl;
				return 1;
			}

			ret_val = std::system((std::string(argv[0]) + " bcp " + quote(std::string(argv[noParams]))).c_str());

			if(ret_val)
			{
				start_err(std::cout, std::string(argv[1])) << " system(\"" << argv[0] << " bcp " << quote(std::string(argv[noParams])) << "\") failed" << std::endl; 
				return ret_val;
			}

			if(chdir("../"))
			{
				start_err(std::cout) << "failed to change to parent directory from " << get_pwd() << std::endl;
				return 1;
			}
		}

		return ret_val;
	}
	else if(cmd == "run" || file_exists(cmd))
	{
		//ensures correct number of parameters given
		if(noParams < 1)
			return parError(noParams, argv, "2+");

		std::string langStr;
		std::vector<std::string> run_params;
		Path path;

		char langCh = '?';
		bool foundFile = 0;

		for(int i=1; i<argc; ++i)
		{
			if(std::string(argv[i]).size() && argv[i][0] == '-')
				langStr = argv[i];
			else if(i == 1 && std::string(argv[i]) == "run") {}
			else
			{
				run_params.push_back(argv[i]);

				if(!foundFile && file_exists(argv[i]))
				{
					foundFile = 1;
					path.set_file_path_from(argv[i]);
					if(langStr == "")
					{
						std::string pathStr = argv[i];
						size_t pos = pathStr.find_last_of('.');
						if(pos == std::string::npos)
							langStr = "f++";
						else
						{
							++pos;
							langStr = pathStr.substr(pos, pathStr.size()-pos);
						}
					}
				}
			}
		}

		if(!foundFile)
		{
			start_err(std::cout) << "run: no file specified to run" << std::endl;
			return 1;
		}

		if(langStr.find_first_of("xX") != std::string::npos)
			langCh = 'x';
		else if(langStr.find_first_of("nNmM") != std::string::npos)
			langCh = 'n';
		else if(langStr.find_first_of("fF") != std::string::npos)
			langCh = 'f';
		else if(langStr.find_first_of("lL") != std::string::npos)
			langCh = 'l';
		else
		{
			size_t pos = langStr.find_first_of("eEtTfFlL");

			if(pos == std::string::npos)
			{
				start_err(std::cout) << "run: cannot determine chosen language from " << quote(langStr) << ", ";
				std::cout << "valid options include '-n++', '-f++', '-lua', -exprtk'" << std::endl;
				return 1;
			}
			else if(langStr[pos] == 'f' || langStr[pos] == 'F')
				langCh = 'f';
			else if(langStr[pos] == 't' || langStr[pos] == 'T')
				langCh = 'n';
			else if(langStr[pos] == 'e' || langStr[pos] == 'E')
				langCh = 'x';
			else if(langStr[pos] == 'l' || langStr[pos] == 'L')
				langCh = 'l';
		}

		std::set<TrackedInfo> trackedAll;
		std::mutex os_mtx;
		
		ProjectInfo project;
		if(file_exists(".nift/config.json"))
		{
			if(project.open_local_config(0))
				return 1;

			if(project.open_tracking(0))
				return 1;
		}
		else if(project.open_global_config(0))
				return 1;

		Parser parser(&project.trackedAll,
		              &os_mtx,
		              project.contentDir,
		              project.outputDir,
		              project.contentExt,
		              project.outputExt,
		              project.scriptExt,
		              project.defaultTemplate,
		              project.backupScripts,
		              project.unixTextEditor,
		              project.winTextEditor);

		ret_val = parser.run(path, langCh, run_params, std::cout);

		std::cout.precision(4);
		std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

		return ret_val;
	}
	else
	{
		//checks that we have a valid command
		if(cmd != "bcp" &&
		   cmd != "config" &&
		   cmd != "diff" &&
		   cmd != "edit" &&
		   cmd != "pull" &&
		   cmd != "status" &&
		   cmd != "info" &&
		   cmd != "info-all" &&
		   cmd != "info-names" &&
		   cmd != "info-tracking" &&
		   cmd != "info-watching" &&
		   cmd != "incr-mode" &&
		   cmd != "open" &&
		   cmd != "track" &&
		   cmd != "track-from-file" &&
		   cmd != "track-dir" &&
		   cmd != "untrack" &&
		   cmd != "untrack-from-file" &&
		   cmd != "untrack-dir" &&
		   cmd != "rm-from-file" && cmd != "del-from-file" && cmd != "rmv-from-file" &&
		   cmd != "rm-dir"       && cmd != "del-dir"       && cmd != "rmv-dir" &&
		   cmd != "rm"           && cmd != "del"           && cmd != "rmv" &&
		   cmd != "mv" && cmd != "move" && cmd != "mve" &&
		   cmd != "cp" && cmd != "copy" && cmd != "cpy" &&
		   cmd != "mve-output-dir" &&
		   cmd != "mve-cont-dir" &&
		   cmd != "new-title" &&
		   cmd != "new-template" &&
		   cmd != "new-cont-ext" &&
		   cmd != "new-output-ext" &&
		   cmd != "new-script-ext" &&
		   cmd != "no-build-thrds" &&
		   cmd != "backup-scripts" &&
		   cmd != "watch" &&
		   cmd != "unwatch" &&
		   cmd != "build-names" &&
		   cmd != "build" &&
		   cmd != "build-updated" &&
		   cmd != "build-all" &&
		   cmd != "build-auto" &&
		   cmd != "browse")
		{
			#if defined _WIN32 || defined _WIN64 //what is this for?
				if(ProjectInfo().open_global_config(1))
					return 1;
			#endif

			unrecognisedCommand(cmd);
			return 1;
		}

		//ensures Nift is managing a project from this directory or one of the parent directories
		std::string parentDir = "../",
		    rootDir = "/",
		    projectRootDir = get_pwd(),
		    owd = get_pwd(),
		    pwd = get_pwd(),
		    prevPwd;

		while(!file_exists(".nift/config.json") &&
		      !file_exists(".nsm/nift.config"))
		{
			//sets old pwd
			prevPwd = pwd;

			//changes to parent directory
			ret_val = chdir(parentDir.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "failed to change directory to " << quote(parentDir) << " from " << quote(get_pwd()) << " when searching for project config" << std::endl;
				return ret_val;
			}

			//gets new pwd
			pwd = get_pwd();

			//checks we haven't made it to root directory or stuck at same dir
			if(pwd == rootDir || pwd == prevPwd)
			{
				start_err(std::cout) << "Nift is not managing a project from " << owd << " (or any accessible parent directories)" << std::endl;
				return 1;
			}
		}

		if(file_exists(".nsm/nift.config") && upgrade_config_dir())
			return 1;

		//ensures both tracked.json and config.json exist
		if(!file_exists(".nift/tracked.json"))
		{
			start_err(std::cout) << quote(get_pwd() + "/.nift/tracked.json") << " is missing" << std::endl;
			return 1;
		}

		if(!file_exists(".nift/config.json"))
		{
			start_err(std::cout) << quote(get_pwd() + "/.nift/config.json") << " is missing" << std::endl;
			return 1;
		}

		//Nift commands that just need to know it is a project directory
		if(cmd == "diff") //diff of file
		{
			//ensures correct number of parameters given
			if(noParams != 2)
			{
				start_err(std::cout) << "correct usage 'diff file-path'" << std::endl;
				return parError(noParams, argv, "2");
			}

			if(!dir_exists(".git"))
			{
				start_err(std::cout) << "project directory not a git repository" << std::endl;
				return 1;
			}

			if(!path_exists(argv[2]))
			{
				start_err(std::cout) << "path " << quote(argv[2]) << " not in working tree" << std::endl;
				return 1;
			}
			else
			{
				ret_val = system(("git diff " + std::string(argv[2])).c_str());
				if(ret_val)
				{
					start_err(std::cout) << "diff: system('git diff " << std::string(argv[2]) << "') failed in " << quote(get_pwd()) << std::endl;
					return ret_val;
				}
			}

			return 0;
		}

		ProjectInfo project;
		if(project.open_local_config(1))
			return 1;

		if(cmd == "build")
		{
			if(noParams == 1 || (noParams == 2 && argv[2][0] == '-'))
				cmd = "build-updated";
			else
				cmd = "build-names";
		}

		//Nift commands that need project information file open
		if(cmd == "config")
		{
			if(noParams != 2)
			{
				return parError(noParams, argv, "2");
				return 1;
			}
			else if(std::string(argv[2]) != "project")
			{
				start_err(std::cout) << "do not have a config file for " << c_blue << argv[2] << c_white << std::endl;
				return 1;
			}

			std::string editor;
			#if defined _WIN32 || defined _WIN64
				editor = project.winTextEditor;
			#else
				editor = project.unixTextEditor;
			#endif

			ret_val = std::system((editor + " .nift/config.json").c_str());

			if(ret_val)
				start_err(std::cout) << "config: system(\"" << editor << " .nift/config.json\") failed" << std::endl;

			return ret_val;
		}
		else if(cmd =="pull")
		{
			//ensures correct number of parameters given
			if(noParams != 1)
				return parError(noParams, argv, "1");

			//checks that git is configured
			if(!is_git_configured())
				return 1;

			//checks that remote git url is set
			if(!is_git_remote_set())
				return 1;

			std::string pullCmnd,
			            projectDirRemote,
			            projectRootDirRemote,
			            projectDirBranch,
			            projectRootDirBranch;

			if(get_remote(projectRootDirRemote))
			{
				start_err(std::cout) << "pull: get_remote() failed in project root directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectRootDirRemote == "")
			{
				start_err(std::cout) << "pull: get_remote() did not find any git remote in project root directory " << quote(get_pwd()) << std::endl;
			}

			if(get_pb(projectRootDirBranch))
			{
				start_err(std::cout) << "pull: get_pb() failed in project root directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectRootDirBranch == "")
			{
				start_err(std::cout) << "pull: no branch found in project root directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectRootDirBranch != project.rootBranch)
			{
				start_err(std::cout) << "pull: project root dir branch " << quote(projectRootDirBranch) << " is not the same as rootBranch " << quote(project.rootBranch) << " in .nift/config.json" << std::endl;
				return 1;
			}

			ret_val = chdir(project.outputDir.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "pull: failed to change directory to " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			if(get_remote(projectDirRemote))
			{
				start_err(std::cout) << "pull: get_remote() failed in project directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(get_pb(projectDirBranch))
			{
				start_err(std::cout) << "pull: get_pb() failed in project directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectDirBranch == "")
			{
				start_err(std::cout) << "pull: no branch found in project directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectDirBranch != project.outputBranch)
			{
				start_err(std::cout) << "pull: output dir branch " << quote(projectDirBranch) << " is not the same as outputBranch " << quote(project.outputBranch) << " in .nift/config.json" << std::endl;
				return 1;
			}

			if(projectDirBranch != projectRootDirBranch)
			{
				pullCmnd = "git pull " + projectDirRemote + " " + projectDirBranch;

				ret_val = system(pullCmnd.c_str());
				if(ret_val)
				{
					start_err(std::cout) << "pull: system(" << quote(pullCmnd) << ") failed in project directory " << quote(get_pwd()) << std::endl;
					return ret_val;
				}
			}

			ret_val = chdir(projectRootDir.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "pull: failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			pullCmnd = "git pull " + projectRootDirRemote + " " + projectRootDirBranch;

			ret_val = system(pullCmnd.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "pull: system(" << quote(pullCmnd) << ") failed in project root directory " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			return 0;
		}
		else if(cmd == "no-build-thrds")
		{
			//ensures correct number of parameters given
			if(noParams != 1 && noParams != 2)
				return parError(noParams, argv, "1 or 2");

			if(noParams == 1)
			{
				if(project.buildThreads < 0)
					std::cout << project.buildThreads << " (" << -project.buildThreads*std::thread::hardware_concurrency() << " on this machine)" << std::endl;
				else
					std::cout << project.buildThreads << std::endl;
			}
			else
			{
				if(!isInt(std::string(argv[2])))
				{
					start_err(std::cout) << "number of build threads should be a non-zero integer (use negative numbers for a multiple of the number of cores on the machine)" << std::endl;
					return 1;
				}
				return project.no_build_threads(std::stoi(std::string(argv[2])));
			}

			return 0;
		}
		else if(cmd == "backup-scripts")
		{
			//ensures correct number of parameters given
			if(noParams != 1 && noParams != 2)
				return parError(noParams, argv, "1 or 2");

			if(noParams == 1)
				std::cout << project.backupScripts << std::endl;
			else
			{
				if(!std::string(argv[2]).size())
				{
					start_err(std::cout) << "do not recognise the option " << quote(argv[2]) << std::endl;
					return 1;
				}
				if(std::string(argv[2])[0] == 'y' || std::string(argv[2]) == "1")
				{
					if(project.backupScripts)
					{
						start_err(std::cout) << "project is already set to backup scripts" << std::endl;
						return 1;
					}
					project.backupScripts = 1;
					std::cout << "scripts will now be backed up when moved to run" << std::endl;
				}
				else if(std::string(argv[2])[0] == 'n' || std::string(argv[2]) == "0")
				{
					if(!project.backupScripts)
					{
						start_err(std::cout) << "project is already set to not backup scripts" << std::endl;
						return 1;
					}
					project.backupScripts = 0;
					std::cout << "scripts will no longer be backed up when moved to run" << std::endl;
				}
				else
				{
					start_err(std::cout) << "do not recognise the option " << argv[2] << std::endl;
					return 1;
				}

				project.save_local_config();

				return 0;
			}

			return 0;
		}
		else if(cmd == "incr-mode")
		{
			if(noParams > 2)
				return parError(noParams, argv, "1-2");

			if(noParams == 1)
			{
				if(project.incrMode == INCR_MOD)
					std::cout << "mod" << std::endl;
				else if(project.incrMode == INCR_HASH)
					std::cout << "hash" << std::endl;
				else if(project.incrMode == INCR_HYB)
					std::cout << "hybrid" << std::endl;
				else
				{
					start_err(std::cout) << "incr-mode: do not recognise incremental mode " << project.incrMode << std::endl;
					return 1;
				}

				return 0;
			}
			else
				return project.set_incr_mode(argv[2]);
		}
		else if(cmd == "watch")
		{
			//ensures correct number of parameters given
			if(noParams < 2 && noParams > 5)
				return parError(noParams, argv, "2-5");

			WatchList wl;
			if(wl.open())
			{
				start_err(std::cout) << "watch: failed to open watch list '.nift/.watchinfo/watching.list'" << std::endl;
				return 1;
			}

			WatchDir wd;
			wd.watchDir = comparable(argv[2]);
			if(!dir_exists(std::string(argv[2])))
			{
				start_err(std::cout) << "cannot watch directory " << quote(wd.watchDir) << " as it does not exist" << std::endl;
				return 1;
			}
			else if(wd.watchDir.substr(0, project.contentDir.size()) != project.contentDir)
			{
				start_err(std::cout) << "cannot watch directory " << quote(wd.watchDir) << " as it is not a subdirectory of the project-wide content directory " << quote(project.contentDir) << std::endl;
				return 1;
			}
			std::string contExt, outputExt;
			Path templatePath;
			if(wd.watchDir[wd.watchDir.size()-1] != '/' && wd.watchDir[wd.watchDir.size()-1] != '\\')
			{
				start_err(std::cout) << "watch directory " << quote(wd.watchDir) << " must end with '/' or '\\'" << std::endl;
				return 1;
			}
			if(noParams >= 3)
			{
				contExt = argv[3];
				if(contExt[0] != '.')
				{
					start_err(std::cout) << "content extension " << quote(contExt) << " must start with a '.'" << std::endl;
					return 1;
				}
			}
			else
				contExt = project.contentExt;
			if(noParams >= 4)
			{
				templatePath.set_file_path_from(argv[4]);

				if(!file_exists(templatePath.str()))
				{
					start_err(std::cout) << "template path " << templatePath << " does not exist" << std::endl;
					return 1;
				}
			}
			else
				templatePath = project.defaultTemplate;
			if(noParams == 5)
			{
				outputExt = argv[5];
				if(outputExt[0] != '.')
				{
					start_err(std::cout) << "output extension " << quote(outputExt) << " must start with a '.'" << std::endl;
					return 1;
				}
			}
			else
				outputExt = project.outputExt;

			auto found = wl.dirs.find(wd);

			if(found != wl.dirs.end())
			{
				if(found->contExts.count(contExt))
				{
					start_err(std::cout) << "already watching directory " << quote(wd.watchDir) << " with content extension " << quote(contExt) << std::endl;
					return 1;
				}
				else
				{
					wd.contExts = found->contExts;
					wd.templatePaths = found->templatePaths;
					wd.outputExts = found->outputExts;

					wd.contExts.insert(contExt);
					wd.templatePaths[contExt] = templatePath;
					wd.outputExts[contExt] = outputExt;

					wl.dirs.erase(found);

					wl.dirs.insert(wd);
					wl.save();

					std::cout << "successfully watching directory " << quote(wd.watchDir) << " with:" << std::endl;
					std::cout << "  content extension: " << quote(contExt) << std::endl;
					std::cout << "      template path: " << templatePath << std::endl;
					std::cout << "   output extension: " << quote(outputExt) << std::endl;
				}
			}
			else
			{
				wd.contExts.insert(contExt);
				wd.templatePaths[contExt] = templatePath;
				wd.outputExts[contExt] = outputExt;
				wl.dirs.insert(wd);
				wl.save();

				std::cout << "successfully watching directory " << quote(wd.watchDir) << " with:" << std::endl;
				std::cout << "  content extension: " << quote(contExt) << std::endl;
				std::cout << "      template path: " << templatePath << std::endl;
				std::cout << "   output extension: " << quote(outputExt) << std::endl;
			}

			return 0;
		}
		else if(cmd == "unwatch")
		{
			//ensures correct number of parameters given
			if(noParams < 2 || noParams > 3)
				return parError(noParams, argv, "2 or 3");

			if(file_exists(".nift/.watch/watched.json"))
			{
				std::string contExt;
				WatchList wl;
				if(wl.open())
				{
					start_err(std::cout) << "unwatch: failed to open watch list '.nift/.watch/watched.json'" << std::endl;
					return 1;
				}

				WatchDir wd;
				wd.watchDir = comparable(argv[2]);

				if(noParams == 3)
					contExt = argv[3];
				else
					contExt = project.contentExt;

				auto found = wl.dirs.find(wd);

				if(found != wl.dirs.end())
				{
					if(found->contExts.count(contExt))
					{
						if(found->contExts.size() == 1)
						{
							wl.dirs.erase(found);
							remove_path(Path(".nift/.watch/" + wd.watchDir + "exts.json"));
							remove_path(Path(".nift/.watch/" + wd.watchDir + "tracked.json"));
						}
						else
						{
							wd.contExts = found->contExts;
							wd.templatePaths = found->templatePaths;
							wd.outputExts = found->outputExts;

							wd.contExts.erase(contExt);

							wl.dirs.erase(found);
							wl.dirs.insert(wd);
						}

						wl.save();
						std::cout << "successfully unwatched directory " << quote(argv[2]) << " with content extension " << quote(contExt) << std::endl;
					}
					else
					{
						start_err(std::cout) << "not watching directory " << quote(argv[2]) << " with content extension " << quote(contExt) << std::endl;
						return 1;
					}
				}
				else
				{
					start_err(std::cout) << "not watching directory " << quote(argv[2]) << std::endl;
					return 1;
				}
			}
			else
			{
				start_err(std::cout) << "not watching directory " << quote(argv[2]) << std::endl;
				return 1;
			}

			return 0;
		}

		//opens up tracking.list file
		if(project.open_tracking(1))
			return 1;

		//Nift commands that need tracking list file open
		if(cmd == "bcp") //build-updated commit push
		{
			//ensures correct number of parameters given
			if(noParams != 2)
			{
				start_err(std::cout) << "correct usage 'bcp \"commit message\"'" << std::endl;
				return parError(noParams, argv, "2");
			}

			if(!is_git_configured())
				return 1;

			if(!is_git_remote_set())
				return 1;

			if(project.build_updated(std::cout, 1, 0, 1))
				return 1;

			std::string commitCmnd = "git commit -m \"" + std::string(argv[2]) + "\"",
			            pushCmnd,
			            outputDirBranch,
			            projectRootDirBranch;

			if(get_pb(projectRootDirBranch))
			{
				start_err(std::cout) << "bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
				return 0;
			}

			if(projectRootDirBranch == "")
			{
				start_err(std::cout) << "bcp: no branch found in project root directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(projectRootDirBranch != project.rootBranch)
			{
				start_err(std::cout) << "bcp: root dir branch " << quote(projectRootDirBranch) << " is not the same as rootBranch " << quote(project.rootBranch) << " in .nift/config.json" << std::endl;
				return 1;
			}

			std::cout << commitCmnd << std::endl;

			ret_val = chdir(project.outputDir.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "bcp: failed to change directory to " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			if(get_pb(outputDirBranch))
			{
				start_err(std::cout) << "bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
				return 0;
			}

			if(outputDirBranch == "")
			{
				start_err(std::cout) << "bcp: no branch found in project directory " << quote(get_pwd()) << std::endl;
				return 1;
			}

			if(outputDirBranch != project.outputBranch)
			{
				start_err(std::cout) << "bcp: output dir branch " << quote(outputDirBranch) << " is not the same as outputBranch " << quote(project.outputBranch) << " in .nift/config.json" << std::endl;
				return 1;
			}

			if(outputDirBranch != projectRootDirBranch)
			{
				pushCmnd = "git push origin " + outputDirBranch;

				ret_val = system("git add -A .");
				if(ret_val)
				{
					start_err(std::cout) << "bcp: system('git add -A .') failed in " << quote(get_pwd()) << std::endl;
					return ret_val;
				}

				ret_val = system(commitCmnd.c_str());
				//can't handle error here incase we had to run bcp multiple times

				ret_val = system(pushCmnd.c_str());
				if(ret_val)
				{
					start_err(std::cout) << "bcp: system(" << quote(pushCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
					return ret_val;
				}
			}

			ret_val = chdir(projectRootDir.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "bcp: failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			pushCmnd = "git push origin " + projectRootDirBranch;

			ret_val = system("git add -A .");
			if(ret_val)
			{
				start_err(std::cout) << "bcp: system('git add -A .') failed in " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			//clear_console_line();
			ret_val = system(commitCmnd.c_str());
			//can't handle error here incase we had to run bcp multiple times

			//clear_console_line();
			ret_val = system(pushCmnd.c_str());
			if(ret_val)
			{
				start_err(std::cout) << "bcp: system(" << quote(pushCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
				return ret_val;
			}

			return 0;
		}
		if(cmd == "edit" || cmd == "open")
		{
			//ensures correct number of parameters given
			if(noParams < 2)
				return parError(noParams, argv, ">1");

			std::string editor, paths_str;
			#if defined _WIN32 || defined _WIN64
				editor = project.winTextEditor;
			#else
				editor = project.unixTextEditor;
			#endif

			for(int p=2; p<argc; ++p)
			{
				if(!project.tracking(argv[p]))
				{
					start_err(std::cout, std::string(argv[p])) << "is not tracked by the current project" << std::endl;
					return 1;
				}
				else
					paths_str += " " + project.contentDir + argv[p] + project.contentExt;
			}

			ret_val = std::system((editor + paths_str).c_str());

			if(ret_val)
				start_err(std::cout, std::string(argv[1])) << "system(\"" << editor << paths_str << "\") failed" << std::endl;

			return ret_val;
		}
		else if(cmd == "info")
		{
			//ensures correct number of parameters given
			if(noParams <= 1)
				return parError(noParams, argv, ">1");

			std::vector<Name> names;
			for(int p=2; p<argc; ++p)
				names.push_back(argv[p]);
			return project.info(names);
		}
		else if(cmd == "info-all")
		{
			//ensures correct number of parameters given
			if(noParams > 1)
				return parError(noParams, argv, "1");

			return project.info_all();
		}
		else if(cmd == "info-names")
		{
			//ensures correct number of parameters given
			if(noParams > 1)
				return parError(noParams, argv, "1");

			return project.info_names();
		}
		else if(cmd == "info-tracking")
		{
			//ensures correct number of parameters given
			if(noParams > 1)
				return parError(noParams, argv, "1");

			return project.info_tracking();
		}
		else if(cmd == "info-watching")
		{
			//ensures correct number of parameters given
			if(noParams > 1)
				return parError(noParams, argv, "1");

			return project.info_watching();
		}
		else if(cmd == "track")
		{
			//ensures correct number of parameters given
			if(noParams < 2 || noParams > 4)
				return parError(noParams, argv, "2-4");

			Name newName = argv[2]; 
			Title newTitle;
			if(noParams >= 3)
				newTitle = argv[3]; 
			else
				newTitle = get_title(argv[2]);

			Path newTemplatePath;
			if(noParams == 4)
				newTemplatePath.set_file_path_from(argv[4]);
			else
				newTemplatePath = project.defaultTemplate;

			return project.track(newName, newTitle, newTemplatePath);
		}
		else if(cmd == "track-from-file")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			int result = project.track_from_file(argv[2]);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "track-dir")
		{
			//ensures correct number of parameters given
			if(noParams < 2 || noParams > 5)
				return parError(noParams, argv, "2-5");

			Path dirPath,
			     templatePath = project.defaultTemplate;
			std::string contExt = project.contentExt,
			            outputExt = project.outputExt;

			dirPath.set_file_path_from(argv[2]);

			if(noParams >= 3)
				contExt = argv[3];
			if(noParams >= 4)
				templatePath.set_file_path_from(argv[4]);
			if(noParams == 5)
				outputExt = argv[5];

			int result = project.track_dir(dirPath, contExt, templatePath, outputExt);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "untrack")
		{
			//ensures correct number of parameters given
			if(noParams < 2)
				return parError(noParams, argv, "2+");

			std::vector<Name> namesToUntrack;
			for(int i=2; i<=noParams; ++i)
				namesToUntrack.push_back(argv[2]);

			return project.untrack(namesToUntrack);
		}
		else if(cmd == "untrack-from-file")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			int result = project.untrack_from_file(argv[2]);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "untrack-dir")
		{
			//ensures correct number of parameters given
			if(noParams < 2 || noParams > 3)
				return parError(noParams, argv, "2-3");

			Path dirPath;
			std::string contExt = project.contentExt;

			dirPath.set_file_path_from(argv[2]);

			if(noParams == 3)
				contExt = argv[3];

			int result = project.untrack_dir(dirPath, contExt);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "rmv-from-file" || cmd == "rm-from-file" || cmd == "del-from-file")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			int result = project.rm_from_file(argv[2]);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "rmv-dir" || cmd == "rm-dir" || cmd == "del-dir")
		{
			//ensures correct number of parameters given
			if(noParams < 2 || noParams > 3)
				return parError(noParams, argv, "2-3");

			Path dirPath;
			std::string contExt = project.contentExt;

			dirPath.set_file_path_from(argv[2]);

			if(noParams == 3)
				contExt = argv[3];

			int result = project.rm_dir(dirPath, contExt);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "rmv" || cmd == "rm" || cmd == "del")
		{
			//ensures correct number of parameters given
			if(noParams < 2)
				return parError(noParams, argv, "2+");

			std::vector<Name> namesToRemove;
			for(int i=2; i<=noParams; ++i)
				namesToRemove.push_back(argv[2]);

			return project.rm(namesToRemove);
		}
		else if(cmd == "mve" || cmd == "mv" || cmd == "move")
		{
			//ensures correct number of parameters given
			if(noParams != 3)
				return parError(noParams, argv, "3");

			Name oldName = argv[2],
			     newName = argv[3];

			return project.mv(oldName, newName);
		}
		else if(cmd == "cpy" || cmd == "cp" || cmd == "copy")
		{
			//ensures correct number of parameters given
			if(noParams != 3)
				return parError(noParams, argv, "3");

			Name trackedName = argv[2],
			     newName = argv[3];

			return project.cp(trackedName, newName);
		}
		else if(cmd == "new-title")
		{
			//ensures correct number of parameters given
			if(noParams != 3)
				return parError(noParams, argv, "3");

			Name name = argv[2];
			Title newTitle;
			newTitle.str = argv[3];

			return project.new_title(name, newTitle);
		}
		else if(cmd == "new-template")
		{
			//ensures correct number of parameters given
			if(noParams != 2 && noParams != 3)
				return parError(noParams, argv, "2 or 3");

			if(noParams == 2)
			{
				Path newTemplatePath;
				newTemplatePath.set_file_path_from(argv[2]);

				return project.new_template(newTemplatePath);
			}
			else
			{
				Name name = argv[2];
				Path newTemplatePath;
				newTemplatePath.set_file_path_from(argv[3]);

				int return_val =  project.new_template(name, newTemplatePath);

				if(!return_val)
					std::cout << "successfully changed template path to " << newTemplatePath << std::endl;

				return return_val;
			}
		}
		else if(cmd == "new-output-dir")
		{
			start_err(std::cout) << "command has changed to mve-output-dir" << std::endl;
			return 1;
		}
		else if(cmd == "mve-output-dir" || cmd == "mv-output-dir" || cmd == "move-output-dir")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			Directory newOutputDir = argv[2];

			if(newOutputDir.size() && newOutputDir[newOutputDir.size()-1] != '/' && newOutputDir[newOutputDir.size()-1] != '\\')
				newOutputDir += "/";

			return project.new_output_dir(newOutputDir);
		}
		else if(cmd == "new-cont-dir")
		{
			start_err(std::cout) << "command has changed to mve-cont-dir" << std::endl;
			return 1;
		}
		else if(cmd == "mve-cont-dir" || cmd == "mv-cont-dir" || cmd == "move-cont-dir")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			Directory newContDir = argv[2];

			if(newContDir.size() && newContDir[newContDir.size()-1] != '/' && newContDir[newContDir.size()-1] != '\\')
				newContDir += "/";

			return project.new_content_dir(newContDir);
		}
		else if(cmd == "new-cont-ext")
		{
			//ensures correct number of parameters given
			if(noParams != 2 && noParams != 3)
				return parError(noParams, argv, "2 or 3");

			if(noParams == 2)
				return project.new_content_ext(argv[2]);
			else
			{
				Name name = argv[2];

				int return_val = project.new_content_ext(name, argv[3]);

				if(!return_val) //informs user that content extension was successfully changed
					std::cout << "successfully changed content extention for " << name << " to " << argv[3] << std::endl;

				return return_val;
			}
		}
		else if(cmd == "new-output-ext")
		{
			//ensures correct number of parameters given
			if(noParams != 2 && noParams != 3)
				return parError(noParams, argv, "2 or 3");

			if(noParams == 2)
				return project.new_output_ext(argv[2]);
			else
			{
				Name name = argv[2];

				int return_val = project.new_output_ext(name, argv[3]);

				if(!return_val) //informs user that output extension was successfully changed
					std::cout << "successfully changed output extention for " << name << " to " << argv[3] << std::endl;

				return return_val;
			}
		}
		else if(cmd == "new-script-ext")
		{
			//ensures correct number of parameters given
			if(noParams != 2 && noParams != 3)
				return parError(noParams, argv, "2 or 3");

			if(noParams == 2)
				return project.new_script_ext(argv[2]);
			else
			{
				Name name = argv[2];

				int return_val = project.new_script_ext(name, argv[3]);

				if(!return_val) //informs user that script extension was successfully changed
					std::cout << "successfully changed script extention for " << quote(name) << " to " << quote(argv[3]) << std::endl;

				return return_val;
			}
		}
		else if(cmd == "status")
		{
			//ensures correct number of parameters given
			if(noParams > 2)
				return parError(noParams, argv, "1-2");

			int result;

			if(noParams == 1)
				result = project.status(std::cout, 0, 0, 0);
			else
			{
				std::string optStr = argv[2];

				if(optStr == "-a") //add expl
					result = project.status(std::cout, 0, 1, 0);
				else if(optStr == "-ab")
					result = project.status(std::cout, 0, 1, 1);
				else if(optStr == "-ap")
					result = project.status(std::cout, 1, 1, 0);
				else if(optStr == "-abp")
					result = project.status(std::cout, 1, 1, 1);
				else if(optStr == "-b") //basic
					result = project.status(std::cout, 0, 0, 1);
				else if(optStr == "-bp")
					result = project.status(std::cout, 1, 0, 1);
				else if(optStr == "-n")
					result = project.status(std::cout, 2, 0, 0);
				else if(optStr == "-p") //progress
					result = project.status(std::cout, 1, 0, 0);
				else
				{
					start_err(std::cout) << "do not recognise status option " << quote(argv[2]) << std::endl;
					return 1;
				}
			}

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "build-updated")
		{
			//ensures correct number of parameters given
			if(noParams > 2)
				return parError(noParams, argv, "1-2");

			int result;

			if(noParams == 1)
				result = project.build_updated(std::cout, 0, 0, 0);
			else
			{
				std::string optStr = argv[2];

				if(optStr == "-a") //add expl
					result = project.build_updated(std::cout, 0, 1, 0);
				else if(optStr == "-ab")
					result = project.build_updated(std::cout, 0, 1, 1);
				else if(optStr == "-ap")
					result = project.build_updated(std::cout, 1, 1, 0);
				else if(optStr == "-abp")
					result = project.build_updated(std::cout, 1, 1, 1);
				else if(optStr == "-b") //basic
					result = project.build_updated(std::cout, 0, 0, 1);
				else if(optStr == "-bp")
					result = project.build_updated(std::cout, 1, 0, 1);
				else if(optStr == "-n") //newines for progress
					result = project.build_updated(std::cout, 2, 0, 0);
				else if(optStr == "-p") //progress
					result = project.build_updated(std::cout, 1, 0, 0);
				else
				{
					start_err(std::cout) << "do not recognise build-updated option " << quote(argv[2]) << std::endl;
					return 1;
				}
			}

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "build-names")
		{
			//ensures correct number of parameters given
			if(noParams <= 1)
				return parError(noParams, argv, ">1");

			int p=2;
			bool noOpt = 1;
			std::string optStr = argv[2];
			if(optStr == "-s" || optStr == "-n")
			{
				noOpt = 0;
				++p;
			}

			std::vector<Name> namesToBuild;
			for(; p<argc; p++)
				namesToBuild.push_back(argv[p]);

			int result;

			if(noOpt)
				result = project.build_names(std::cout, 0, namesToBuild);
			else if(optStr == "-p")
				result = project.build_names(std::cout, 1, namesToBuild);
			else
				result = project.build_names(std::cout, 2, namesToBuild);

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "build-all")
		{
			//ensures correct number of parameters given
			if(noParams > 2)
				return parError(noParams, argv, "1-2");

			int result;

			if(noParams == 1)
				result = project.build_all(std::cout, 0);
			else
			{
				std::string optStr = argv[2];

				if(optStr == "-p")
					result = project.build_all(std::cout, 1);
				else if(optStr == "-n")
					result = project.build_all(std::cout, 2);
				else
				{
					start_err(std::cout) << "do not recognise build-all option " << quote(argv[2]) << std::endl;
					return 1;
				}
			}

			std::cout.precision(4);
			std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

			return result;
		}
		else if(cmd == "build-auto")
		{
			//ensures correct number of parameters given
			if(noParams > 2)
				return parError(noParams, argv, "1 or 2");

			if(noParams == 2 && isDouble(std::string(argv[2])))
			{
				double sleepTimeSec = std::strtod(argv[2], NULL);
				sleepTime = sleepTimeSec*1000000;
				noParams = 1;
				if(sleepTimeSec >= 60)
					std::cout << "sleep time: " << int_to_timestr(sleepTimeSec) << std::endl;
				else //need to consider 0 < sleepTimeSec < 1
					std::cout << "sleep time: " << sleepTimeSec << "s" << std::endl;
			}

			if(noParams == 2 && std::string(argv[2]) != "-s")
			{
				start_err(std::cout) << "do not recognise command build-auto " << argv[2] << std::endl;
				return 1;
			}

			Parser parser(&project.trackedAll, 
			      &auto_build_mtx, 
			      project.contentDir, 
			      project.outputDir, 
			      project.contentExt, 
			      project.outputExt, 
			      project.scriptExt, 
			      project.defaultTemplate, 
			      project.backupScripts, 
			      project.unixTextEditor, 
			      project.winTextEditor);

			//checks for pre-build-auto scripts
			if(parser.run_script(std::cout, Path("", "pre-build-auto" + project.scriptExt), project.backupScripts, 1))
				return 1;

			auto_build = 1;

			std::thread build_auto_thread(build_auto);
			if(noParams == 1)
			{
				std::thread read_build_auto_commands_thread(read_build_auto_commands);
				read_build_auto_commands_thread.join();
			}
			build_auto_thread.join();

			//checks for post-build-auto scripts
			if(parser.run_script(std::cout, Path("", "post-build-auto" + project.scriptExt), project.backupScripts, 1))
				return 1;
		}
		else if(cmd == "browse")
		{
			//ensures correct number of parameters given
			if(noParams != 2)
				return parError(noParams, argv, "2");

			if(project.browser == "" && project.open_global_config(1))
				return 1;

			std::string page_path = argv[2];

			if(!file_exists(std::string(page_path)))
				page_path = project.get_info(std::string(argv[2])).outputPath.str();

			if(std::system((project.browser + " " + page_path).c_str()))
			{
				start_err(std::cout) << "failed to browse " << argv[2] << " using broswer " << c_blue << project.browser << c_white << std::endl;
				return 1;
			}
		}
		else
		{
			unrecognisedCommand(cmd);
			return 1;
		}
	}

	return 0;
}
