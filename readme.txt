NDIR - The Ultimate Color Directory Program

Written by:
Derell Licht
Fremont, CA

Email: derelict@comcast.net
Web:   derelllicht.com

****************************************************************************************
This project is licenced under Creative Commons CC0 1.0 Universal;  
https://creativecommons.org/publicdomain/zero/1.0/

The person who associated a work with this deed has dedicated the work to the
public domain by waiving all of his or her rights to the work worldwide under
copyright law, including all related and neighboring rights, to the extent
allowed by law.

You can copy, modify, distribute and perform the work, even for commercial
purposes, all without asking permission. 

****************************************************************************************
NOTES: 

> As of version 2.54 (Sep 2022), NDIR is now a 64-bit program.
  This was necessary in order to support resolving target filename from symlinks
  
> In V2.58 (May 2023), removed /mm (list multimedia info) option, moved to media_list utility  

****************************************************************************************

NDIR - The directory lister.

This program is a color directory lister with many options.  In addition
to the standard file-display options, it also offers special display
options to:

   ->  Display drive summaries for all disk drives in your system
   ->  Display a complete, recursive directory tree that shows summary
       information for all subdirectories on a drive.  *NO OTHER*
       color directory lister shows as much information in the TREE
       view as NDIR does!!
   ->  Batch mode: enables user to create batch files from lists
       of filenames in a directory.

The NDIR software package contains the 32-bit version of NDIR.  
It properly handles long filenames under WinNT.

Effective with release of version 2.44, in Nov 2016, I am no longer distributing
the 16-bit version of NDIR.  Nothing has changed with that version since 2003,
and I no longer have the ability to either build or run that version.
I also am abandoning the ndirh utility, which was never particularly impressive.    

***************************************************************************

                                Installation

The only installation required by NDIR is to place *.EXE and NDIR.INI
somewhere in your path, so they can be located when you run them from a DOS prompt.
If ndir.ini does not exist in the location where the .exe file is located,
it will be created with default values, the first time the program is run.

***************************************************************************

- Program configuration

Ndir is now configured via a configuration file, called NDIR.INI.  
*All* of the executables use the same configuration file.  A default INI
file is included with this distribution; I recommend copying the INI file and
the appropriate executable files to a directory that is in your path. 

When NDIR first executes, it searches for NDIR.INI.  The search method is:

1. search the current subdirectory

2. If not found, search the directory where the executable file was located.

3. If still not found, create a default NDIR.INI in the location where the
   executable file was located, and then loads that file.

One advantage of this search method is that you can have a default file with
your normal settings, but if you want "special handling" in a certain 
directory, you can make a copy of the INI file there, and put different 
settings in it.  For example, you might want /2 normally, but want /6 in 
directories with many files.  NDIR.INI lets you easily do this!!

Another advantage of using INI files is that you'll never lose your settings 
if I revise the program, as has periodically happened in the past.  There's 
also no need to do the NDIRMOD trick to copy configurations from one program 
to another.  The INI format is guaranteed to remain valid in the future, 
regardless of what I do to internal data formats.

***************************************************************************

History of changes:

See revisions.txt for detailed revision history.

The -o option now has a required following numeric digit (i.e., it is 
specified as -o0, -o1, or -o2). More options will probably follow this 
pattern in the future, in order to increase the flexibility of the program.  
As a result, when using a numeric switch as a normal switch character, it is a 
good idea to place it FIRST in a group of switches, to avoid ambiguity in 
interpretation.  In other words, if you want a single-column display, it's 
better to specify -1tr rather than -tr1, in case I later add a numeric option 
to the -r switch!!

Converted both programs to use an INI file for configuration.  NDIRMOD has now 
been eliminated, and in fact won't work at all.

There's still a small problem with determining the cluster size on
network drives from the 32-bit programs; I haven't yet figured out
that trick.  

***************************************************************************
                Handling System/Hidden/Readonly files

