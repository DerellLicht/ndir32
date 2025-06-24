//*************************************************************************
//  Copyright (c) 1998-2025 Derell Licht                       
//  NDIR32.CPP - The Ultimate directory program (32-bit).          
//*************************************************************************
//  DAN:  Please update revisions.txt with each change.            
//*************************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>     //  getenv(), exit()
#include <ctype.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "qualify.h" //  must be *after* vector_res.h
#include "treelist.h"

//lint -esym(864, target)  Expression involving variable possibly depends on order of evaluation

#define  VER_NUMBER "2.68"

//lint -esym(843, Version, ShortVersion) could be declared as const
TCHAR *Version = _T(" NDIR.EXE, Version " VER_NUMBER " ") ;
TCHAR *ShortVersion = _T(" NDIR " VER_NUMBER " ") ;

TCHAR ininame[MAX_PATH_LEN] = _T("");

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' (line 134, file Ndir32.cpp) could be made static
//lint -e714  Symbol '_CRT_glob' (line 134, file Ndir32.cpp) not referenced
int _CRT_glob = 0 ;

//*****************************************************************
//                  CONFIGURATION VARIABLES (new style)
//*****************************************************************
ndir_data n ;

uchar dtree_colors[MAX_DIR_ENTRY] ;

unsigned tree_level_limit = 0 ;

//*****************************************************************
//              END OF CONFIGURATION VARIABLES
//*****************************************************************

ULONGLONG diskbytes, diskfree, clbytes ;

// TCHAR* target[20] ;
std::vector<std::wstring> target {};

// TCHAR volume_name[MAX_PATH_LEN] ;
std::wstring volume_name {};

//  name of drive+path without filenames
TCHAR base_path[MAX_PATH_LEN] ;
unsigned base_len ;  //  length of base_path

unsigned start, finish ;
TCHAR tempstr[MAXLINE] ;
TCHAR tline, bline, vline, dline ;
unsigned filecount = 0 ;
unsigned columns ;           //  number of display columns on screen

