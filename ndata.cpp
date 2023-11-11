//*****************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  NDATA.CPP - NDIR global data declarations                      
//                                                                 
//  Written by:  Daniel D. Miller  (the Derelict)                  
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
   0, //  switch to EGA43/VGA50 line mode (deprecated 01.17.23)
   0, //  display disk_drive summary
   0, //  file-date option (MSDOS 7.00+ only)
   0, //  1 = force old DOS 8.3 filenames in MSDOS 7.00+
   0, //  0=normal tree, 1=show limited level of children
   0, //  uchar long_attr: show full attribute as hex data
   0  //  0=bytes/normal, 1=KB, 2=MB
};

uchar dtree_colors[MAX_DIR_ENTRY] ;

unsigned tree_level_limit = 0 ;

//*****************************************************************
//              END OF CONFIGURATION VARIABLES
//*****************************************************************

WIN32_FIND_DATA fdata ; //  long-filename file struct

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

unsigned start, finish ;
char leftstr[37], rightstr[37] ; //  used by batch mode
char tempstr[MAXLINE] ;
uchar tline, bline, vline, xline, dline ;
unsigned filecount = 0 ;
unsigned columns ;           //  number of display columns on screen

