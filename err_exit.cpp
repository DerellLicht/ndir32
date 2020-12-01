//*********************************************************
//  ERR_EXIT.CPP -  A standard error-handler.              
//                                                         
//  Written By:   Daniel D. Miller                         
//                                                         
//  Last Update:  01/21/03 09:33                           
//                                                         
//  compile with makefile                                  
//                                                         
//*********************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "ndir32.h"
#include "conio32.hpp"

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
      printf("%s\n", error_text[index]) ;
   else
      printf("%s: %s\n", outstr, error_text[index]) ;

   restore_console_attribs();

   exit(index) ;
   }