//*********************************************************
//  NDIR information screen
//*********************************************************
static TCHAR *idtxt[] = {
   _T(" "),
   _T("Copyright 1990, 1993-2025 by:"),
   _T(" "),
   _T("   Derell Licht"),
   _T("   Email:    derelict@comcast.net"),
   _T("   Website:  https://derelllicht.42web.io/index.htm"),
   _T(" "),
   _T("This program, NDIR.EXE, and its associated files, are distributed under"),
   _T("Creative Commons CC0 1.0 Universal"),
   _T("https://creativecommons.org/publicdomain/zero/1.0/"),
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
_T("   -k *     Toggle color mode."),
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

//**************************************************
static void info (TCHAR *data[])
{
   unsigned j = 0;

   while (data[j] != NULL) {
      dputsf(L"%s\n", data[j++]);
   }
}

//**************************************************
// #define  USE_SYSLOG
//lint -esym(714, dump_target)
//lint -esym(759, dump_target)
//lint -esym(765, dump_target)
void dump_target(TCHAR *msg)
{
   if (msg != NULL) {
#ifdef  USE_SYSLOG   
      syslog(_T("%s"), msg);
#else
      dputsf(_T("%s\n"), msg);
#endif      
   }
   for(auto &tgt : target) {
      auto telement = tgt ;
#ifdef  USE_SYSLOG   
      syslog(L"%s\n", telement.c_str());
#else      
      dputsf(L"%s\n", telement.c_str());
#endif      
   }
}  //lint !e529

//**************************************************************
//  string compare routine, case-insensitive, 
//  wildcards are handled in the DOS fashion.
//**************************************************************
bool strcmpiwc(const TCHAR *onestr, const TCHAR *twostr)
{
   char onechar, twochar ;
   int k = 0 ;

   while (LOOP_FOREVER) {
      onechar = *(onestr+k) ;
      twochar = *(twostr+k) ;

      //  if both are at end of string and no differences
      //  have been found, the strings are equal.
      if (onechar == 0  &&  twochar == 0) 
         return true;

      //  if one string is at end and the other is not,
      //  there is NOT a match.
      if (onechar == 0  ||  twochar == 0)
         return false;

      //  at this point, neither char is NULL

      //  if either char is a 'match all' wildcard, the strings are equal
      if (onechar == '*'  ||  twochar == '*') 
         return true ;

      if (onechar == '?'  ||  twochar == '?') ; //  match continues


      else if (tolower(onechar) != tolower(twochar)) 
         return false;

      k++ ;
   }
}

//***********************************************************
//  DEBUG function: insert filespecs in display list
//***********************************************************
void insert_target_filespec(TCHAR *fstr)
{
   target.emplace_back(fstr);
   uint idx = target.size() - 1 ;

   unsigned result = qualify(target[idx]) ;
   if ((result & (QUAL_INV_DRIVE | QUAL_NO_PATH)) != 0) {
      syslog(_T("qualify failed: %s\n"), target[idx].c_str());
      error_exit(INV_DRIVE, (TCHAR *)target[idx].c_str()) ;
   }
}

//********************************************************
//  DELETE DUPLICATE FILESPECS from target array.
//  Delete record if no differences found.   
//  Compare file and ext separately.         
//  This routine uses selection sort, because the list
//  usually only has a couple of items in it.
//********************************************************
static void check_for_duplicate_targets(void)
{  
   unsigned idxHead, idxTail ;
   unsigned ltcount = target.size() ;
   // dump_target(_T("target(s) input\n"));
   //  head index should iterate over all elements *except* the last one,
   //  since the last element in list would not have any others to compare against.
   for (idxHead=0 ; idxHead< (ltcount - 1) ; idxHead++) {
      //  tail index should iterate over all elements after 
      //  the current head index.
      //  Note that the number of elements in the list may vary
      //  as items are deleted by this operation.
      for (idxTail=idxHead+1   ; idxTail < ltcount ; idxTail++) {
try_next_tail:
         //  Scan file name and extension for equality.
         //  If both filename and extension are equal, delete one.
         if (target[idxHead].compare(target[idxTail]) == 0) {
            target.erase(target.begin()+idxTail) ;
            ltcount-- ;
            //  we don't want to increment idxTail after deleting element;
            //  it is now pointing to the next element in array
            if (idxTail < ltcount) {
               goto try_next_tail ;
            }
         }
      }
   }  //lint !e850 for loop index variable 'j' whose type category is 'integral' is modified in body of the for loop that began at 'line 206'
   // dump_target(_T("targets(s) filtered\n"));
   // syslog(_T("target size: %u elements\n"), target.size());
}  
         
/**********************************************************************/
/**                     File listing routines                        **/        
/**********************************************************************/
//lint -esym(745, process_filespecs)   function has no explicit type or class, int assumed
static void process_filespecs(void)
{
   TCHAR * strptr ;
   unsigned j ;

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
      tree_listing(target.size()) ;
   }
   else if (target.size() == 1  &&  !n.exec_only) {
      start = finish = 0 ;

      //  in lfn format, convert /3 to /4
      if (columns == 3) {
         columns = 4 ;
      }

      //  Extract base path from first filespec,
      //  and strip off filename
      _tcscpy(base_path, target[start].c_str()) ;
      strptr = _tcsrchr(base_path, '\\') ;
      if (strptr != 0) {
          strptr++ ;  //lint !e613  skip past backslash, to filename
         *strptr = 0 ;  //  strip off filename
      }
      base_len = _tcslen(base_path) ;

      //**************************************************
      get_disk_info(base_path) ;

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

         //  in lfn format, convert /3 to /4
         if (columns == 3) {
            columns = 4 ;
         }

         //  Extract base path from first filespec,
         //  and strip off filename
         _tcscpy(base_path, target[start].c_str()) ;
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
         //  This block sets variable 'finish'
         j = start ;
         j++ ;
         while (LOOP_FOREVER) {
            // if (j >= tcount) {
            if (j >= target.size()) {
               finish = j-1 ;
               break;
            }
            else {
               //  strip filename from next argument
               _tcscpy(tempstr, target[j].c_str()) ;
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

         check_for_duplicate_targets();

         //**************************************************
         //  delete contents of existing file list
         //**************************************************
         clear_existing_file_list();

         //**************************************************
         file_listing() ;

         //  this block set variable 'start'
         start = finish + 1 ;
         if (start >= target.size()) 
            break;
         ncrlf() ;

         columns = temp_columns ;
      }  //  while not done
   }  //  if multiple filespecs are present
}  //lint !e533


//**************************************************
bool const comp(std::wstring a, std::wstring b)
{
   return (a.compare(b) < 0) ;
}

//**************************************************
//  Sort filespecs alphabetically,
//  to group those in one directory.
//**************************************************
//lint -esym(745, sort_target_paths)  function has no explicit type or class, int assumed
//lint -esym(533, sort_target_paths)  function should return a value
static void sort_target_paths(void)
{
   std::sort(target.begin(), target.end(), comp);
   // dump_target(_T("sorted list\n"));
}

//********************************************************************************
//  this solution is from:
//  https://github.com/coderforlife/mingw-unicode-main/
//********************************************************************************
#if defined(__GNUC__) && defined(_UNICODE)

#ifndef NOMAKEDEPEND
#ifndef __MSVCRT__
#error Unicode main function requires linking to MSVCRT
#endif
#endif

#include <wchar.h>
#include <stdlib.h>

extern int _CRT_glob;
extern 
#ifdef __cplusplus
"C" 
#endif
void __wgetmainargs(int*,wchar_t***,wchar_t***,int,int*);

#ifdef MAIN_USE_ENVP
int wmain(int argc, wchar_t *argv[], wchar_t *envp[]);
#else
int wmain(int argc, wchar_t *argv[]);
#endif

int main() 
{
   wchar_t **enpv, **argv;
   int argc, si = 0;
   __wgetmainargs(&argc, &argv, &enpv, _CRT_glob, &si); // this also creates the global variable __wargv
#ifdef MAIN_USE_ENVP
   return wmain(argc, argv, enpv);
#else
   return wmain(argc, argv);
#endif
}

#endif //defined(__GNUC__) && defined(_UNICODE)

//*****************************************************************
#ifdef UNICODE
int wmain(int argc, WCHAR **argv)
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
   TCHAR exename[MAX_PATH_LEN] ;
   
   // [clang] _WIN32_WINNT: 0x0601
   // [cygwin] _WIN32_WINNT: 0x0A00
   // syslog(_T("_WIN32_WINNT: 0x%04X\n"), _WIN32_WINNT);

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
      SearchPath(NULL, argv[0], _T(".exe"), MAX_PATH_LEN, ininame, NULL) ;
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
   if (n.help) {
      display_logo() ;
      info(helptxt) ;
   }
   else if (n.info) {
      display_logo() ;
      info(idtxt) ;
   }
   else if (n.drive_summary > DSUMMARY_NONE) {
      display_logo() ;
      display_drive_summary() ;
   }
   else {
      //  If no filespec was given, insert current path with *.*
      if (target.size() == 0) {
         insert_target_filespec(_T(".")) ;
      }

      sort_target_paths() ;
      display_logo() ;
      process_filespecs() ;
   }

   error_exit(DATA_OKAY, NULL) ;
   return 0 ;
}
