------------------
Nift Release Notes
------------------

TODO List (not in order of priority)
* [TODO] add multi-threading to Nift command `status`

* [TODO] output @system/@script without piping to file if using std::cout (don't forget to use os_mtx)
	- https://stackoverflow.com/questions/3318714/check-if-ostream-object-is-cout-or-ofstream-c

* [TODO] add project name to ProjectInfo.h, .nsm/nift.config etc. with @[projectname] variable/syntax

* [TODO] add script extension option to track commands

* [TODO] add error handling for rename etc. throughout

* [TODO] add more html entities (try to be similar to latex)
* [TODO] add @char/@ch syntax to template language (do both latex and html equivalents)

* [TODO] need syntax in the template language to ensure existence of a file given relative path from the page
* [TODO] further clean up reading parameters and parsing them
* [TODO] implement better variable types with scoping
* [TODO] implement type defs and function defs

* [TODO] add no template option when building
* [TODO] add way to build non-tracked pages (including without even being a site, and/or without having a page to output to)
	- need to be able to do both outputfile to stdout as well as programmingoutput
	- maybe:
		- nsm build cont-path (template-path) output-path
		- nsm render cont-path (template-path)
		- nsm render-name name
		- nsm run cont-path (template-path)
		- nsm run-name name

Version 1.25 of Nift
* changed the way @ is escaped and removed most escape characters (too likely to conflict in other places)
* added @ent syntax to template language
* added @\@, @\<, @\\ escaping syntax to template language
* improved reading parameters
* changed how strings/variables are done in preparation for more types (to come, type defs and function defs)
* now allow multiple string definitions in the one definition
* improved syntax for inputting tracked file and project/site information, now basically hard-coded constants
* fixed bug with delDir, which fixed bugs with `clone` etc.
* removed website specific terminology (lots of changes)
* added Nift command `backup-scripts (option)`
* changed one of the Nift `config` commands to `info-config`
* added Nift command `info-tracking`
* check if new content path etc. exist with `new-cont-ext` etc.
* check whether content file already exists when moving/copying 
* make sure all tracks/untracks/rms will be successful before any of them are done

Versin 1.24 of Nift
* fixed bugs with content/page/script extensions and mv/cp/rm aka move/copy/del
* updated/moved removePath to remove_path and remove_file and now removes now-empty directories
* added in ^ option for @script syntax in template language for not making a backup copy
* added in backupScripts to config files
* added Nift commands track-dir, untrack-dir, rm-dir, track-from-file, untrack-from-file, rm-from-file (note when removing large volumes of pages using rm-from-file can cause significant machine lags while running and not long after)
* added Nift commands watch, unwatch, info-watching
* got rid of pre/post build-all and build-updated scripts, costs too much time and not really needed
* added pre/post build scripts to page dependencies

Version 1.23 of Nift
* fixed Windows bugs and tidied up with pre/post build/serve scripts and @script, @scriptoutput, @scriptraw
* fixed indenting inside pre blocks with methods to input from file
* added allowing quoted string variable names with whitespace and open brackets
* improved filenames for temporary files
* added syntax @\n to template language
* added in syntax for Nift comments to template language:
	- <@-- .. --@>   (raw multi line comment)
	- @/*  .. @*/    (parsed multi line comment)
	- @--- .. @---   (parsed special multi line comment)
	- @#             (raw single line comment)
	- @//            (parsed single line comment)
	- @!\n           (parsed special single line comment)

Version 1.22 of Nift
* added in scriptExt to config files, better way to do pre/post build/serve scripts
* added command `new-script-ext (page-name) scriptExt`
* changed/improved how pre/post build/serve scripts are done
* hopefully fixed bugs with @script, @scriptoutput, @scriptraw functions
* added optional parameter string parameter to @script, @scriptoutput, @scriptraw functions
* updated Nift info commands with additional page information
* added pageinfo syntax @pagecontentext, @pagepageext, @pagescriptext to template language
* added siteinfo syntax @scriptext to template language
* added in buildThreads to config files
* added command `no-build-thrds (no-threads)`

Version 1.21 of Nift
* removed trailing '/' or '\' from @contentdir and @sitedir output
* fixed bugs with @contentdir, @sitedir, @contentext syntax
* fixed indenting when parsing parameters
* added more error handling with Nift commands new-[cont/page]-ext 
* added file input syntax @rawcontent, @inputraw, @scriptraw, @systemraw to template language
* added user input syntax @userin, @userfilein to template language

Version 1.20 of Nift
* made template language available with input parameters
* added pageinfo syntax @pagename, @pagepath, @contentpath, @templatepath to template language
* added siteinfo syntax @contentdir, @sitedir, @contentext, @pageext, @defaulttemplate to template language
* fixed indenting bugs
* fixed os_mtx functionality
* added optional sleepTime parameter for Nift serve command

Version 1.19 of Nift
* added more error handling
* added string variables
* added rootBranch and siteBranch to config files
* changed/improved/finalised how pre/post build/serve scripts are done
* fixed @script[output] and @system[output/content]
* added pre/post build-[all/updated] script support

Version 1.18 of Nift
* added FileSystem.[h/cpp] to the project
* added cpDir function to FileSystem.[h/cpp]
* renamed trash to ret_val and handle more errors
* fixed numerous minor bugs

Version 1.17 of Nift
* changed std::endl to "\n" when writing to file, 20% improvement in build-all time on some machines, no improvement on others
* changed pages set to pointer in PageBuilder.h, significantly less memory consumption
* added/improved Nift commands new-template, new-site-dir, new-cont-dir, new-cont-ext, new-page-ext
* added @systemcontent(sys-call) syntax to template language
* fixed bug with @system, @systemoutput, @script, @scriptoutput syntax

Version 1.16 of Nift
* improved multithreading
* added @inputhead syntax to the template language
* fixed read_sys_call and read_path
* improved new-page-ext

Version 1.15 of Nift
* added non-default page extension support
* added @dep syntax to the template language

Version 1.14 of Nift
* improved multithreading

Version 1.13 of Nift
* added in user-defined dependency support
* changed the way pre blocks are parsed

Version 1.12 of Nift
* fixed some bugs under the hood
* made some improvements under the hood

Version 1.11 of Nift
* added in Nift commands diff and pull
* fixed major bug with quote string function introduced in v1.10
* fixed various other minor things

Version 1.10 of Nift
* added in @system and @systemout syntax to template language
* added in pre/post build/serve script support

Version 1.9 of Nift
* added in timer for build/build-updated/build-all commands

Version 1.8 of Nift
* improved multithreading support

Version 1.7 of Nift
* added version command

Version 1.6 of Nift
* fixed open file limit bug

Version 1.5 of Nift
* added serve command to serve pages locally

Version 1.4 of Nift
* added -O3 compiler optimisation (significant improvement in build time for 10,000 pages)

Version 1.3 of Nift
* added multithreading when building all pages (significant improvement in build time for 10,000 pages)

Version 1.2 of Nift
* removed output for successful building of pages (significant improvement in build time for 10,000 pages)

Version 1.1 of Nift
* fixed clone bug
* fixed init bug
* added windows flavoured del, copy and move commands
