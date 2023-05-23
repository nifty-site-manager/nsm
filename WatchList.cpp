#include "WatchList.h"

std::string strip_trailing_slash(const std::string& source)
{
	std::string result = "";

	if(source.size() && (source[source.size()-1] == '/' || source[source.size()-1] == '\\'))
		result = source.substr(0, source.size()-1);
	else
		result = source;

	return result;
}

WatchDir::WatchDir()
{
}

bool operator<(const WatchDir& wd1, const WatchDir& wd2)
{
	return (wd1.watchDir < wd2.watchDir);
}

WatchList::WatchList()
{
}

int WatchList::open()
{
	std::cout << "loading project watch file: " << Path(".nift/.watch/", "watched.json") << std::endl;

	if(file_exists(".nift/config.json"))
	{
		if(file_exists(".nift/.watch/watched.json"))
		{
			WatchDir wd;
			std::string contExt;
			Path templatePath;

			rapidjson::Document watched_doc;
			rapidjson::Value arr(rapidjson::kArrayType), exts_arr(rapidjson::kArrayType);

			watched_doc.Parse(string_from_file(".nift/.watch/watched.json").c_str());
			arr = watched_doc["watched"].GetArray();

			//while(read_quoted(ifs, wd.watchDir))
			for(auto it = arr.Begin(); it != arr.End(); ++it) 
			{
				wd.watchDir = it->GetString();
				wd.contExts.clear();
				wd.templatePaths.clear();
				wd.outputExts.clear();
				if(wd.watchDir != "" && wd.watchDir[0] != '#')
				{
					std::string watchDirExtsFileStr = ".nift/.watch/" + wd.watchDir + "exts.json";

					if(file_exists(watchDirExtsFileStr))
					{
						rapidjson::Document exts_doc;
						exts_doc.Parse(string_from_file(watchDirExtsFileStr).c_str());
						exts_arr = exts_doc["exts"].GetArray();

						for(auto ext=exts_arr.Begin(); ext!=exts_arr.End(); ++ext)
						{
							contExt = (*ext)["content-ext"].GetString();

							wd.contExts.insert(contExt);
							wd.templatePaths[contExt] = Path((*ext)["template"].GetString());
							wd.outputExts[contExt] = (*ext)["output-ext"].GetString();
						}
					}
					else
					{
						std::cout << "error: watched content extensions file " << quote(watchDirExtsFileStr) << " does not exist for watched directory" << std::endl;
						return 1;
					}

					dirs.insert(wd);
				}
			}
		}
		else
		{
			//watch list isn't present, so technically already open
			/*std::cout << "error: cannot open watch list as '.nift/.watch/watched.json' file does not exist" << std::endl;
			return 1;*/
		}
	}
	else
	{
		//no Nift configuration file found
		std::cout << "error: cannot open watch list as Nift configuration file '.nift/config.json' does not exist" << std::endl;
		return 1;
	}

	return 0;
}

int WatchList::save()
{
	if(file_exists(".nift/config.json"))
	{
		if(dirs.size())
		{
			Path(".nift/.watch/", "").ensureDirExists();
			std::ofstream ofs(".nift/.watch/watched.json");

			ofs << "{\n";
			ofs << "\t\"watched\": [\n";

			for(auto wd=dirs.begin(); wd!=dirs.end(); wd++)
			{
				if(wd != dirs.begin())
					ofs << ",\n";

				ofs << "\t\t" << double_quote(wd->watchDir);

				std::string watchDirExtsFileStr = ".nift/.watch/" + wd->watchDir + "exts.json";

				Path watchDirExtsFilePath;
				watchDirExtsFilePath.set_file_path_from(watchDirExtsFileStr);
				watchDirExtsFilePath.ensureFileExists();
				
				std::ofstream ofxs(watchDirExtsFileStr);
				ofxs << "{\n";
				ofxs << "\t\"exts\": [\n";
				for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
				{
					if(ext != wd->contExts.begin())
						ofxs << ",\n";

					ofxs << "\t\t{";
					ofxs << "\t\t\t\"content-ext\": " << double_quote(*ext)         << ",\n";
					ofxs << "\t\t\t\"template\": "    << double_quote(wd->templatePaths.at(*ext).str()) << ",\n";
					ofxs << "\t\t\t\"output-ext\": "  << double_quote(wd->outputExts.at(*ext))    << "\n";
					ofxs << "\t\t}";
				}
				ofxs << "\n\t]\n";
				ofxs << "}";
				ofxs.close();
			}

			ofs << "\n\t]\n";
			ofs << "}";

			ofs.close();
		}
		else
		{
			remove_path(Path(".nift/.watch/", "watched.json"));
		}
	}
	else
	{
		std::cout << "error: cannot save watch list to '.nift/.watch/watched.json' as cannot find Nift configuration file '.nift/config.json'" << std::endl;
		return 1;
	}

	return 0;
}