NDIR can now display S/H/R files with a background color selected by
the user.  The foreground color is determined by the file attribute,
with the BRIGHT bit set, and a background color which is specified
by the user.  The following table lists the foreground colors for various 
file attributes:

        S  H  R         foreground color
      =================================================
        0  0  0      determined by file extension
        0  0  1      Bright Blue
        0  1  0      Bright Green
        0  1  1      Bright Cyan
        1  0  0      Bright Red
        1  0  1      Bright Magenta
        1  1  0      Bright Brown (Yellow)
        1  1  1      Bright White

***************************************************************************

WHAT DOES NDIR DO THAT EVERY DIR LISTER DOESN'T ? ?

Here are some of the features of NDIR, a summary of which can be
obtained by typing NDIR -? :

- Long-filename support (MSDOS 7.00+ and NTFS)

- FAT32 and NTFS support.

- Configuration via INI file(s).

- Full color file display.

- List executable files only (.BAT, .COM, .EXE) 

- Full control over display format 
  (pause-on-full-screen, clear-screen-before-listing, 
   select number of columns and amount of file info shown).

- File exclusion (list all files EXCEPT *.ult, for example).

- Full directory TREE, showing size of current directory
  as well as current directories PLUS subdirectories.

- Show ALL files, including read-only, hidden and system files.

- A separate configuration program, for customizing default options,
  as well as all color and extension settings.

- Support for various console rows/columns

- A "batch" mode which can assist you in creating batch files.


WHAT DOESN'T NDIR DO THAT EVERY DIR LISTER DOES OR DOESN'T ? ?

Functions that are NOT supported (because I ran out of time before other
projects developed), but may be added if interest is shown, are:

- Compressed FAT32 drives are not reported correctly.  This will be added 
  if anyone tells me they are using disk compression; with huge disks so 
  cheap, I'm assuming nobody is paying the performance penalty.

- Support for global subdirectories; i.e., search for files across all
  subdirectories under current default. The framework for implementing 
  this is in place, but it would take a fair amount of work to finish 
  it, and I'm waiting to see if anyone cares.

- Ability to search for files with only a certain attribute or combination
  of attributes.

- Anything else that I didn't think of, and nobody's requested.

***************************************************************************

WHO ARE YOU ? ?  (who-who. who-who.)

