------------------
Nift Release Notes
------------------

Version 3.0.3 of Nift
* fixed bug with json config on Windows
* fixed bug in Parser::read_paramsStr

Version 3.0.2 of Nift
* fixed bug with variable definitions
* fixed bug with variable if conditions (trying to compile exprtk expression segmentation faults)

Version 3.0.1 of Nift
* fixed bug copying/moving pages with non-default content and/or output extension(s)
* fixed bug with copying whitespace when parsing functionstein, especially for string definitions
* fixed bug with semicolons in quoted string definitions
* fixed bug with using seekp/tellp on Windows when converting config files to json
* changed to adding operator function names with zero params to output rather than throwing errors

Version 3.0.0 of Nift
* now allow relative paths for `input`/`inject`
* added duplicate of `!i` option called `!indent` for `input`/`inject`
* can now have names ending in `/` or `\` for implicit index eg. `about/index` vs `about/`
* experimentally allow names containing `.` (seems to work, lots of find_first_of('.') still though)
* have allowed cp/copy, mv/move and rm/del with nift commands again
* changed silent option to progress for build and status commands
* made some lolcat-cc modifcations
* reverted LuaJIT 2.1.0-beta3 to website version
* added 'browse' nift command
* modified default project constructed with 'nift init .html'

* changed to using JSON for all config files (auto updates projects)
* fixed compiler flags on LuaJIT for using LuaRocks

============

Version 2.4.12 of Nift
* further improved line number tracing for errors
* added get_pwd and pwd to embedded lua
* updated to newer versions of LuaJIT 2.1.0-beta3 and ExprTk

Version 2.4.11 of Nift
* added basic/fast multi-threading options to poke and rmv
* changed to strictly using templatestein when parsing constructed string for join function
* fixed up being able to use "fn" instead of "function"
* fixed up error function to handle multiple parameters
* improved reading bracketless codeblocks inside bracketed codeblocks (no longer need an empty end line)
* hunted down some bugs wth line number tracing for errors

Version 2.4.10 of Nift
* fixed up lolcat to handle finding the end of things with 'm' to avoid so much white
* fixed up bug with reading {} in unbracketed code blocks
* added trying to compile and evaluate unrecognised lines using ExprTk with functionstein (f++)
* added option (default value can be changed with `exprtk.eval_params` fn) to compile and evaluate params with ExprTk

Version 2.4.9 of Nift
* added in ability to run scripts with `nift script-path` as well as `nift run script-path`, makes shebangs nicer for example
* further improved language detection
* improved lolcat piping of system calls across multiple statements and made more consistent across f++/n++/Lua(JIT)/ExprTK
* hopefully fixed bugs with n++ and zero param non-fns output (in particular $ instances)
* added script path as first parameter for argv

Version 2.4.8 of Nift
* added in default of trying system call if failure with lua(jit) or exprtk in interactive (interp/sh) mode,
  they work better as shell extensions this way
* cleaned up language choice so ExprTk and templatestein and all reasonable derivatives work okay
* fixed up nsm_lang fns for Lua(JIT) and ExprTk to work with both string and char version of language selection 
* updated sys/system fn output to use the called fn name rather than defaulting to system
* added in passing parameters to the nift run command with variables 'argc' (int) and 'argv' (vector<string>)

Version 2.4.7 of Nift
* Reverted to handling @fns with f++, too ingrained in the codebase
* Reverted to calling sys call on whole line for zero param functions
* Reverted to breaking at non-escaped @'s when reading function names
* Fixed up allowing for ';' at end of statements (hopefully no bugs/undesirable 
  features), though any potential system call will be called on whole line for
  possibly multiple statements, use a new line if needed
* cleaned up (hopefully) language choce with interactive REPL's (both interp and sh) and run

Version 2.4.6 of Nift
* Initialised all bLineNo's to zero (started getting warnings on OSX)
* Fixed up "run commands" in-built scripts from last version to handle language names as well as chars for script extensions
* Added skip_whitespace fn to clean Parser.cpp codebase up a little bit
* Can now optionally use `fn` in place of `function`, mostly intended for interpreter/shell sessions

Version 2.4.5 of Nift
* You can now do `nift sh/interp lang` and provided lang starts with the same char as the language you are specifying the prompt will match
* Added `prompt.char(string)` command
* Added in global `$[cmd]rc.$[langChar]` "run command" support on start of an interpreter or shell session
* Added in local `.nsm/[interp/sh].$[langChar]` "run command" support on start of an interpreter or shell session
* Added in treating a single parameter for f++/n++/lua/exprtk function calls as a file-path to a file
* Added in `exprtk.file` function to read and process ExprTk code at the file-path specified parameter
* Added in `f/file` option for `exprtk` and `exprtk.compile` options

Version 2.4.4 of Nift
* Built-in improvements of uneccessarily repetitious code for lolcat-cc
* Fixed $() syntax to hopefully not conflict with common bash code
* Added lolcat.cmd and lolcat.status functions
* Added alternative `inj` option for `sys/system` calls for terminal/cmd line brevity

Version 2.4.3 of Nift
* Fixed lolcat bug - it was a missing a space between `nift/nsm` and adding `lolcat-cc` in `lolcat_init`
* Improved escape code handling for `lolcat-cc`, now seems to work for most animations
* Stopped Nift from creating hash info files wherever it runs a script (anyone remember what that code is for?)

Version 2.4.2 of Nift
* fixed some rather embarrassing bugs for initialising projects

Version 2.4.1 of Nift
* added duplicate Nift commands edit/open for opening files from page names
* added duplicate Nift commands medit/mopen for opening files for a specific page name from mirror directories
* added Nift command mbcp for running build-commit-push on multiple mirrors

Version 2.4.0 of Nift
* added some general 'unknown error' try-catch blocks to help avoid REPL sessions fully crashing
* made a few alterations to `FixIndenting.cpp` including a stern "use at your own risk" warning
* fixed a bunch of bugs and added a few try-catch blocks to catch unknown bugs without crashing
* removed `info-config` command (can now use Nift to open the config files!)

Version 2.3.13 of Nift
* added program with makefile option to clean up indenting for source code
* fixed indenting whitespace for the Makefile
* fixed major bug with reading blocks (both bracketed and non-bracketed)
* patched (ie. reverted `LDFLAGS` to `LINK`) makefile for FreeBSD and Gentoo
* cleaned up various ways of using lolcat with Nift
* added native support for external lolcat programs using `lolcat.on(lolcat_command)`
* (breaking change sorry) had to change to $ before grave accent ExprTk expressions so that functionstein works better as both a scripting extension language and a shell extension language, especially for flashell (hopefully not an issue when using Nift with perl or anything else)
* (breaking change sorry) no longer require @ in front of grave accent ExprTk expressions 

Version 2.3.12 of Nift
* one liner fix of not reading terminal config string quoted
* added `nsm config global/project` command to open config file with configured editor
* added improved mod fn from lolcat-cc to Lolcat.cpp

Version 2.3.11 of Nift
* added config support for number of pagination threads
* have changed to leaving instances of '@' with functionstein (f++) (fixes using @, eg. cloning from bitbucket)
* ran a regression on read block code (python gang assemble)

Version 2.3.10 of Nift
* added '!round' option to console fn
* added 'f++' and 'n++' options to exprtk and exprtk.compile fns
* added '...' option for std::vector definitions (parameters are values)
* added vjoin and ? (ternary operator) fns
* changed options/parameters/types variables for user-defined structs/fns to std::vector<string>
* added add_scope, add_member_fns and replace_vars fns
* added 's' (add scope) option to if, for, while, do-while and fn calls
* added 'mf' option to definitions and fn calls
* fixed various bugs
* made various improvements

Version 2.3.9 of Nift
* added exprtk_disable_caseinsensitivity for ExprTk
* merged PR from Mamadou fixing support for compiling with Lua 5.2 and 5.1
* added !mf option to definitions
* added in types std::vector<double> and std::vector<string>
* added in std::vector. fns at, erase, pop_back, push_back, set, size (also member fns)
* added in stream. fns close, open (also member fns)
* added in exprtk.compile, exprtk.eval, exprtk.load and exprtk.str fns for f++/n++
* added in exprtk_compile, exprtk_eval, exprtk_load and exprtk_str fns for Lua
* added in to_string fn for ExprTk
* fixed so that fn name/options/params are parsed with f++ always
* fixed various bugs
* made various improvements

Verstion 2.3.8 of Nift
* added clang support to Makefile (pass `CXX=clang` to Makefile)
* fixed up Makefile and Lua.h for various versions of Lua and LuaJIT
* changed how pagination works so that paths are not broken
* added b/block, pb, f++ and n++ options to = and write fns
* added exprtk.add_variable fn
* added lua_version fn
* improved errors for lua_tonumber and lua_tostring
* fixed a couple of bugs with tab completion
* fixed various bugs
* made various improvements

Version 2.3.7 of Nift
* fixed bug with retrieving present git branch when git is in another language
* cleaned up GitInfo.cpp
* changed new-cont-dir/new-output-dir to mve-cont-dir/mve-output-dir
* removed cp/copy mv/move rm/del from commands (use nift specific)
* added option to embed Lua 5.3 instead of LuaJIT at compile time
* fixed up options for compiling with system installed LuaJIT/5.3 instead of bundled version
* added options to compile version without clear lines, colours and/or progress
* added option to compile binary for Vercel's servers

Version 2.3.6 of Nift
* improved how Nift searches for installed version of lolcat
* fixed bug with lolcat output
* embedded HashTk
* added hash fn
* added hash and hybrid modes for incremental builds
* fixed various bugs
* made various improvements

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