int WatchList::open_old()
{
	if(file_exists(".nift/config.json"))
	{
		if(file_exists(".nift/.watchinfo/watching.list"))
		{
			WatchDir wd;
			std::string contExt;
			Path templatePath;

			std::ifstream ifs(".nift/.watchinfo/watching.list");

			while(read_quoted(ifs, wd.watchDir))
			{
				wd.contExts.clear();
				wd.templatePaths.clear();
				wd.outputExts.clear();
				if(wd.watchDir != "" && wd.watchDir[0] != '#')
				{
					std::string watchDirExtsFileStr = ".nift/.watchinfo/" + strip_trailing_slash(wd.watchDir) + ".exts";

					if(file_exists(watchDirExtsFileStr))
					{
						std::ifstream ifxs(watchDirExtsFileStr);
						while(read_quoted(ifxs, contExt))
						{
							if(contExt != "" && contExt[0] != '#')
							{
								if(wd.contExts.count(contExt))
								{
									std::cout << "error: watched content extensions file " << quote(watchDirExtsFileStr) << " contains duplicate copies of content extension " << quote(contExt) << std::endl;
									return 1;
								}
								else
								{
									wd.contExts.insert(contExt);
									wd.templatePaths[contExt].read_file_path_from(ifxs);
									read_quoted(ifxs, wd.outputExts[contExt]);
								}
							}
						}
						ifxs.close();
					}
					else
					{
						std::cout << "error: watched content extensions file " << quote(watchDirExtsFileStr) << " does not exist for watched directory" << std::endl;
						return 1;
					}

					dirs.insert(wd);
				}
			}

			ifs.close();
		}
		else
		{
			//watch list isn't present, so technically already open
			/*std::cout << "error: cannot open watch list as '.nift/.watchinfo/watching.list' file does not exist" << std::endl;
			return 1;*/
		}
	}
	else
	{
		//no Nift configuration file found
		std::cout << "error: cannot open watch list as Nift configuration file '.nift/config.json' does not exist" << std::endl;
		return 1;
	}

	return 0;
}

int WatchList::save_old()
{
	if(file_exists(".nift/config.json"))
	{
		if(dirs.size())
		{
			Path(".nift/.watchinfo/", "").ensureDirExists();
			std::ofstream ofs(".nift/.watchinfo/watching.list");

			for(auto wd=dirs.begin(); wd!=dirs.end(); wd++)
			{
				ofs << quote(wd->watchDir) << "\n";

				std::string watchDirExtsFileStr = ".nift/.watchinfo/" + strip_trailing_slash(wd->watchDir) + ".exts";

				Path watchDirExtsFilePath;
				watchDirExtsFilePath.set_file_path_from(watchDirExtsFileStr);
				watchDirExtsFilePath.ensureFileExists();
				
				std::ofstream ofxs(watchDirExtsFileStr);
				for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
					ofxs << *ext << " " << wd->templatePaths.at(*ext) << " " << wd->outputExts.at(*ext) << "\n";
				ofxs.close();
			}

			ofs.close();
		}
		else
		{
			remove_path(Path(".nift/.watchinfo/", "watching.list"));
		}
	}
	else
	{
		std::cout << "error: cannot save watch list to '.nift/watching.list' as cannot find Nift configuration file '.nift/config.json'" << std::endl;
		return 1;
	}

	return 0;
}
