//***************************************************************************
//  CONIO32.CPP: Template for 32-bit console programs                        
//                                                                           
//  Written by:   Daniel D. Miller                                           
//                                                                           
//  Last Update:  11/07/01 14:44                                             
//                                                                           
//  compile with:    cl /W3 /O2 /G4 conio32.cpp                              
//    NOTE: This program requires Microsoft Visual C++ 4.0 or greater.       
//                                                                           
//***************************************************************************
//  Windows console data structures
//  
// typedef struct _CONSOLE_SCREEN_BUFFER_INFO { // csbi 
//     COORD      dwSize; 
//     COORD      dwCursorPosition; 
//     WORD       wAttributes; 
//     SMALL_RECT srWindow; 
//     COORD      dwMaximumWindowSize; 
// } CONSOLE_SCREEN_BUFFER_INFO ; 
// 
// typedef struct _SMALL_RECT { // srct 
//     SHORT Left; 
//     SHORT Top; 
//     SHORT Right; 
//     SHORT Bottom; 
// } SMALL_RECT; 
// 
// typedef struct _COORD { // coord. 
//     SHORT X;      // horizontal coordinate 
//     SHORT Y;      // vertical coordinate 
// } COORD; 
//***************************************************************************
  
#include <windows.h>
#include <stdio.h>
#include <conio.h>   // _getch(), _kbhit()
#include <stdlib.h>
#include "conio32.hpp"

typedef  unsigned char  uchar ;
typedef  unsigned long  ulong ;

#ifndef false
#define false  0
#endif
#ifndef true
#define true   1
#endif

HANDLE hStdOut, hStdIn ;

static CONSOLE_SCREEN_BUFFER_INFO sinfo ;

static int redirected = 0 ;

static WORD original_attribs = 3 ;

//***************************************************************************
//***************************************************************************
//                GENERIC 32-BIT CONSOLE I/O FUNCTIONS
//***************************************************************************
//***************************************************************************

//**********************************************************
//lint -esym(715,dwCtrlType)
BOOL control_handler(DWORD dwCtrlType)
   {
   BOOL bSuccess;
   DWORD dwMode;

   //  restore the screen mode
   bSuccess = GetConsoleMode(hStdOut, &dwMode);
   PERR(bSuccess, "GetConsoleMode");
   bSuccess = SetConsoleMode(hStdOut, 
      dwMode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT ) ;
   PERR(bSuccess, "SetConsoleMode");

   //  display message and do other work
   return FALSE ;
   }   

//***************************************************************************
int is_redirected(void)
{
   return redirected ;
}         

//***************************************************************************
//  This stores CONSOLE_SCREEN_BUFFER_INFO in global var sinfo
//***************************************************************************
void console_init(char *title)
   {
   BOOL bSuccess;
   DWORD dwMode;

   /* get the standard handles */
   hStdOut = GetStdHandle(STD_OUTPUT_HANDLE); 
	if (hStdOut == INVALID_HANDLE_VALUE) {
      printf("GetStdHandle(STD_OUTPUT_HANDLE): %s\n", get_system_message()) ;
      exit(1) ;
	}
   // PERR(hStdOut != INVALID_HANDLE_VALUE, "GetStdHandle");
   hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdIn == INVALID_HANDLE_VALUE) {
      printf("GetStdHandle(STD_INPUT_HANDLE): %s\n", get_system_message()) ;
      exit(1) ;
	}
   // PERR(hStdIn != INVALID_HANDLE_VALUE, "GetStdHandle");

   //  Put up a meaningful console title.
   //  Will this *always* succeed???
   if (title != 0) {
      bSuccess = SetConsoleTitle(title);
      PERR(bSuccess, "SetConsoleTitle");
   }

   //  get screen information.
   //  If this call fails, assume we are re-directing output.
   //  Unfortunately, this also fails on all Bash-window terminals
   bSuccess = GetConsoleScreenBufferInfo(hStdOut, &sinfo) ;
   // PERR(bSuccess, "GetConsoleScreenBufferInfo");
   if (bSuccess == false) {
      // printf("GetConsoleScreenBufferInfo: %s\n", get_system_message()) ;
      // exit(1) ;
		//  if we cannot get console info for StdOut, 
		//  most likely StdOut is redirected to a file.
		//  Let's see if we can get info for StdIn
   	// bSuccess = GetConsoleScreenBufferInfo(hStdIn, &sinfo) ;
		// if (!bSuccess) {
		// 	printf("darn, can't get info on stdin either...\n") ;
		// }
		//  Nope, that doesn't work either.  I guess I just can't get
		//  console info (in particular, console width) when we're redirecting output.
		
      redirected = 1 ;
      return ; 
   }
   //  on systems without ANSI.SYS, this is apparently 0...
   original_attribs = sinfo.wAttributes ;
   if (original_attribs == 0) {
       original_attribs = 7 ;
   }

