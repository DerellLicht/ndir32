/******************************************************************/
/*  qualify() modifies a string as required to generate a         */
/*  "fully-qualified" filename, which is a filename that          */
/*  complete drive specifier and path name.                       */
/*                                                                */
/*  input:  argptr: the input filename.                           */
/*                                                                */
/*  output: qresult, a bit-mapped unsigned int with the           */
/*                   following definitions:                       */
/*                                                                */
/*          bit 0 == 1 if wildcards are present.                  */
/*          bit 1 == 1 if no wildcards and path does not exist.   */
/*          bit 2 == 1 if no wildcards and path exists as a file. */
/*          bit 7 == 1 if specified drive is invalid.             */
/*                                                                */
/******************************************************************/

// #ifdef _WIN32_IE >= 0x0600
// c:\mingw\include\shlwapi.h  72  Error 87: expression too complicated for #ifdef or #ifndef
//lint -e87   expression too complicated for #ifdef or #ifndef  (rejecting >= )

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>             //  _getdrive()
#include <sys/stat.h>
//  lint says I don't need this header, and in fact for MSVC6.0
//  I *don't* need it, but for gcc I do...
#include <ctype.h>              //  tolower()
#include <shlwapi.h>
#include <limits.h>
#include <tchar.h>

#include "qualify.h"

#define  LOOP_FOREVER   true

static TCHAR path[PATH_MAX];

/******************************************************************/
unsigned qualify (TCHAR *argptr)
{
   TCHAR *pathptr = &path[0];
   TCHAR *strptr, *srchptr ;
   DWORD plen;
   int result ;
   struct _stat my_stat ;
   unsigned len, qresult = 0;

   //******************************************************
   //  first, determine requested drive number,            
   //  in "A: = 1" format.                                 
   //******************************************************
   //  if arg len == 0 or arg is "."
   if (_tcslen (argptr) == 0 || (_tcslen (argptr) == 1 && *argptr == '.')
      ) {                       /*  no arguments given  */
      // printf("args=none or dot\n") ;         
      int drive = _getdrive ();     //  1 = A:
      //  see if we have a UNC drive...
      if (drive == 0) {
         GetCurrentDirectory (250, pathptr); //lint !e534
         _tcscat (pathptr, _T("\\*"));
         goto exit_point;
      }
   }
   //  05/26/25  These were shown unused, by clang-tidy
   //   else if arg == "x:"
//    else if (*(argptr + 1) == ':') { /*  a drive spec was provided  */
//       // printf("args=colon\n") ;      
//       TCHAR tempchar;
//       tempchar = *argptr;
//       drive = tolower (tempchar) - '`';   //  char - ('a' - 1)
//    }
//    //  else anything else
//    else {                       /*  a path with no drive spec was provided  */
//       // printf("args=no drive\n") ;      
//       drive = _getdrive ();     //  1 = A:
//    }

   //******************************************************
   //  strings in quotes will also foil the DOS routines;
   //  strip out all double quotes
   //******************************************************
   strptr = argptr;
   while (LOOP_FOREVER) {
      srchptr = _tcschr (strptr, '"');
      if (srchptr == 0)
         break;
      _tcscpy (srchptr, srchptr + 1);
      strptr = ++srchptr;
   }

   //******************************************************
   //  get expanded path (this doesn't support UNC)
   //******************************************************
   plen = GetFullPathName (argptr, PATH_MAX, (LPTSTR) pathptr, NULL);
   if (plen == 0)
      return QUAL_INV_DRIVE;

   len = _tcslen (pathptr);
   if (len == 3) {
      _tcscat (pathptr, _T("*"));
      qresult |= QUAL_WILDCARDS;
   }
   else {
      //  see if there are wildcards in argument.
      //  If not, see whether path is a directory or a file,
      //  or nothing.  If directory, append wildcard char
      if (_tcspbrk (pathptr, _T("*?")) == NULL) {
         if (*(pathptr + len - 1) == '\\') {
            len--;
            *(pathptr + len) = 0;
         }

         //  see what kind of file was requested
         //  FindFirstFile doesn't work with UNC paths,
         //  stat() doesn't either
         // handle = FindFirstFile (pathptr, &fffdata);
         if (PathIsUNC(pathptr)) {
            if (PathIsDirectory(pathptr)) {
               _tcscpy (pathptr + len, _T("\\*"));   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            } else if (PathFileExists(pathptr)) {
               qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
            } else {
               _tcscpy (pathptr + len, _T("\\*"));   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            }
         } 
         //  process drive-oriented (non-UNC) paths
         else {
            result = _tstat(pathptr, &my_stat) ;
            if (result != 0) {
               qresult |= QUAL_INV_DRIVE; //  path does not exist.
            } else if (my_stat.st_mode & S_IFDIR) {
               _tcscpy (pathptr + len, _T("\\*"));   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            } else {
               qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
            }
         }

         // handle = FindFirstFile (pathptr, &fffdata);
         // if (handle == INVALID_HANDLE_VALUE)
         //   qresult |= QUAL_INV_DRIVE; //  path does not exist.
         // else {
         //   // if (fffdata.attrib & _A_SUBDIR)
         //   if (fffdata.dwFileAttributes & _A_SUBDIR) {
         //      _tcscpy (pathptr + len, "\\*");   //lint !e669  possible overrun
         //      qresult |= QUAL_WILDCARDS; //  wildcards are present.
         //   }
         //   else
         //      qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
         // 
         //   FindClose (handle);
         // }
      }
   }

   //**********************************************
   //  copy string back to input, then return
   //**********************************************
 exit_point:
   _tcscpy (argptr, pathptr);
// printf("found: [%s]\n", pathptr) ;
// getchar() ;
   return (qresult); //lint !e438  drive
}