I am a retired Software Engineer, living in Fremont, CA.  I started out as a
digital hardware engineer, but discovered that software is more fun!!!
I have professional experience with C (mostly), numerous assemblers, 
PASCAL, Visual Basic (but I got better), and DBASE III (don't laugh).  

***************************************************************************

NDIR Command-Line switches
--------------------------

This is a list of the command-line switches supported by NDIR.
(Many of these options do *not* apply to NDIRH)

This list can also be obtained by typing 'NDIR /?'

USAGE:  NDIR <filespecs> -options or /options !<exclusions>
 
 OPTIONS                      FUNCTIONS
   -a *     List ALL files (hidden, system, read-only, etc.).
   -c *     Clear screen before listing.
   -p *     Pause on full screen.
   -m *     Minimize size of header and footer.
   -w *     Use special colors for system/hidden/readonly files.
   -d *     Display directory TREE (work with -s, -n (default), -r).
   -e       Sort by extension.
   -n        "   by name.
   -s        "   by file size, smallest first.
   -t        "   by Date, oldest first.
   -z        "   by DOS order (no sort).
   -r *     Reverse normal sort order.
   -1       Display one column,  with name/size/date/attr.
   -2          "    two   "   , with name/size/date.
   -3          "    three "     (short-filename form only)
   -4          "    four  "   , with name/size.
   -6          "    six   "   , with name only.
   -i       Display drive summary for all drives in system.
   -?       Display HELP screen.
   -x *     List executables only (.EXE,.COM,.BAT).
   -l *     Toggle long-filename enable flag (MSDOS 7.00+ only)
   -k *     Toggle color mode. (non-color mode is redirectable)
   -j *     Use standard ASCII (for redirection). (forces -k)
   -5 *     Switch to EGA 43-line/VGA 50-line mode.
   -o *     Display LAST_MODIFY (default) or FILE_CREATE date/time.
   -oN      Date/Time display: 0=LastWrite (default), 
                               1=LastAccess, 
                               2=FileCreated
   -u *     List filenames in UPPERCASE.
   -v       Display distribution status and author information.
   -g *     List directories FIRST.
   -h *     List files horizontally.
   -f *     List files only (No directories).
 
   -b       Batch mode;  files listed in one column.
            (This format can be redirected to a batch file)
   ["string"  specifies a string BEFORE each filename (Batch mode)
   ]"string"  specifies a string AFTER  each filename (Batch mode)
 
NOTE: items with a * after the flag are TOGGLES
 
***************************************************************************
                     
NOTES on using command-line switches:
-------------------------------------
NDIR's functionality can be modified in three ways:

1. Type NDIR <options> as documented above.

2. Modify the NDIR.INI file as required.

3. Use the NDIR ENVIRONMENT VARIABLE to specify NDIR options.
   This probably isn't worth the effort anymore, since editing an INI file
   is just as easy.  In fact, it hasn't been tested in a *long* time.
   In any case:

   At the DOS prompt, type SET NDIR=-options
   This method is convenient when you wish to make temporary, on-the-fly
   modifications to NDIR's configuration.

   If NDIR.EXE is renamed to some other filename, the name of the 
   environment variable will change with it.  For example, if NDIR.EXE
   is renamed to N.EXE, you would use SET N=-options to set the 
   environment.

Order of interpretation for command options:

The environment variable will override any default options in NDIR.EXE.
Options typed on the command line will override both the default options
AND the options in the environment variable.

//****************************************************************

Notes on using Batch mode (-b) :
--------------------------------
The purpose of batch mode is to assist you in creating DOS batch files.
The output from NDIR in batch mode is a single-column list of the 
requested filenames, with no other information.  This output is written
using BIOS functions, so the output can be redirected to printer or file.

In addition, NDIR provides a method for inserting text before and after
each filename in the listing.  The LEFT square bracket ([) is used to
indicate a string PRECEDING each filename.  The RIGHT square bracket (])
is used to indicate a string FOLLOWING each filename.  This is easy to
remember if you think of the filename enclosed in 
square brackets ( [filename] ).  Then it's easy to remember which string goes
in front of (filename) and which goes after.

Let's look at some examples of NDIR batch-mode commands.
Assume that the following files are in the current subdirectory;

NDINSTAL.C          NDINSTAL.EXE
NDIR.C              NDIR.DOC
NDIR.EXE            NDIR.HLP
NDIR.TXT            NDIRMOD.C
NDIRMOD.EXE         NDIRSRC.BAT
NDIRZIP.BAT         NUPDATE.BAT

Here are some sample commands and their outputs:
------------------------------------------------

   NDIR -B *.BAT

This will list all BAT files, in single-column, with no other information.
The output looks like:

ndirscr.bat
ndirzip.bat
nupdate.bat

------------------------------------------------

   NDIR -B *.TXT *.DOC ["PRINT " >printit.bat

The output from this command will be a list of all DOC and TXT files,
preceded by "print ", and these lines will be written to a file 
called PRINTIT.BAT .  The contents of PRINT.BAT will be:

print ndir.doc
print ndir.txt

------------------------------------------------

   NDIR -B *.EXE *.BAT ["COPY " ]" B:" >copyexec.bat

This will list all EXE and BAT files, preceded by 'copy ' and followed by ' b:'.
The output of NDIR will be written to COPYEXEC.BAT.
The output:

copy ndinstal.exe b:
copy ndir.exe b:
copy ndirmod.exe b:
copy ndirsrc.bat b:
copy ndirzip.bat b:
copy nupdate.bat b:

------------------------------------------------
