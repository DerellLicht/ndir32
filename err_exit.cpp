//*********************************************************
//  ERR_EXIT.CPP -  A standard error-handler.              
//                                                         
//  Written By:   Daniel D. Miller                         
//                                                         
//*********************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"

static char const * const error_text[4] = {
   "Type 'NDIR /?' for help", //  not actually used in this program
   "Out of memory",
   "Invalid drive or path specified",
   0 } ;

//*********************************************************
void error_exit(int index, char* outstr)
{
   if (index == DATA_OKAY)
      ;
   else if (outstr == NULL)
      syslog("%s\n", error_text[index]) ;
   else
      syslog ("%s: %s\n", outstr, error_text[index]) ;

   restore_console_attribs();

   exit(index) ;
}

