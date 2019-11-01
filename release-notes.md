------------------
Nift Release Notes
------------------

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
