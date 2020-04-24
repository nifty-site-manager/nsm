------------------
Nift Release Notes
------------------

Version 2.3.5 of Nift
* added REPL shortcut support for more terminals
* added support for powershell (add `terminal ps` to global/project config files)
* made some cosmetic improvements to general output
* now prevent output colour until global/project config is loaded
* renamed type fn to typeof
* made some improvements to cpy/mve/rmv fns with regards to working with permissions
* now switch cat/type cp/copy ls/dir mv/move rm/del fn names on Windows vs (Free)BSD/Linux/OSX
* added in-built version of lolcat-cc
* fixed various bugs
* made various improvements

Version 2.3.4 of Nift
* fixed major bug with new copy directory function
* added in more familiar syntax for incrementing and decrementing variables
* modified ExprTk to remove the old variable when registering a duplicate
* added exprtk option to forget fn to remove variables from both Nift variables and ExprTk symbol table
* added skipping whitespace to functions that needed it 
* added cpy/mve/rmv commands

Version 2.3.3 of Nift
* fixed major bug where project content directory was deleted when removing a single tracked file

Version 2.3.2 of Nift
* added in proper cpy/lst/mve/rmv functions
* renamed touch fn to poke
* added in more familiar syntax for const and private variable definitions
* added in lolcat, lolcat.on and lolcat.off fns for rainbow output on FreeBSD/Linux/OSX
* cleaned up output (added a few select emojis on Linux/OSX)
* hopefully fixed bug with build status/progress output
* fixed various bugs
* made various improvements

Version 2.3.1 of Nift
* added in pagination, including fns:
	- item
	- paginate
	- paginate.no_items_per_page
	- paginate.separator
	- paginate.template
  and constants:
    - paginate.no_items_per_page
  	- paginate.no_pages
  	- paginate.page
  	- paginate.page_no
  	- paginate.separator
* added in getenv, refresh_completions and replace_all functions
* added in sys fn for ExprTk
* added in exprtk and sys fns for Lua
* added in alternative way to define variables, now very similar to other languages
* added in way to add $[] print syntax with user-defined types
* added in f++/n++ options when defining structs/types
* added round option to exprtk fn
* fixed various bugs
* made various improvements

Version 2.2 of Nift
* added in mod functions % and %=
* fixed various bugs
* made various improvements

Version 2.1 of Nift
* embedded lua(jit) and exprtk
* added in-built template language n++ and in-built scripting language f++
* added CLI interpreters for f++, n++, lua and exprtk
* added shell extensions for f++ and n++  
* added blank/empty/no template option
* changed to $[varname]/$(varname) instead of @[varname]/@<varname>
* added - between words for hard-coded constant variables 
* added lots of functions to Nift's template languages
* various other smaller changes and bug fixes

Version 2.0.1 of Nift
* fixed bug when cloning a website repository from an empty directory (and any other similar bugs from using remove_path where remove_file is more appropriate, ie. when creating temporary text files)

Version 2.0 of Nift
* changed @inputcontent to @content()
* changed @inputraw(file-path) to @input{raw}(file-path)
* addded if-exists/raw option to @input(file-path) and @content()
* added file/name options to @pathto
* changed @userin(msg) to @in(msg) and @userfilein(msg) to @in{from-file}(msg)
* added if-exists/inject/raw/content options to @script/@system
* changed from using @[varname] and @{varname} to @[varname] and @<varname> when printing variables
* changed to @funcname{options}(params) for function call syntax
* changed from using * option to parse params to {!p} option to NOT parse function name, options and params
* changed from using ^ option to not backup scripts to {!bs} option
* added read_params
* fixup up read_def and read_func_name
* fixed multi-line comments

============

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
	- /\*  .. \*/    (parsed multi line comment)
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
