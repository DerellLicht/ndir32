# NDIR32/64 Changelog

## [2.71] - 2026-08-08
- fixed: search for *.cpp would include .suppress.cppcheck
- all symlinks and .lnk files now resolve targets successfully

## [2.70] - 2026-07-31
- implement enum values for n.tree, move files_only flag out of this element
- implement new value in n.tree (-d4), to support "max filename length" display
- redirection now outputs Unicode strings correctly
- removed flags for "no color" and "uppercase only"
- tree display ("max filename length" option) now sorts on overall maxlen (normal or reversed)

## [2.69] - 2025-08-11
- In disk summary (-i) display, add a percentage-of-total column
- dirtree: -y switch now eliminates .git folder from searches

## [2.68] - 2025-06-23
- Converting various strings to vector and wstring classes
- Fix bugs in handling batch left/right string command-line arguments
- Check for duplicate filenames in final file list

## [2.67] - 2025-06-08
- Converting all malloc() to new ; adds 110KB to file size, but enables non-static data init.

## [2.66] - 2025-05-26
- Implemented support for clang-tidy static analyzer

## [2.65] - 2025-04-26
- Fixed a bug in filename listing, when filename was exactly the same width as column

## [2.64] - 2025-03-17
- implement UNICODE support
- Deprecate low_ascii (actually, remove high_ascii) due to Unicode requirements

## [2.63] - 2025-01-04
- Dirtree: fix bug with very long folder names

## [2.62] - 2024-01-10
- Add support for UNICODE file/folder names
- Removed support for short filenames

## [2.61] - 2023-12-12
- Add support for extracting targets from .lnk (shortcut) files

## [2.60] - 2023-11-10
- Code cleanup in diskparm.cpp - optimizing, remove unused code

## [2.59] - 2023-05-16
- Reverted change in V2.56, which broke scrolling

## [2.58] - 2023-05-13
- Remove /mm (list multimedia info) option, moved to medialist utility

## [2.56] - 2023-01-17
- Fix bug in non-minimize display, with screen > 132 characters

## [2.54] - 2022-09-30
- This is built with 64-bit compiler (ndir64.exe), to support symlink name resolution

## [2.53] - 2022-09-30
- In -1 display, add support for symlink files

## [2.52] - 2022-07-15
- /ii - show used space, vs free space, in disk summary

## [2.51] - 2022-07-04
- /mm - Add support for other webp variants

## [2.50] - 2021-08-05
- /mm - Add support for webp extension

## [2.49] - 2020-09-06
- fix mis-handled dir-tree level-limiting operation
- Add option to alter level limit, using multiple commas

## [2.48] - 2020-06-20
- Fix parsing error with small mp3 files; in some case, offset to next frame is off by 1 byte.

## [2.47] - 2018-08-11
- Modify directory tree to support consoles > 80 chars wide
- tweak positioning of Volume Label on wide consoles

## [2.46] - 2018-07-23
- Modify to accept extensions in INI file, with *or* without the '.'

## [2.45] - 2017-12-04
- Remove incorrect limitation on length of exclusion extensions

## [2.44] - 2016-11-03
- Modify /i to support >9TB drives

## [2.43] - 2015-03-12
- Fix problem with %llu not actually printing u64 correctly.
- First fix was to add -std=c++98, but this added over 30% to file size.
- Better solution was to switch to GCC/TDM V4.4.1;
- This required replacing %llu with %I64u, but it worked, without an increase in file size

## [2.42] - 2015-02-17
- replace i64tostr class with a simple C function

## [2.41] - 2013-12-17
- Once again, try to get sector/cluster sizes right

## [2.40] - 2013-04-02
- Convert file data/time computations from FileTimeToDosDateTime() to FileTimeToSystemTime(); This provides 1-second resolution on file timestamps

## [2.39] - 2012-10-23
- /i now properly identifies many BluRay devices
- /i now allows enough space for terabyte drive sizes

## [2.38] - 2011-05-27
- /i now tries to determine more specific info on CD-type devices.
- At this point, it still does not detect BluRay devices as such.

## [2.37] - 2011-01-13
- /mm - added support for .sid files (well, partially)

## [2.36] - 2009-06-17
- /mm, added support for .ico and .cur files
- linted the code

