//*********************************************************************************
//  Copyright (c) 2023  Daniel D Miller
//  system.cpp - general-purpose debug functions
//  
//  The syslog() function outputs to OutputDebugStringA();
//  those messages can be monitored using the free DebugView utility,
//  available from SysInternals (owned by Microsoft)
//*********************************************************************************

#include <windows.h>
#include <stdio.h>
#include <ctype.h>              //  tolower()

#include "common.h"
#include "ndir32.h"

//lint -esym(759,get_system_message) -esym(765,get_system_message) -esym(714,get_system_message)

//*************************************************************
//  handlers for short-filename conversion
//*************************************************************
static char *tail_ptr = 0;
static char lfn_src[PATH_MAX + 1];
static char lfn_dest[PATH_MAX + 1];

void save_sfn_base_path(char *sfn_base_path)
{
   //  we need the whole path before we can get short filename
   // syslog("[%s]\n", sfn_base_path);
   strcpy (lfn_src, sfn_base_path);
   tail_ptr = strrchr (lfn_src, '\\');  //  strip off wildcards or target name
   if (tail_ptr != 0) {
      *(++tail_ptr) = 0;
   }
   else {
      syslog("no backslash found\n");
   }
}

char *sfn_convert_filename(char *lfn_filename)
{
   // syslog("<%s>\n", lfn_filename);
   strcpy (tail_ptr, (char *) lfn_filename); //  append filename to lfn path
   // strcpy(ftemp->filename, (char *) fdata.cAlternateFileName) ;
   // syslog("{%s}\n", lfn_src);
   int result = GetShortPathName (lfn_src, lfn_dest, sizeof(lfn_dest));
   if (result == 0) {
      return (char *) "no_path" ;   //lint !e1773
   }
   char *strptr = strrchr (lfn_dest, '\\');
   if (strptr == 0) {
      return (char *) "No_strrchr" ;   //lint !e1773
   }
   strptr++;
   *tail_ptr = 0 ;   //  restore base length
   return strptr ;
}

//*************************************************************
//  each subsequent call to this function overwrites 
//  the previous report.
//*************************************************************
char *get_system_message(void)
{
   static char msg[261] ;
   int slen ;

   LPVOID lpMsgBuf;
   FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   strncpy(msg, (char *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = strlen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 10) {
      msg[slen-1] = 0 ;
   }

   return msg;
}

char *get_system_message(DWORD errcode)
{
   static char msg[261] ;
   int slen ;

   LPVOID lpMsgBuf;
   FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      errcode,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   strncpy(msg, (char *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = strlen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 10) {
      msg[slen-1] = 0 ;
   }

   return msg;
}

//********************************************************************
//  On Windows platform, try to redefine printf/fprintf
//  so we can output code to a debug window.
//  Also, shadow syslog() within OutputDebugStringA()
//  Note: printf() remapping was unreliable,
//  but syslog worked great.
//********************************************************************
//lint -esym(714, syslog)
//lint -esym(759, syslog)
//lint -esym(765, syslog)
int syslog(const char *fmt, ...)
{
   char consoleBuffer[3000] ;
   va_list al; //lint !e522

//lint -esym(526, __builtin_va_start)
//lint -esym(628, __builtin_va_start)
   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   // if (common_logging_enabled)
   //    fprintf(cmlogfd, "%s", consoleBuffer) ;
   OutputDebugStringA(consoleBuffer) ;
   va_end(al);
   return 1;
}

//*****************************************************************************
// ULLONG_MAX = 18,446,744,073,709,551,615
//*****************************************************************************
char *convert_to_commas(ULONGLONG uli, char *outstr)
{  //lint !e1066
   int slen, inIdx, j ;
   char *strptr ;
   char temp_ull_str[MAX_ULL_COMMA_LEN+1] ;
   static char local_ull_str[MAX_ULL_COMMA_LEN+1] ;
   if (outstr == NULL) {
       outstr = local_ull_str ;
   }

   // sprintf(temp_ull_str, "%"PRIu64"", uli);
   // sprintf(temp_ull_str, "%llu", uli);
   sprintf(temp_ull_str, "%I64u", uli);
   // _ui64toa(uli, temp_ull_str, 10) ;
   slen = strlen(temp_ull_str) ;
   inIdx = --slen ;//  convert byte-count to string index 

   //  put NULL at end of output string
   strptr = outstr + MAX_ULL_COMMA_LEN ;
   *strptr-- = 0 ;   //  make sure there's a NULL-terminator

   for (j=0; j<slen; j++) {
      *strptr-- = temp_ull_str[inIdx--] ;
      if ((j+1) % 3 == 0)
         *strptr-- = ',' ;
   }
   *strptr = temp_ull_str[inIdx] ;

   //  copy string from tail-aligned to head-aligned
   strcpy(outstr, strptr) ;
   return outstr ;
}

//**************************************************************
//  string compare routine, case-insensitive, 
//  wildcards are handled in the DOS fashion.
//**************************************************************
int strcmpiwc(const char *onestr, const char *twostr)
{
   char onechar, twochar ;
   int k = 0 ;

   while (LOOP_FOREVER) {
      onechar = *(onestr+k) ;
      twochar = *(twostr+k) ;

      //  if both are at end of string and no differences
      //  have been found, the strings are equal.
      if (onechar == 0  &&  twochar == 0) 
         return 1;

      //  if one string is at end and the other is not,
      //  there is NOT a match.
      if (onechar == 0  ||  twochar == 0)
         return 0;

      //  at this point, neither char is NULL

      //  if either char is a 'match all' wildcard, the strings are equal
      if (onechar == '*'  ||  twochar == '*') 
         return 1 ;

      if (onechar == '?'  ||  twochar == '?') ; //  match continues


      else if (tolower(onechar) != tolower(twochar)) 
         return 0;

      k++ ;
   }
}