// FILE *fp = fopen("duh", "wt") ;
// fprintf(fp, "original attribs=%X\n", original_attribs) ;
// fclose(fp) ;

   /* set up mouse and window input */
   bSuccess = GetConsoleMode(hStdOut, &dwMode);
   PERR(bSuccess, "GetConsoleMode");

   /* when turning off ENABLE_LINE_INPUT, you MUST also turn off */
   /* ENABLE_ECHO_INPUT. */
   // bSuccess = SetConsoleMode(hStdIn, (dwMode & ~(ENABLE_LINE_INPUT |
   //     ENABLE_ECHO_INPUT)) | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
   bSuccess = SetConsoleMode(hStdOut, 
      dwMode & (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)) ;
//      (dwMode & ~(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) )) ;
   PERR(bSuccess, "SetConsoleMode");

   //  set up Ctrl-Break handler
   SetConsoleCtrlHandler((PHANDLER_ROUTINE) control_handler, TRUE) ;
   }   

//**********************************************************
void restore_console_attribs(void)
{
   sinfo.wAttributes = original_attribs ;
   SetConsoleTextAttribute(hStdOut, sinfo.wAttributes) ;
}

/*****************************************************************
* FUNCTION: myGetchar(void)                                      *
*                                                                *
* PURPOSE: get a single character from the standard input handle *
*                                                                *
* INPUT: none                                                    *
*                                                                *
* RETURNS: the char received from the console                    *
*****************************************************************/
//  NOTE: this does *not* return special keys such as 
//        function or keypad keys.
//****************************************************************
CHAR get_char(void)
   {
   DWORD dwInputMode; /* to save the input mode */
   BOOL bSuccess;
   CHAR chBuf; /* buffer to read into */
   DWORD dwRead;

   /* save the console mode */
   bSuccess = GetConsoleMode(hStdIn, &dwInputMode);
   PERR(bSuccess, "GetconsoleMode");

   //  Disable line input. 
   //  Echo input must be disabled when disabling line input 
   bSuccess = SetConsoleMode(hStdIn, 
      dwInputMode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT);
   PERR(bSuccess, "SetConsoleMode");

   /* read a character from the console input */
   bSuccess = ReadFile(hStdIn, &chBuf, sizeof(chBuf), &dwRead, 0);
   PERR(bSuccess, "ReadFile");

   /* restore the original console input mode */
   bSuccess = SetConsoleMode(hStdIn, dwInputMode);
   PERR(bSuccess, "SetConsoleMode");

   return(chBuf);
   }

//**********************************************************
WORD get_scode(void)
   {
   WORD inchr ;
   inchr = _getch() ;
   if (inchr == 0)
      {
      inchr = _getch() ;
      inchr <<= 8 ;
      }
   else if (inchr == 0xE0)
      {
      inchr = _getch() ;
      inchr <<= 8 ;
      inchr |= 0xE0 ;
      }   
   return inchr ;
   }   

//**********************************************************
void hide_cursor(void)
   {
   BOOL bSuccess;
   CONSOLE_CURSOR_INFO cci; /* used when turning off the cursor */

   /* save then hide the cursor */
   cci.dwSize = 100;
   cci.bVisible = FALSE;
   bSuccess = SetConsoleCursorInfo(hStdOut, &cci);
   PERR(bSuccess, "SetConsoleCursorInfo");
   }   

//**********************************************************
unsigned get_window_cols(void)
{
   return (unsigned) (int) (sinfo.srWindow.Right - sinfo.srWindow.Left + 1) ;
}

//**********************************************************
unsigned get_window_rows(void)
{
   return (unsigned) (int) (sinfo.srWindow.Bottom - sinfo.srWindow.Top + 1) ;
}

//**********************************************************
int _where_x(void)
   {
   return sinfo.dwCursorPosition.X ;
   }   

int _where_y(void)
   {
   return sinfo.dwCursorPosition.Y ;
   }   

