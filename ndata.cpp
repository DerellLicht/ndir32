//*****************************************************************
//                                                                 
//  NDATA.CPP - NDIR global data declarations                      
//                                                                 
//  Written by:  Daniel D. Miller  (the Derelict)                  
//                                                                 
//  Last update:  01/15/01 20:25                                   
//                                                                 
//  compile with makefile                                          
//                                                                 
//*****************************************************************

#include <windows.h>
#ifdef _lint
#include <stdlib.h>  //  PATH_MAX
#endif
#include "ndir32.h"

//*****************************************************************
//                  CONFIGURATION VARIABLES (new style)
//*****************************************************************
ndir_data n =
{
   //  default colors
   0x13, 0x02, 0x0A, 0x08, 0x01, 0x05, 0x0C, 0x04, 0x09, 0x18, 0x06,

   //*********************************************************
   //                program state flags                      
   //*********************************************************
   0, //  uppercase = false
   1, //  sort by filename
   0, //  no reverse
   1, //  clearscreen = true
   1, //  pause on fullscreen = true
   2, //  display format = 4column
   0, //  Minimize display (make abbreviated header/footer)
   0, //  display normal files, not tree
   1, //  show all files = true
   1, //  list directories first = true
   0, //  show executables only = false
   0, //  display help screen = false
   0, //  display information screen = false
   0, //  list files horizontally = false
   0, //  use low-ASCII characters = false
   0, //  display in batch mode = false
   0, //  unused1
   1, //  display files in color = true
   0, //  showSHRfiles
   0, //  switch to EGA43/VGA50 line mode
   0, //  display disk_drive summary
   0, //  file-date option (MSDOS 7.00+ only)
   0, //  1 = force old DOS 8.3 filenames in MSDOS 7.00+
   0, //  0=normal tree, 1=show only one level of children
   0, //  uchar long_attr: show full attribute as hex data
   0  //  0=bytes/normal, 1=KB, 2=MB
};

attrib_list attr_table[MAX_EXT] ;
unsigned attrib_count = 0 ;
uchar dtree_colors[MAX_DIR_ENTRY] ;

//*****************************************************************
//              END OF CONFIGURATION VARIABLES
//*****************************************************************

WIN32_FIND_DATA fdata ; //  long-filename file struct

static char dnamestr[4] = "c:\\" ;
char *dname = &dnamestr[0] ;  //  used by is_lfn_supported() call

int lfn_supported = 1 ; //  always true for 32-bit version

// ulong diskbytes, diskfree ;
ULONGLONG diskbytes, diskfree, clbytes ;

//*****************************************************************
ffdata *ftop = NULL, *ftail ;

char* target[20] ;
char volume_name[PATH_MAX] ;

//  name of drive+path without filenames
char base_path[PATH_MAX] ;
unsigned base_len ;  //  length of base_path

// struct _find_t fblk;

unsigned tcount = 0 ;   //  number of target filespecs
unsigned start, finish ;
char leftstr[37], rightstr[37] ; //  used by batch mode
char tempstr[MAXLINE] ;
uchar tline, bline, vline, xline, dline ;
int  filecount = 0 ;

int  columns ;           //  number of display columns on screen
unsigned curlines, lines, linecnt ;

double total_ptime = 0.0 ;
//*********************************************************
//  NDIR information screen
//*********************************************************
char *idtxt[] = {
   " ",
   "Copyright 1990, 1993-2006 by:",
   " ",
   "          Daniel D. Miller",
   "          4835 Coco Palm Drive",
   "          Fremont, CA  94538",
   " ",
   "          Email:    derelict@comcast.net",
   "          Website:  home.comcast.net/~derelict",
   " ",
   "This program, NDIR.EXE, and its associated files, are hereby released as",
   "Freeware, though I retain the copyrights on them.  Please feel free to",
   "distribute copies to anyone who is (or might be) interested in them.",
   " ",
   "If you like this program, a letter, post card or Email telling me what you",
   "do or don't like would be very welcome, and will help me decide what ",
   "features to add in the future.  If your letter is accompanied by a small ",
   "($10 or more) donation , I will send you the current version of the",
   "program as well.  (Of course, if you just send an Email, I'll send it",
   "to you for free!!)  If you DO contact me, PLEASE mention what version",
   "you are using, so I'll know how far behind you are.",
   NULL } ;

//*********************************************************
//  NDIR help screen
//*********************************************************
char *helptxt[] = {
" ",
"USAGE:  NDIR <filespecs> -options or /options !<exclusions>",
" ",
" OPTIONS                      FUNCTIONS",
"   -a *     List ALL files (hidden, system, read-only, etc.).",
"   -a1      List attributes in HEX format.",
"   -c *     Clear screen before listing.",
"   -p *     Pause on full screen.",
"   -m *     Minimize size of header and footer.",
"   -w *     Use special colors for system/hidden/readonly files.",
"   -d       dir TREE: normal size display (work with -s, -n (default), -r).",
"   -d2      dir TREE: file/directory counts",
"   -d3      dir TREE: mixed size and file/directory counts",
"   -e       Sort by extension.",
"   -n        \"   by name.",
"   -s        \"   by file size, smallest first.",
"   -t        \"   by Date, oldest first.",
"   -z        \"   by DOS order (no sort).",
"   -S0      Show sizes in mixed bytes/KB",
"   -S1      Show sizes in Kilobytes",
"   -S2      Show sizes in Megabytes",
"   -r *     Reverse normal sort order.",
"   -1       Display one column,   with name/size/date/attr.",
"   -2          \"    two   \"   ,   with name/size/date.",
"   -3          \"    three \"   ,   with name/size/mmYY. (short fn only)",
"   -4          \"    four  \"   ,   with name/size.",
"   -6          \"    six   \"   ,   with name only.",
"   -i       Display drive summary for all drives in system.",
"   -l *     Toggle long-filename enable flag (NDIR16 and MSDOS 7.00+ only)",
"   -k *     Toggle color mode.",
"   -j *     Use standard ASCII (for redirection). (forces -k)",
"   -5 *     Switch to EGA 43-line/VGA 50-line mode.",
"   -u *     List filenames in UPPERCASE.",
"   -oN      Date/Time display: 0=Last Write, 1=Last Access, 2=File Created",
"   -x *     List executables only (.EXE,.COM,.BAT).",
"   -v       Display registration/update information.",
"   -?       Display HELP screen.",
"   -g *     List directories FIRST.",
"   -h *     List files horizontally.",
"   -f *     List files only (No directories).",
"   -, *     Dir Tree: show only one level of subdirectories.",
" ",
"   -b       Batch mode;  files listed in one column.",
"            (This format can be redirected to a batch file)",
"   [\"string\"  specifies a string BEFORE each filename (Batch mode)",
"   ]\"string\"  specifies a string AFTER  each filename (Batch mode)",
" ",
"NOTE: items with a * after the flag are TOGGLES",
" ",
NULL } ;

