//*************************************************************************
//  Copyright (c) 1998-2025 Daniel D. Miller                       
//  NDIR32.CPP - The Ultimate directory program (32-bit).          
//                                                                 
//  Written by:   Daniel D. Miller  (the derelict)                 
//                                                                 
//  compile with  makefile                                         
//                                                                 
//*************************************************************************
//  DAN:  Please update revisions.txt with each change.            
//*************************************************************************

#include <windows.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>     //  getenv(), exit()
#include <ctype.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "qualify.h"

#define  VER_NUMBER "2.64"

//lint -esym(843, Version, ShortVersion) could be declared as const
TCHAR *Version = _T(" NDIR.EXE, Version " VER_NUMBER " ") ;
TCHAR *ShortVersion = _T(" NDIR " VER_NUMBER " ") ;

TCHAR ininame[PATH_MAX] = _T("");

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' (line 134, file Ndir32.cpp) could be made static
//lint -e714  Symbol '_CRT_glob' (line 134, file Ndir32.cpp) not referenced
int _CRT_glob = 0 ;

static unsigned tcount = 0 ;   //  number of target filespecs

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
   // 0, //  1 = force old DOS 8.3 filenames in MSDOS 7.00+ (removed in V2.62)
   0, //  0=normal tree, 1=show limited level of children
   0, //  uchar long_attr: show full attribute as hex data
   0  //  0=bytes/normal, 1=KB, 2=MB
};

uchar dtree_colors[MAX_DIR_ENTRY] ;

unsigned tree_level_limit = 0 ;

//*****************************************************************
//              END OF CONFIGURATION VARIABLES
//*****************************************************************

ULONGLONG diskbytes, diskfree, clbytes ;

//*****************************************************************
ffdata *ftop = NULL, *ftail ;

TCHAR* target[20] ;
TCHAR volume_name[PATH_MAX] ;

//  name of drive+path without filenames
TCHAR base_path[PATH_MAX] ;
unsigned base_len ;  //  length of base_path

unsigned start, finish ;
TCHAR leftstr[37], rightstr[37] ; //  used by batch mode
TCHAR tempstr[MAXLINE] ;
uchar tline, bline, vline, xline, dline ;
unsigned filecount = 0 ;
unsigned columns ;           //  number of display columns on screen

//*********************************************************
//  NDIR information screen
//*********************************************************
static TCHAR *idtxt[] = {
   _T(" "),
   _T("Copyright 1990, 1993-2023 by:"),
   _T(" "),
   _T("          Daniel D. Miller"),
   _T("          4835 Coco Palm Drive"),
   _T("          Fremont, CA  94538"),
   _T(" "),
   _T("          Email:    derelict@comcast.net"),
   _T("          Website:  home.comcast.net/~derelict"),
   _T(" "),
   _T("This program, NDIR.EXE, and its associated files, are hereby released as"),
   _T("Freeware, though I retain the copyrights on them.  Please feel free to"),
   _T("distribute copies to anyone who is (or might be) interested in them."),
   NULL } ;