//**********************************************************
void dgotoxy(int x, int y)
   {
   sinfo.dwCursorPosition.X = x ;
   sinfo.dwCursorPosition.Y = y ;
   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
   }

//**********************************************************
void set_text_attr(WORD tFG, WORD tBG)
   {
   sinfo.wAttributes = (tBG << 4) + tFG ;
   SetConsoleTextAttribute(hStdOut, sinfo.wAttributes) ;
   }   

void set_text_attr(WORD tFGBG)
   {
   sinfo.wAttributes = tFGBG ;
   SetConsoleTextAttribute(hStdOut, sinfo.wAttributes) ;
   }   

//**********************************************************
// BOOL ScrollConsoleScreenBuffer(
//   HANDLE hConsoleOutput,  // handle to a console screen buffer
//   CONST SMALL_RECT *lpScrollRectangle,
//                           // address of screen buffer rect. to move
//   CONST SMALL_RECT *lpClipRectangle,
//                           // address of affected screen buffer rect.
//   COORD dwDestinationOrigin,  // new location of screen buffer rect.
//   CONST CHAR_INFO *lpFill   // address of fill character and color
// );
//**********************************************************
static void dscroll(WORD tBG)
{
   SMALL_RECT src ;
   // SMALL_RECT dest ;
   COORD co = { 0, 0 } ;
   // CHAR_INFO ci = { ' ', tBG } ;
   CHAR_INFO ci ;
   ci.Char.AsciiChar = ' ' ;
   ci.Attributes = tBG ;

   // GetConsoleScreenBufferInfo(hStdOut, &sinfo) ;
   // src.Left   = sinfo.srWindow.Left ;
   // src.Right  = sinfo.srWindow.Right ;
   // src.Top    = sinfo.srWindow.Top + 1 ;
   // src.Bottom = sinfo.srWindow.Bottom ;

   // Jason Hood's "buffer > screen" fix
   src.Left   = 0 ;
   src.Right  = sinfo.dwSize.X - 1;
   src.Top    = 1 ;
   src.Bottom = sinfo.dwSize.Y - 1;

   // dest.Left   = sinfo.srWindow.Left ;
   // dest.Right  = sinfo.srWindow.Right ;
   // dest.Top    = sinfo.srWindow.Top ;
   // dest.Bottom = sinfo.srWindow.Bottom ;

   ScrollConsoleScreenBuffer(hStdOut, &src, 0, co, &ci) ;
}

//**********************************************************
void dreturn(void)
   {
   sinfo.dwCursorPosition.X = 0 ;
   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;

   //  try to scroll the window
   // dscroll(original_attribs) ;
   }   

//**********************************************************
void dnewline(void)
{
   WORD x = 0 ;
   WORD y = sinfo.dwCursorPosition.Y ;

   sinfo.dwCursorPosition.X = 0 ;
   //  *this* probably shouldn't use dwSize.Y either...
   if (sinfo.dwCursorPosition.Y >= (sinfo.dwSize.Y-1)) {
      // dclreol() ;
      // 
      // //  move cursor to beginning of line
      // sinfo.dwCursorPosition.X = 0 ;
      dscroll(original_attribs) ;
      sinfo.dwCursorPosition.X = x ;
      sinfo.dwCursorPosition.Y = y ;
   }
   else {
      sinfo.dwCursorPosition.Y++ ;
   }

   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
}   

//**********************************************************
void dprintc(unsigned row, unsigned col, const char outchr)
{
   dgotoxy(col, row) ;
   dputc(outchr) ;
}   

//**********************************************************
void dprintc(unsigned row, unsigned col, unsigned attr, const char outchr)
{
   dgotoxy(col, row) ;
   set_text_attr(attr) ;
   dputc(outchr) ;
}   

//**********************************************************
void clear_visible_rows(int u, int l)
{
   COORD coord ;
   int slen ;
   DWORD wrlen ;

   coord.X = 0 ;
   coord.Y = sinfo.srWindow.Top + u ;
   slen = (l - u + 1) * sinfo.dwMaximumWindowSize.X ;

   FillConsoleOutputCharacter(hStdOut, ' ', slen, coord, &wrlen) ;
   FillConsoleOutputAttribute(hStdOut, original_attribs, slen, coord, &wrlen) ;
}         