## [2.35] - 2009-01-04
- Re-implement redirection flag, so output files are useable.

## [2.34] - 2007-10-10
- fix computation of MFT size.  On large drives, the MFT info documented by Sysinternals' ntfsinfo does not give meaningful results, so I'm forced to skip my free-disk correction.

## [2.33] - 2007-06-17
- Update disk-params report to recognize more drives

## [2.32] - 2006-12-22
- Add total playing time for Wave files in /mm display
- Add mp3 support to /mm option
- Add total playing time for mp3 files in /mm display

## [2.31] - 2006-08-18
- Fix handling of volume label in directory tree with color disabled
- Fix a but in the size-display option from version 2.30, which caused -1 display to show all sizes as 0.

## [2.30] - 2006-08-15
- Add options to size display, to show as KB/MB

## [2.29] - 2006-07-24
- Linted code, possibly introducing bugs

## [2.28] - 2006-07-21
- Fix low_ascii display in directory tree
- Add -a1 option to show file attributes as HEX value

## [2.27] - 2005-12-24
- Fix "disk params" display so it aligns correctly for sizes > 99GB

## [2.25] - 
- Jason Hood: Fix scrolling problem when buffer size is greater than screen size
- Eliminate html-redirection; this is broken in Bash shells

## [2.24] - 
- add -mm (multimedia) mode, which display special info about special files.  Currently, only .wav, .jpg, and .gif will be supported.
- Update email and website data

## [2.23] - 
- Fix a problem with directory tree on Win2K when read from WinNT4 directory.
- Allow ESCAPE key to cleanly exit from reading files in the directory tree.  It will display all files that were read up to that point.

## [2.22] - 
- Add some support for UNC paths
- adding new color-spec format to ndir.ini
- Added -d3 command, which displays combined file/directory counts and file sizes.
- Fix dir-tree alignment on long path names
- In Directory Tree, show large sizes in KBytes
- In Directory Tree, show progress bar while reading all the data.

## [2.19] - 
- Change Email address and web site
- Added -d2 command, which displays directory tree with *counts* of files and directories, rather than *sizes* of files and directories. (NDIR32 only)
- Added "," switch to /d listing, to display only one level of directory summary.  This makes it easier to see where disk usage is concentrated. (NDIR32 and WNDIR only)
- Modified ndir32 to support screen widths other than 80 columns (mostly for WinNT).

## [2.17] - 
- Fix bugs in parsing of INI file
- Fix ancient display bug in 16-bit program

## [2.16] - 
- Remove extra blank line at end of multiple- filespec listings.
- ndir16: fix decade-old bug in LFNFindFile, where I wasn't restoring DS before using it.

## [2.13] - 
- Fix screen scrolling with ndir32 on Win9x.
- The Steven Bensky mod (1) - restore -q listing to ndir32 - also, improve it with long filenames in both implementations.
- The Steven Bensky mod (2) - Implement short filename display in ndir32.
- Restore prompt color properly in ndir32, on Win9x systems which don't run ANSI.SYS.

## [2.08] - 
- Change my email and web addresses
- ndir32: fix a bug with WinNT windows that had screen buffer larger than displayed size.

## [2.04] - 
- Expand -o to support all three file date/time options.
- o now has an optional following number.

## [2.03] - 
- Mainly, converting to INI files for configuration, instead of NDIRMOD.
- A variety of minor bug fixes that I never documented

## [2.01] - 2000-07-05
- Converted get_cluster_size() to use the undocumented WinNT function from the ntfsinfo utility, Copyright (C) 1997 Mark Russinovich

## [2.00] - 1999-03-01
- volume_name[] needs to be _MAX_PATH in size, for Win32 support; RedHat Linux CD was over-running our buffer.

## [1.86] - 
- 12-02-97  - Fix bug in handling filenames beginning with '.'
- Fix "year 2000" bugs

## [1.85] - 
- 04-27-97  - Fix short-filename bug.

## [1.84] - 
- 04-05-97  - Fixed TREE display to recognize uppercase flag when long filenames are disabled.
- Hacked NDIRMOD to clear screen on exit, so screen colors are restored correctly. 04-23-97  - Make EXT sort case-insensitive.
