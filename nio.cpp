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
#ifdef USE_64BIT
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
   if (!n.pause)
      return;
   if (is_redirected ())
     return;

   if (++linecnt >= lines - 1) {
      nputs (n.colornhead, _T("Press any key to continue (or ESC to exit)"));
      unsigned inkey = get_scode ();
      if (inkey == ESC) {
         // if ((curlines != lines) && (!(n.ega_keep))) {
         //    set_lines (25);
         // }
         error_exit (DATA_OKAY, NULL);
      }

      dreturn ();            // CR only!! 
      dclreol ();
      linecnt = 1;
   }
}

/******************************************************************/
void ncrlf_raw(void)
{
   dnewline ();
   testpause ();
}

/******************************************************************/
void ncrlf (void)
{
   dnewline ();
   
   if (!n.pause)
      return;
   //  don't do 'pause on full screen' if we are redirecting
   if (is_redirected ())
     return;

   if (++linecnt >= lines - 1) {
      nputs (n.colornhead, _T("Press any key to continue (or ESC to exit)"));
      unsigned inkey = get_scode ();
      if (inkey == ESC) {
         error_exit (DATA_OKAY, NULL);
      }

      dreturn ();            // CR only!! 
      dclreol ();
      linecnt = 1;
   }
}

/******************************************************************/
void nputc (uchar attr, const uchar outchr)
{
   set_text_attr (attr);
   dputc (outchr);
}

/******************************************************************/
void nputs (uchar attr, const TCHAR *outstr)
{
   set_text_attr (attr);
   dputs (outstr);
}

/******************************************************************/
void nput_char (uchar attr, TCHAR chr, int count)
{
   if (count <= 0) {
      return ;
   }
   set_text_attr (attr);
   dputnchar (chr, attr, count);
}

/******************************************************************/
void nput_line (uchar attr, TCHAR chr)
{
   int wincols = get_window_cols() - 1 ;
   dputnchar (chr, attr, wincols);
   ncrlf ();
}