//**********************************************************
void dclreol(void)
{
   int slen ;
   COORD ctemp ;
   DWORD wrlen ;

   CONSOLE_SCREEN_BUFFER_INFO tinfo ;

   //  make sure cursor position is updated, because WriteConsole()
   //  and WriteFile() do not update my sinfo struct!!
   GetConsoleScreenBufferInfo(hStdOut, &tinfo) ;
   ctemp = tinfo.dwCursorPosition ;
   sinfo.dwCursorPosition = ctemp ;

   slen  = sinfo.dwMaximumWindowSize.X - ctemp.X ;

   FillConsoleOutputCharacter(hStdOut, ' ', slen, ctemp, &wrlen) ;
   FillConsoleOutputAttribute(hStdOut, original_attribs, slen, ctemp, &wrlen) ;

   //  restore cursor position (this probably isn't needed!!)
   sinfo.dwCursorPosition = ctemp ;
   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
}   

//**********************************************************
//  This could be better done with FillConsoleOutput...(),
//  but to get this working now, I'll do it the cheap way,
//  and bank on fast machines to cover my laziness...
//**********************************************************
void dclreos(void)
{
   int width, rows, slen ;
   COORD ctemp ;
   DWORD wrlen ;
   CONSOLE_SCREEN_BUFFER_INFO tinfo ;

   //  make sure cursor position is updated, because WriteConsole()
   //  and WriteFile(), which are used by dputs/dputc,
   //  do not update my sinfo struct!!
   GetConsoleScreenBufferInfo(hStdOut, &tinfo) ;
   sinfo.dwCursorPosition = tinfo.dwCursorPosition ;
   ctemp = sinfo.dwCursorPosition ; //  save cursor position
   
   width = sinfo.dwMaximumWindowSize.X ;
   rows  = sinfo.srWindow.Bottom - ctemp.Y ;  //  full rows
   slen  = rows * width + (width - ctemp.X) ;

   FillConsoleOutputCharacter(hStdOut, ' ', slen, ctemp, &wrlen) ;
   FillConsoleOutputAttribute(hStdOut,   7, slen, ctemp, &wrlen) ;

   //  restore cursor position (this probably isn't needed!!)
   sinfo.dwCursorPosition = ctemp ;
   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
}         

//**********************************************************
void dputnchar(CHAR chr, CHAR attr, int count)
{
   static char ncbfr[133] ;
   set_text_attr(attr) ;
   memset(ncbfr, chr, count) ;
   ncbfr[count] = 0 ;   //  NULL-term the string
   dputs(ncbfr) ;
}   

//**********************************************************
void dclrscr(void)
   {
   COORD coord = { 0, 0 };
   int slen ;
   DWORD wrlen ;

   slen = sinfo.dwSize.X * sinfo.dwSize.Y ;

   FillConsoleOutputCharacter(hStdOut, ' ', slen, coord, &wrlen) ;
   FillConsoleOutputAttribute(hStdOut, original_attribs, slen, coord, &wrlen) ;

   dgotoxy(0,0) ; // home the cursor
   }   

//**********************************************************
int is_CRLF_present(const char *cstr)
{
   while (*cstr != 0) {
      if (*cstr == 0x0D  ||  *cstr == 0x0A)
         return 1;
      cstr++ ;
   }
   return 0;
}   

//**********************************************************
void dputc(const CHAR c)
{
   DWORD wrlen ;
   WriteFile(hStdOut, &c, 1, &wrlen, 0) ;
   sinfo.dwCursorPosition.X++ ;
}

//**********************************************************
//  This does not process special characters,
//  but writes faster than dputs()
//**********************************************************
void dputsi(const char *outstr, int slen)
{
   DWORD wrlen ;
   WriteFile(hStdOut, outstr, slen, &wrlen, 0) ;
   sinfo.dwCursorPosition.X += slen ;
}