//*********************************************************
//  NDIR help screen
//*********************************************************
static TCHAR *helptxt[] = {
_T(" "),
_T("USAGE:  NDIR <filespecs> -options or /options !<exclusions>"),
_T(" "),
_T(" OPTIONS                      FUNCTIONS"),
_T("   -a *     List ALL files (hidden, system, read-only, etc.)."),
_T("   -a1      List attributes in HEX format."),
_T("   -c *     Clear screen before listing."),
_T("   -p *     Pause on full screen."),
_T("   -m *     Minimize size of header and footer."),
_T("   -w *     Use special colors for system/hidden/readonly files."),
_T("   -d       dir TREE: normal size display (work with -s, -n (default), -r)."),
_T("   -d2      dir TREE: file/directory counts"),
_T("   -d3      dir TREE: mixed size and file/directory counts"),
_T("   -e       Sort by extension."),
_T("   -n        \"   by name."),
_T("   -s        \"   by file size, smallest first."),
_T("   -t        \"   by Date, oldest first."),
_T("   -z        \"   by DOS order (no sort)."),
_T("   -S0      Show sizes in mixed bytes/KB"),
_T("   -S1      Show sizes in Kilobytes"),
_T("   -S2      Show sizes in Megabytes"),
_T("   -r *     Reverse normal sort order."),
_T("   -1       Display one column,   with name/size/date/attr."),
_T("   -2          \"    two   \"   ,   with name/size/date."),
_T("   -4          \"    four  \"   ,   with name/size."),
_T("   -6          \"    six   \"   ,   with name only."),
_T("   -i       Display drive summary for all drives in system."),
_T("   -ii      Display drive summary for all drives in system, with used vs free space."),
_T("   -l *     Toggle long-filename enable flag (removed in V2.62)"),
_T("   -k *     Toggle color mode."),
_T("   -j *     Use standard ASCII (for redirection). (forces -k)"),
_T("   -u *     List filenames in UPPERCASE."),
_T("   -oN      Date/Time display: 0=Last Write, 1=Last Access, 2=File Created"),
_T("   -x *     List executables only (.EXE,.COM,.BAT)."),
_T("   -v       Display registration/update information."),
_T("   -?       Display HELP screen."),
_T("   -g *     List directories FIRST."),
_T("   -h *     List files horizontally."),
_T("   -f *     List files only (No directories)."),
_T("   -q *     XTDIR mode - list files by extension."),
_T("   -, *     Dir Tree: show only L level of subdirectories."),
_T("               L is incremented for each additional comma"),
_T(" "),
_T("   -b       Batch mode;  files listed in one column."),
_T("            (This format can be redirected to a batch file)"),
_T("   [\"string\"  specifies a string BEFORE each filename (Batch mode)"),
_T("   ]\"string\"  specifies a string AFTER  each filename (Batch mode)"),
_T(" "),
_T("NOTE: items with a * after the flag are TOGGLES"),
_T(" "),
NULL } ;

//***********************************************************
//  DEBUG function: insert filespecs in display list
//***********************************************************
void insert_target_filespec(TCHAR *fstr)
{
   // target[tcount] = new char[PATH_MAX] ;
   target[tcount] = (TCHAR *) malloc(PATH_MAX * sizeof(TCHAR)) ;
   if (target[tcount] == NULL) {
      error_exit(OUT_OF_MEMORY, NULL) ;
   }
   _tcscpy(target[tcount], fstr) ;

   unsigned result = qualify(target[tcount]) ;
   if ((result & QUAL_INV_DRIVE) != 0) {
      error_exit(INV_DRIVE, target[tcount]) ;
   }

   tcount++ ;
}

/**********************************************************************/
/**                     File listing routines                        **/        
/**********************************************************************/
static TCHAR fi_name[PATH_MAX], fi_ext[PATH_MAX] ;
static TCHAR fj_name[PATH_MAX], fj_ext[PATH_MAX] ;

