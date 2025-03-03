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
#include <tchar.h>

#include "common.h"
#include "ndir32.h"

//lint -esym(759,get_system_message) -esym(765,get_system_message) -esym(714,get_system_message)

//*************************************************************
//  handlers for short-filename conversion
//*************************************************************
// static char *tail_ptr = 0;
// static char lfn_src[PATH_MAX + 1];
// static char lfn_dest[PATH_MAX + 1];

// void save_sfn_base_path(char *sfn_base_path)
// {
//    //  we need the whole path before we can get short filename
//    // syslog("[%s]\n", sfn_base_path);
//    _tcscpy (lfn_src, sfn_base_path);
//    tail_ptr = _tcsrchr (lfn_src, '\\');  //  strip off wildcards or target name
//    if (tail_ptr != 0) {
//       *(++tail_ptr) = 0;
//    }
//    else {
//       syslog("no backslash found\n");
//    }
// }

// char *sfn_convert_filename(char *lfn_filename)
// {
//    // syslog("<%s>\n", lfn_filename);
//    _tcscpy (tail_ptr, (char *) lfn_filename); //  append filename to lfn path
//    // _tcscpy(ftemp->filename, (char *) fdata.cAlternateFileName) ;
//    // syslog("{%s}\n", lfn_src);
//    int result = GetShortPathName (lfn_src, lfn_dest, sizeof(lfn_dest));
//    if (result == 0) {
//       return (char *) "no_path" ;   //lint !e1773
//    }
//    char *strptr = _tcsrchr (lfn_dest, '\\');
//    if (strptr == 0) {
//       return (char *) "No__tcsrchr" ;   //lint !e1773
//    }
//    strptr++;
//    *tail_ptr = 0 ;   //  restore base length
//    return strptr ;
// }

//**********************************************************
//lint -esym(757, isUpperAscii)  // global declarator not referenced
bool isUpperAscii(TCHAR *outstr, uint slen)
{
   uint idx ;
   for (idx=0; idx<slen; idx++) {
      if (*outstr > 0x7F) {   //lint !e685
         return true ;
      }
      outstr++ ;
   }
   return false ;
}

//**********************************************************************
//  Modify this to build entire string and print once.
//  This command has several forms:
//
//  - the basic form has too many arguments!!
//    bfr - data buffer to display
//    bytes - number of bytes (of bfr) to display
//    addr  - base address to display at beginning of line.
//            This helps with repeated calls to this function.
//    mode  - 0=output to printf, 1=output to syslog
//
//  - The other two forms take only buffer and length args,
//    and implicitly print to either printf or syslog.
//**********************************************************************
//lint -esym(714, hex_dump)
//lint -esym(759, hex_dump)
//lint -esym(765, hex_dump)
int hex_dump(u8 *bfr, int bytes, unsigned addr)
{
   int j, len ;
   char tail[40] ;
   char pstr[81] ;
   static bool hex_dump_active = false ;
   if (hex_dump_active)
      return 0;
   hex_dump_active = true ;

   tail[0] = 0 ;
   int idx = 0 ;
   int plen = 0 ;
   while (1) {
      int leftovers = bytes - idx ;
      if (leftovers > 16)
          leftovers = 16 ;

      plen = wsprintfA(pstr, "%05X:  ", addr+idx) ;  //lint !e737
      len = 0 ;
      for (j=0; j<leftovers; j++) {
         u8 chr = bfr[idx+j] ;
         plen += wsprintfA(&pstr[plen], "%02X ", chr) ;
         if (chr < 32) {
            len += wsprintfA(tail+len, ".") ;
         } else if (chr < 127) {
            len += wsprintfA(tail+len, "%c", chr) ;
         } else {
            len += wsprintfA(tail+len, "?") ;
         }
      }
      //  last, print fill spaces
      for (; j<16; j++) {
         plen += wsprintfA(&pstr[plen], "   ") ;
         len += wsprintfA(tail+len, " ") ;
      }

      // printf(" | %s |\n", tail) ;
      strcat(pstr, " | ") ;
      strcat(pstr, tail) ;
      strcat(pstr, " |") ;
      // printf("%s\n", pstr) ;
      syslog(_T("%s\n"), pstr) ;

      idx += leftovers ;
      if (idx >= bytes)
         break;
   }
   hex_dump_active = false ;
   return 0;
}  //lint !e818 

//**************************************************************************
int hex_dump(u8 *bfr, int bytes)
{
   return hex_dump(bfr, bytes, 0) ;
}

//*************************************************************
//  each subsequent call to this function overwrites 
//  the previous report.
//*************************************************************
TCHAR *get_system_message(void)
{
   static TCHAR msg[261] ;
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
   _tcsncpy(msg, (TCHAR *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = _tcslen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 10) {
      msg[slen-1] = 0 ;
   }

   return msg;
}

TCHAR *get_system_message(DWORD errcode)
{
   static TCHAR msg[261] ;
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
   _tcsncpy(msg, (TCHAR *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = _tcslen(msg) ;
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
int syslog(const TCHAR *fmt, ...)
{
   TCHAR consoleBuffer[3000] ;
   va_list al; //lint !e522

//lint -esym(526, __builtin_va_start)
//lint -esym(628, __builtin_va_start)
   va_start(al, fmt);   //lint !e1055 !e530
   _vstprintf(consoleBuffer, fmt, al);   //lint !e64
   // if (common_logging_enabled)
   //    fprintf(cmlogfd, "%s", consoleBuffer) ;
   OutputDebugString(consoleBuffer) ;
   va_end(al);
   return 1;
}

//*****************************************************************************
// ULLONG_MAX = 18,446,744,073,709,551,615
//*****************************************************************************
TCHAR *convert_to_commas(ULONGLONG uli, TCHAR *outstr)
{  //lint !e1066
   int slen, inIdx, j ;
   TCHAR *strptr ;
   TCHAR temp_ull_str[MAX_ULL_COMMA_LEN+1] ;
   static TCHAR local_ull_str[MAX_ULL_COMMA_LEN+1] ;
   if (outstr == NULL) {
       outstr = local_ull_str ;
   }

   // sprintf(temp_ull_str, "%"PRIu64"", uli);
   // sprintf(temp_ull_str, "%llu", uli);
   _stprintf(temp_ull_str, _T("%I64u"), uli);
   // _ui64toa(uli, temp_ull_str, 10) ;
   slen = _tcslen(temp_ull_str) ;
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
   _tcscpy(outstr, strptr) ;
   return outstr ;
}

//**************************************************************
//  string compare routine, case-insensitive, 
//  wildcards are handled in the DOS fashion.
//**************************************************************
int strcmpiwc(const TCHAR *onestr, const TCHAR *twostr)
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