//**********************************************************
void dputs(const char *outstr)
   {
   DWORD wrlen ;
   WORD slen = strlen(outstr) ;
   WORD rlen = sinfo.dwSize.X - sinfo.dwCursorPosition.X ;
   WORD Xi   = sinfo.dwCursorPosition.X ;
   const char *hdptr, *tlptr ;
   int ccount, done ;

   //  watch out for trouble conditions
   if (outstr == 0  ||  *outstr == 0  ||  rlen > sinfo.dwSize.X)  //lint !e774
      return ;

   //  if entire string fits on line, do this the easy way.
   if (!is_CRLF_present(outstr)  &&  rlen >= slen) {
      WriteFile(hStdOut, outstr, slen, &wrlen, 0) ;
      sinfo.dwCursorPosition.X += slen ;
   }

   //  if string has newlines in it, or if it does not
   //  fit on the current line, handle the line in parts.
   else
      {
      hdptr = tlptr = outstr ;
      ccount = 0 ;
      done = 0 ;
      while (!done)
         {
         switch (*tlptr)
            {
            case  0: //  NULL; end of string
               done = 1 ;
               break;

            case 10: //  LF
               dputsi(hdptr, ccount) ;
               dnewline() ;
               hdptr = ++tlptr ; //  this doesn't address compound CR/LF
               ccount = 0 ;
               Xi = 0 ;
               break;

            case 13: //  CR
               dputsi(hdptr, ccount) ;
               dreturn() ;
               hdptr = ++tlptr ; //  this doesn't address compound CR/LF
               ccount = 0 ;
               Xi = 0 ;
               break;

            default:
               if (++Xi == sinfo.dwSize.X)
                  {
                  dputsi(hdptr, ++ccount) ;
                  dnewline() ;
                  hdptr = ++tlptr ;
                  ccount = 0 ;
                  Xi = 0 ;
                  }
               else
                  {
                  ccount++ ;
                  tlptr++ ;
                  }   
               break;

            }  //  end switch
         }  // end while
      //  print last line fragment
      dputsi(hdptr, ccount) ;
      }   
   }

//**********************************************************
void dprints(unsigned row, unsigned col, const char* outstr)
   {
   dgotoxy(col, row) ;
   dputs(outstr) ;
   }   

//**********************************************************
unsigned drandom(unsigned rmax)
   {
   double dbl = (double) rand() / (double) (RAND_MAX + 1) ;
   unsigned rval = (unsigned) (rmax * dbl) ;
   if (rval >= rmax)
       rval = rmax - 1 ;
   return rval ;
   }   

/*********************************************************************
* FUNCTION: perr(PCHAR szFileName, int line, PCHAR szApiName,        *
*                DWORD dwError)                                      *
*                                                                    *
* PURPOSE: report API errors. Allocate a new console buffer, display *
*          error number and error text, restore previous console     *
*          buffer                                                    *
*                                                                    *
* INPUT: current source file name, current line number, name of the  *
*        API that failed, and the error number                       *
*                                                                    *
* RETURNS: none                                                      *
*********************************************************************/

/* maximum size of the buffer to be returned from FormatMessage */
#define MAX_MSG_BUF_SIZE 512

void perr(PCHAR szFileName, int line, PCHAR szApiName, DWORD dwError)
   {
   BOOL bSuccess;
   DWORD dwMode;
   static int pErrActive = 0 ;

   static CHAR szTemp[1024];
   DWORD cMsgLen;
   CHAR *msgBuf; /* buffer for message text from system */
   // int iButtonPressed; /* receives button pressed in the error box */

   //  avoid recursion
   if (pErrActive)
      return ;
   pErrActive = 1 ;

   /* set up mouse and window input */
   bSuccess = GetConsoleMode(hStdOut, &dwMode);
   PERR(bSuccess, "GetConsoleMode");

   /* when turning off ENABLE_LINE_INPUT, you MUST also turn off */
   /* ENABLE_ECHO_INPUT. */
   // bSuccess = SetConsoleMode(hStdIn, (dwMode & ~(ENABLE_LINE_INPUT |
   //     ENABLE_ECHO_INPUT)) | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
   bSuccess = SetConsoleMode(hStdOut, 
      dwMode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT ) ;
   PERR(bSuccess, "SetConsoleMode");

   /* format our error message */
   sprintf(szTemp, "\n%s: Error %u from %s on line %d:\n", szFileName,
       (unsigned) dwError, szApiName, line);

   /* get the text description for that error number from the system */
   cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, dwError,
       MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR) &msgBuf, MAX_MSG_BUF_SIZE,
       0);

   if (!cMsgLen)
     sprintf(szTemp + strlen(szTemp), "Unable to obtain error message text! \n"
         "%s: Error %u from %s on line %d", __FILE__,
         (unsigned) GetLastError(), "FormatMessage", __LINE__);
   else
     strcat(szTemp, msgBuf);

   puts(szTemp) ;
   pErrActive = 0 ;
   exit(1);
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