static void process_filespecs(void)
{
   TCHAR * strptr ;
   unsigned i, j, k ;

   /***********************************************************************/
   /*************************  loop on filespecs  *************************/
   /***********************************************************************/

   //***********************************************************************
   //  when tree listing is selected, it is assumed that each specified
   //  argument is a separate path, and that no wildcards nor specific 
   //  filenames were provided.
   //  If such anomalies are presented, unpredictable results will occur.
   //***********************************************************************
   if (n.tree == 1  ||  n.tree == 4  ||  n.tree == 5) {
      tree_listing(tcount) ;
   }
   else if (tcount == 1  &&  !n.exec_only) {
      start = finish = 0 ;

      //  in lfn format, convert /3 to /4
      if (columns == 3) {
         columns = 4 ;
      }

      //  Extract base path from first filespec,
      //  and strip off filename
      _tcscpy(base_path, target[start]) ;
      strptr = _tcsrchr(base_path, '\\') ;
      if (strptr != 0) {
          strptr++ ;  //lint !e613  skip past backslash, to filename
         *strptr = 0 ;  //  strip off filename
      }
      base_len = _tcslen(base_path) ;

      //**************************************************
      get_disk_info(base_path) ;

      //**************************************************
      //  initialize file pointer and filecount,
      //  in case of multiple filespecs.
      //**************************************************
      if (ftop != NULL)
         {
         ffdata *ftemp = ftop ;
         ffdata *fkill ;
         while (ftemp != NULL)
            {
            fkill = ftemp ;
            ftemp = ftemp->next ;
            free(fkill) ;
            }
         ftop = NULL ;
         }
      filecount = 0 ;

      //**************************************************
      //  Call directory_tree or file_listing routines,
      //  as specified by flags.
      //**************************************************
      file_listing() ;
      }
   else {
      int temp_columns = columns ;

      start = 0 ;
      while (LOOP_FOREVER) {
         //  Extract base path from first filespec,
         //  and strip off filename
         j = start ;

         //  in lfn format, convert /3 to /4
         if (columns == 3) {
            columns = 4 ;
         }

         //  Extract base path from first filespec,
         //  and strip off filename
         _tcscpy(base_path, target[start]) ;
         //lint -esym(613,strptr) 
         strptr = _tcsrchr(base_path, '\\') ;
         if (strptr != 0) {
            strptr++ ;  // skip past backslash, to filename
            *strptr = 0 ;  //  strip off filename
         }
         base_len = _tcslen(base_path) ;

         //**************************************************
         get_disk_info(base_path) ;

         //  seek out all other filespecs with same path
         j++ ;
         while (LOOP_FOREVER) {
            if (j >= tcount) {
               finish = j-1 ;
               break;
            }
            else {
               //  strip filename from next argument
               _tcscpy(tempstr, target[j]) ;
               strptr = _tcsrchr(tempstr, '\\') ;   //lint !e613
               strptr++ ;
               *strptr = 0 ;

               //  now see if they are equal
               if (_tcscmp(base_path, tempstr) != 0) {
                  finish = j-1 ;
                  break;
               }
               else 
                  j++ ;
            }
         }

         //********************************************************
         //  DELETE DUPLICATE FILESPECS from target array.
         //  Delete record if no differences found.   
         //  Compare file and ext separately.         
         //  This routine uses selection sort, because the list
         //  usually only has a couple of items in it.
         //********************************************************
         for (i=start ; i< finish ; i++) {
            for (j=i+1   ; j<=finish ; j++) {
               //  extract filename and extension file target string
               //  to compare for duplicate filespecs.
               _tcscpy(fi_name, &target[i][base_len]) ;
               strptr = _tcsrchr(fi_name, '.') ; //lint !e613
               if (strptr != 0) {
                  *strptr++ = 0 ;   //lint !e613
                  _tcscpy(fi_ext, strptr) ;
               } else {
                  fi_ext[0] = 0 ;
               }

               _tcscpy(fj_name, &target[j][base_len]) ;
               strptr = _tcsrchr(fj_name, '.') ; //lint !e613
               if (strptr != 0) {
                  *strptr++ = 0 ;   //lint !e613
                  _tcscpy(fj_ext, strptr) ;
               } else {
                  fj_ext[0] = 0 ;
               }

               //  Scan file name and extension for equality.
               //  If both filename and extension are equal, delete one.
               if (strcmpiwc(fi_name, fj_name)  &&  strcmpiwc(fi_ext, fj_ext)) {
                  strptr = target[j] ;
                  for (k=j+1; k<tcount; k++)
                      target[k] = target[k+1] ;
                  tcount-- ;
                  finish-- ;
                  j-- ;
                  free(strptr) ; // release allocated struct.
               }
            }
         }  //lint !e850 for loop index variable 'j' whose type category is 'integral' is modified in body of the for loop that began at 'line 206'

         //**************************************************
         //  initialize file pointer and filecount,
         //  in case of multiple filespecs.
         //**************************************************
         if (ftop != NULL) {
            ffdata *ftemp = ftop ;
            ffdata *fkill ;
            while (ftemp != NULL)
               {
               fkill = ftemp ;
               ftemp = ftemp->next ;
               free(fkill) ;
               }
            ftop = NULL ;
         }
         filecount = 0 ;

         //**************************************************
         //  Call directory_tree or file_listing routines,
         //  as specified by flags.
         //**************************************************
         if (n.tree == 1)
            tree_listing(tcount) ;
         else
            file_listing() ;

         start = finish + 1 ;
         if (start >= tcount) 
            break;
         ncrlf() ;

         columns = temp_columns ;
      }  //  while not done
   }  //  if multiple filespecs are present
}

