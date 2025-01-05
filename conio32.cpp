//***************************************************************************
//  Copyright (c) 1995-2023  Daniel D Miller
//  CONIO32.CPP: Template for 32-bit console programs                        
//                                                                           
//  Written by:   Daniel D. Miller                                           
//                                                                           
//  Last Update:  11/07/01 14:44                                             
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

#include "common.h"
#include "ndir32.h"
#include "conio32.h"

static HANDLE hStdOut ;
static HANDLE hStdIn ;

static CONSOLE_SCREEN_BUFFER_INFO sinfo ;

static bool redirected = false ;

static WORD original_attribs = 3 ;

unsigned lines = 0 ;

//lint -esym(759,dclreos) -esym(765,dclreos) -esym(714,dclreos) 
//lint -esym(759,dprints) -esym(765,dprints) -esym(714,dprints) 
//lint -esym(765,dputsi) 
//lint -esym(759,dprintc) -esym(765,dprintc) -esym(714,dprintc) 
//lint -esym(759,set_text_attr)

//lint -esym(759,control_handler) -esym(765, control_handler)

//***************************************************************************
//***************************************************************************
//                GENERIC 32-BIT CONSOLE I/O FUNCTIONS
//***************************************************************************
//***************************************************************************

// #define PERR(bSuccess, api) {if (!(bSuccess)) perr(__FILE__, __LINE__, api, GetLastError());}
static void PERR(bool bSuccess, PCHAR szApiName)
{
   if (!bSuccess) {
      syslog("[%u] %s\n", GetLastError(), szApiName);
   }
}

//**********************************************************
//lint -esym(715,dwCtrlType)
BOOL control_handler(DWORD dwCtrlType)
   {
   BOOL bSuccess;
   DWORD dwMode;

   //  restore the screen mode
   bSuccess = GetConsoleMode(hStdOut, &dwMode);
   PERR(bSuccess, "GetConsoleMode");
   bSuccess = SetConsoleMode(hStdOut, dwMode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT ) ;
   PERR(bSuccess, "SetConsoleMode");

   //  display message and do other work
   return FALSE ;
   }   

//**********************************************************
unsigned get_window_cols(void)
{
   return (unsigned) (int) (sinfo.srWindow.Right - sinfo.srWindow.Left + 1) ;
}

//**********************************************************
static unsigned get_window_rows(void)
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

//***************************************************************************
bool is_redirected(void)
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
      syslog("GetStdHandle(STD_OUTPUT_HANDLE): %s\n", get_system_message()) ;
      exit(1) ;
   }
   // PERR(hStdOut != INVALID_HANDLE_VALUE, "GetStdHandle");
   hStdIn = GetStdHandle(STD_INPUT_HANDLE);
   if (hStdIn == INVALID_HANDLE_VALUE) {
      syslog("GetStdHandle(STD_INPUT_HANDLE): %s\n", get_system_message()) ;
      exit(1) ;
   }
   // PERR(hStdIn != INVALID_HANDLE_VALUE, "GetStdHandle");

   //  Put up a meaningful console title.
   //  Will this *always* succeed???
   if (title != 0) {
      bSuccess = SetConsoleTitleA(title);
      PERR(bSuccess, "SetConsoleTitle");
   }

   //  get screen information.
   //  If this call fails, assume we are re-directing output.
   //  Unfortunately, this also fails on all Bash-window terminals
   bSuccess = GetConsoleScreenBufferInfo(hStdOut, &sinfo) ;
   // PERR(bSuccess, "GetConsoleScreenBufferInfo");
   if (!bSuccess) {
      // printf("GetConsoleScreenBufferInfo: %s\n", get_system_message()) ;
      // exit(1) ;
      //  if we cannot get console info for StdOut, 
      //  most likely StdOut is redirected to a file.
      //  Let's see if we can get info for StdIn
      // bSuccess = GetConsoleScreenBufferInfo(hStdIn, &sinfo) ;
      // if (!bSuccess) {
      //    printf("darn, can't get info on stdin either...\n") ;
      // }
      //  Nope, that doesn't work either.  I guess I just can't get
      //  console info (in particular, console width) when we're redirecting output.
      
      redirected = true ;
      return ; 
   }
   // [33240] dwSize: 200x2000, cursor: 0x0, max: 200x109, window: L0, T0, R199, B49
   // syslog("dwSize: %ux%u, cursor: %u,%u, max: %ux%u, window: L%u, T%u, R%u, B%u\n",
   //    sinfo.dwSize.X, sinfo.dwSize.Y,
   //    sinfo.dwCursorPosition.X,
   //    sinfo.dwCursorPosition.Y,
   //    sinfo.dwMaximumWindowSize.X, sinfo.dwMaximumWindowSize.Y, 
   //    sinfo.srWindow.Left, sinfo.srWindow.Top, sinfo.srWindow.Right, sinfo.srWindow.Bottom);
   
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
   
   // lines = (unsigned) (int) (sinfo.srWindow.Bottom - sinfo.srWindow.Top + 1) ;
   lines = get_window_rows ();
}   

//**********************************************************
void restore_console_attribs(void)
{
   sinfo.wAttributes = original_attribs ;
   SetConsoleTextAttribute(hStdOut, sinfo.wAttributes) ;
}

