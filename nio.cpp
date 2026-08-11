//*****************************************************************
//  Copyright (c) 1998-2025 Derell Licht                       
//  nio.cpp - Console I/O functions, interface to conio32.cpp
//  This file mostly manages color settings
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>  //  MAX_PATH_LEN
#endif
#ifdef _WIN64
#include <fileapi.h>
#endif
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"

static unsigned linecnt = 0 ;           //  non-color display line counter

//*********************************************************
void nclrscr(void)
{
   linecnt = 0 ;
   dclrscr();
}

//*********************************************************
//  this handles full-screen in NON-COLOR mode.
//*********************************************************
static void testpause (void)
{
   if (is_redirected ())
     return;
   if (!n.pause)
      return;

   if (++linecnt >= lines - 1) {
      nputs (n.colornhead, _T("Press any key to continue (or ESC to exit)"));
      unsigned inkey = get_scode ();
      if (inkey == ESC) {
         error_exit (DATA_OKAY, NULL);
      }

      if (n.color) {
         dreturn ();            // CR only!! 
         dclreol ();
      }
      else {
         dputc (_T('\n'));
      }
      linecnt = 1;
   }
}

/******************************************************************/
void ncrlf (void)
{
   if (n.color) {
      dnewline ();
   }
   else {
      // _tprintf (_T("\n"));
      dputc (_T('\n'));
   }
   
   testpause ();
}

/******************************************************************/
void nputc (uchar attr, const uchar outchr)
{
   if (n.color) {
      set_text_attr (attr);
   }
   dputc (outchr);
}

/******************************************************************/
void nputs (uchar attr, const TCHAR *outstr)
{
   if (n.color) {
      set_text_attr (attr);
   }
   dputs (outstr);
}

/******************************************************************/
void nput_char (uchar attr, TCHAR chr, int count)
{
   if (count <= 0) {
      return ;
   }
   if (n.color) {
      set_text_attr (attr);
   }
   dputnchar (chr, attr, count);
}

/******************************************************************/
void nput_line (uchar attr, TCHAR chr)
{
   int j, wincols = get_window_cols() - 1 ;
   if (n.color) {
      dputnchar (chr, attr, wincols);
   }
   else {
      for (j = 0; j < wincols; j++) {
         // _puttchar (chr);
         dputc (chr);
      }
   }
   ncrlf ();
}