//**************************************************
//  Sort filespecs alphabetically,
//  to group those in one directory.
//**************************************************
static void sort_target_paths(void)
   {
   TCHAR* strptr ;
   unsigned i, j ;

   for (j=0   ; j<tcount-1 ; j++)
   for (i=j+1 ; i<tcount ; i++)
   if (_tcscmp(target[j],target[i]) >  0)
      {
      strptr    = target[i] ;
      target[i] = target[j] ;
      target[j] = strptr ;
      }
   }

//*****************************************************************
#ifdef UNICODE
int wmain(int argc, TCHAR **argv)
#else
int main(int argc, char **argv)
#endif
{
   console_init(Version) ;

   //***********************************************************
   //  Check for NDIR environment variable
   //***********************************************************

   //  get program filename
   int startIdx = 1 ;
   TCHAR exename[PATH_MAX] ;

   //  interesting lessons from WinNT 4.0:
   //  If the OS is WinNT 4.0, and;
   //  If the executable file is located in the current directory,
   //  THEN:
   //    argv[0] does NOT contain the fully-qualified
   //    path of the EXE, it *only* contains the EXE name.
   //    In all other situations, argv[0] is fully qualified!!
   //  
   //  P.S.  While we're here, derive default INI filename also
   // printf("argv0=%s\n", argv[0]) ;
   TCHAR* strptr = _tcsrchr(argv[0], _T('\\')) ;
   //  no path present
   if (strptr == 0) {
      SearchPath(NULL, argv[0], _T(".exe"), PATH_MAX, ininame, NULL) ;
      strptr = _tcsrchr(ininame, _T('\\')) ;
      if (strptr != 0) 
         _tcscpy(strptr, _T("\\ndir.ini")) ;

      _tcscpy(exename, argv[0]) ;
      // ininame[0] = 0 ;  //  ONLY support current location
   }
   else {
      //  pick up INI filename
      _tcscpy(ininame, argv[0]) ;
      strptr = _tcsrchr(ininame, _T('\\')) ;
      if (strptr == 0)
         return 1;
      _tcscpy(strptr, _T("\\ndir.ini")) ;
      
      //  now process exe name for getenv()
      strptr++ ;  //lint !e613:  skip backslash
      _tcscpy(exename, strptr) ;  //lint !e613
      strptr = _tcschr(exename, _T('.')) ;
      if (strptr != 0) {
         *strptr = 0 ;  //  strip the extension
      }
   }

   TCHAR* options = _tgetenv(exename) ; 
   if (options != 0) {
      argv[0] = options ;
      startIdx = 0 ;
   }
   
   //***********************************************************
   //  first read default settings
   //***********************************************************
   read_config_file() ;

   //***********************************************************
   //  override defaults with command line and environment vars
   //***********************************************************
   parse_command_args(startIdx, argc, argv) ;
   verify_flags() ;  //  this may add extensions if -x is given

   //***********************************************************
   //  Execute the requested command
   //***********************************************************
   display_logo() ;

   if (n.help)
      info(helptxt) ;
   else if (n.info)
      info(idtxt) ;
   else if (n.drive_summary > DSUMMARY_NONE)
      display_drive_summary() ;
   else {
      //  If no filespec was given, insert current path with *.*
      if (tcount==0)
         insert_target_filespec(_T(".")) ;

      sort_target_paths() ;      //  LFN: okay
      process_filespecs() ;
   }

   error_exit(DATA_OKAY, NULL) ;
   return 0 ;
}

// #ifdef UNICODE
// int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// {
//   return wmain(_argc, _argv);
// }
// #endif