//*************************************************************************************
// * PURPOSE: get a single character from the standard input handle 
// *                                                                
// * INPUT: none                                                    
// *                                                                
// * RETURNS: the char received from the console                    
//*************************************************************************************
//  This function differs from get_scode() in that it does not require conio.h.
//  However, it does *not* return special keys such as function or keypad keys.
//  It is *not* used in this application
//*************************************************************************************
//lint -esym(759,get_char) -esym(765,get_char) -esym(714,get_char) 
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
//  this is used only by testpause(), to wait on full screen
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
//lint -esym(759,hide_cursor) -esym(765,hide_cursor) -esym(714,hide_cursor) 
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
//   HANDLE hConsoleOutput,               // handle to a console screen buffer
//   CONST SMALL_RECT *lpScrollRectangle, // address of screen buffer rect. to move
//   CONST SMALL_RECT *lpClipRectangle,   // address of affected screen buffer rect.
//   COORD dwDestinationOrigin,           // new location of screen buffer rect.
//   CONST CHAR_INFO *lpFill              // address of fill character and color
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
void dnewline(void)
{
   sinfo.dwCursorPosition.X = 0 ;
   //  *this* probably shouldn't use dwSize.Y either...
   if (sinfo.dwCursorPosition.Y >= (sinfo.dwSize.Y-1)) {
      // dclreol() ;
      // 
      // //  move cursor to beginning of line
      dscroll(original_attribs) ;
      sinfo.dwCursorPosition.Y = sinfo.dwCursorPosition.Y ;
   }
   else {
      sinfo.dwCursorPosition.Y++ ;
   }

   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
}   
   
//**********************************************************
//  CR only, no LF
//**********************************************************
void dreturn(void)
   {
   sinfo.dwCursorPosition.X = 0 ;
   SetConsoleCursorPosition(hStdOut, sinfo.dwCursorPosition) ;
   }   

//**********************************************************
//  debug function
//**********************************************************
//lint -esym(714, dshow_row_info)
//lint -esym(759, dshow_row_info)
//lint -esym(765, dshow_row_info)
void dshow_row_info(char *msg)
{
   char* default_msg = "x"; //lint !e1778 assignment is not const
   if (msg == NULL) {
      msg = default_msg ;
   }
   // syslog("%s: cursor: %ux%u, dwSize: %ux%u, where: %ux%u, win_cols: %u\n", msg,
   //    sinfo.dwCursorPosition.X, 
   //    sinfo.dwCursorPosition.Y, 
   //    sinfo.dwSize.X, 
   //    sinfo.dwSize.Y, 
   //    _where_x(), _where_y(),
   //    get_window_cols()
   //    ) ;
}  //lint !e438  msg

//**********************************************************
void dprintc(unsigned row, unsigned col, const TCHAR outchr)
{
   dgotoxy(col, row) ;
   dputc(outchr) ;
}   

//**********************************************************
void dprintc(unsigned row, unsigned col, unsigned attr, const TCHAR outchr)
{
   dgotoxy(col, row) ;
   set_text_attr(attr) ;
   dputc(outchr) ;
}   

//**********************************************************
//lint -esym(759,clear_visible_rows) -esym(765,clear_visible_rows) -esym(714,clear_visible_rows) 
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
void dputnchar(TCHAR chr, TCHAR attr, int count)
{
   static TCHAR ncbfr[MAX_CHAR_COLS+1] ;
   if (count > MAX_CHAR_COLS) {
      syslog("dputnchar: count too large: %d\n", count);
      count = MAX_CHAR_COLS ;
   }
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
static int is_CRLF_present(const char *cstr)
{
   while (*cstr != 0) {
      if (*cstr == 0x0D  ||  *cstr == 0x0A)
         return 1;
      cstr++ ;
   }
   return 0;
}   

//**********************************************************
void dputc(const TCHAR c)
{
   DWORD wrlen ;
   WriteFile(hStdOut, &c, 1, &wrlen, 0) ;
   sinfo.dwCursorPosition.X++ ;
}

//**********************************************************************************
//  This does not process special characters, but writes faster than dputs().
//  It also is *only* used by dputs()
//**********************************************************************************
static void dputsi(const TCHAR *outstr, int slen)
{
   DWORD wrlen ;
   WriteFile(hStdOut, outstr, slen, &wrlen, 0) ;
   sinfo.dwCursorPosition.X += slen ;
}

//**********************************************************************************
//  This function will deal with printing multi-byte strings
//  in an ASCII (i.e., non-Unicode) program.
//  It passes both the byte-length of the string, for printing,
//  and the multi-byte length for updating screen position.
//**********************************************************************************
void dputsiw(const char *outstr, int wlen, int clen)
{
   DWORD wrlen ;
   WriteFile(hStdOut, outstr, wlen, &wrlen, 0) ;
   sinfo.dwCursorPosition.X += clen ;
}

//**********************************************************
void dputs(const TCHAR *outstr)
   {
   DWORD wrlen ;
   WORD slen = _tcslen(outstr) ;
   WORD rlen = sinfo.dwSize.X - sinfo.dwCursorPosition.X ;
   WORD Xi   = sinfo.dwCursorPosition.X ;
   const TCHAR *hdptr, *tlptr ;
   int ccount, done ;

   //  watch out for trouble conditions
   if (outstr == 0  ||  *outstr == 0  ||  rlen > sinfo.dwSize.X)  //lint !e774
      return ;

   //  if entire string fits on line, do this the easy way.
#ifdef UNICODE   
   if (rlen >= slen) {
#else   
   if (!is_CRLF_present(outstr)  &&  rlen >= slen) {
#endif   
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
void dprints(unsigned row, unsigned col, const TCHAR* outstr)
   {
   dgotoxy(col, row) ;
   dputs(outstr) ;
   }   
