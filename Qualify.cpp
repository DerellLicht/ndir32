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
//  compile with:  cl /W3 /Zp1 /c qualify.cpp lfnfcnc.obj           
//                                         or lfnfcns.obj           
//                                         or lfnfcnl.obj           
//******************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>				  //  strlen()
#include <direct.h>				  //  _getdrive()
#include <sys/stat.h>
//  lint says I don't need this header, and in fact for MSVC6.0
//  I *don't* need it, but for gcc I do...
#include <ctype.h>				  //  tolower()
#include <windows.h>
#include <shlwapi.h>
#include <limits.h>
// #include "dmiller.hpp"
// #include "long_fn.hpp"

/***********************  qualify() constants  ************************/
#define  QUAL_WILDCARDS    0x01
#define  QUAL_NO_PATH      0x02
#define  QUAL_IS_FILE      0x04
#define  QUAL_INV_DRIVE    0x80

// int lfn_supported ;
// static char *dname = "c:\\" ;
// static char *dhere = "c:." ;
// static char fsn_bfr[32] ;
static char path[PATH_MAX];

//***************************************************************
// static WIN32_FIND_DATA fffdata;

/******************************************************************/
unsigned qualify (char *argptr, int lfn_off)
{
	char *pathptr = &path[0];
	char *strptr, *srchptr, tempchar;
	DWORD plen;
   int drive, result ;
	// int done ;
   // HANDLE handle;
   struct stat my_stat ;
	unsigned len, qresult = 0;
	// struct _find_t c_file;

	//******************************************************
	//  first, determine requested drive number,            
	//  in "A: = 1" format.                                 
	//******************************************************
	if (strlen (argptr) == 0 || (strlen (argptr) == 1 && *argptr == '.')
		) {							  /*  no arguments given  */
		// printf("args=none or dot\n") ;         
		drive = _getdrive ();	  //  1 = A:
		//  see if we have a UNC drive...
		if (drive == 0) {
			GetCurrentDirectory (250, pathptr);
			strcat (pathptr, "\\*");
			goto exit_point;
		}
	}
	else if (*(argptr + 1) == ':') {	/*  a drive spec was provided  */
		// printf("args=colon\n") ;      
		tempchar = *argptr;
		drive = tolower (tempchar) - '`';	//  char - ('a' - 1)
	}
	else {							  /*  a path with no drive spec was provided  */
		// printf("args=no drive\n") ;      
		drive = _getdrive ();	  //  1 = A:
	}

	//******************************************************
	//  check for validity of long_filename functions
	//******************************************************
	// if (lfn_okay) 
	//    {
	//    *dname = 'a' - 1 + drive ;
	//    lfn_supported = 1 ;
	//       // (is_lfn_available(dname, (char far *) fsn_bfr) != 0) 
	//       // ? 0 : 1 ;
	//    }
	// else 
	//    lfn_supported = 0 ;

	//******************************************************
	//  strings in quotes will also foil the DOS routines;
	//  strip out all double quotes
	//******************************************************
	strptr = argptr;
	while (1) {
		srchptr = strchr (strptr, '"');
		if (srchptr == 0)
			break;
		strcpy (srchptr, srchptr + 1);
		strptr = ++srchptr;
	}

	//******************************************************
	//  get expanded path (this doesn't support UNC)
	//******************************************************
	plen = GetFullPathName (argptr, PATH_MAX, (LPTSTR) pathptr, NULL);
	if (plen == 0)
		return QUAL_INV_DRIVE;

	len = strlen (pathptr);
	if (len == 3) {
		strcat (pathptr, "*");
		qresult |= QUAL_WILDCARDS;
	}
	else {
		//  see if there are wildcards in argument.
		//  If not, see whether path is a directory or a file,
		//  or nothing.  If directory, append wildcard char
		if (strpbrk (pathptr, "*?") == NULL) {
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
               strcpy (pathptr + len, "\\*");   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            } else if (PathFileExists(pathptr)) {
               qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
            } else {
               strcpy (pathptr + len, "\\*");   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            }
         } 
         //  process drive-oriented (non-UNC) paths
         else {
            result = stat(pathptr, &my_stat) ;
            if (result != 0) {
               qresult |= QUAL_INV_DRIVE; //  path does not exist.
            } else if (my_stat.st_mode & S_IFDIR) {
               strcpy (pathptr + len, "\\*");   //lint !e669  possible overrun
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
         //      strcpy (pathptr + len, "\\*");   //lint !e669  possible overrun
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
	strcpy (argptr, pathptr);
// printf("found: [%s]\n", pathptr) ;
// getchar() ;
   return (qresult);
}
