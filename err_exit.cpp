//*********************************************************
//  ERR_EXIT.CPP -  A standard error-handler.              
//                                                         
//  Written By:   Derell Licht                         
//                                                         
//*********************************************************
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"

//lint -esym(769, error_values::OUT_OF_MEMORY)   global enumeration constant not referenced

static TCHAR const * const error_text[4] = {
   _T("Type 'NDIR /?' for help"), //  not actually used in this program
   _T("Out of memory"),
   _T("Invalid drive or path specified"),
   0 } ;

//*********************************************************
void error_exit(int index, TCHAR* outstr)
{
   if (index == DATA_OKAY) {
      ;
   }
   else if (outstr == NULL)
      syslog(_T("%s\n"), error_text[index]) ;
   else
      syslog(_T("%s: %s\n"), outstr, error_text[index]) ;

   restore_console_attribs();

   exit(index) ;
}

