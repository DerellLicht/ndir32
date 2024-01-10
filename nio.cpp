//*****************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  nio.cpp - Console I/O functions, interface to conio32.cpp
//  This file mostly manages color settings
//*****************************************************************

#undef __STRICT_ANSI__
#ifdef USE_64BIT
#define  _WIN32_WINNT    0x0600
#endif
#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>  //  PATH_MAX
#endif
#ifdef USE_64BIT
#include <fileapi.h>
#endif

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
      nputs (n.colornhead, "Press any key to continue (or ESC to exit)");
      unsigned inkey = get_scode ();
      if (inkey == ESC) {
         // if ((curlines != lines) && (!(n.ega_keep))) {
         //    set_lines (25);
         // }
         error_exit (DATA_OKAY, NULL);
      }

      if (n.color) {
         dreturn ();            // CR only!! 
         dclreol ();
      }
      else {
         printf ("\n");
      }
      linecnt = 1;
   }
}

/******************************************************************/
void ncrlf_raw(void)
{
   if (n.color) {
      dnewline ();
   }
   else {
      printf ("\n");
   }
   testpause ();
}

/******************************************************************/
void ncrlf (void)
{
   if (n.color) {
      dnewline ();
   }
   else {
      printf ("\n");
   }
   
   // testpause ();
   if (!n.pause)
      return;
   if (is_redirected ())
     return;

   if (++linecnt >= lines - 1) {
      nputs (n.colornhead, "Press any key to continue (or ESC to exit)");
      unsigned inkey = get_scode ();
      if (inkey == ESC) {
         error_exit (DATA_OKAY, NULL);
      }

      if (n.color) {
         dreturn ();            // CR only!! 
         dclreol ();
      }
      else {
         printf ("\n");
      }
      linecnt = 1;
   }
}

/******************************************************************/
void nputc (uchar attr, const uchar outchr)
{
   // unsigned hattr ;
   if (n.color) {
      set_text_attr (attr);
      dputc (outchr);
   }
   else
      printf ("%c", outchr);
}

/******************************************************************/
void nputs (uchar attr, const char *outstr)
{
   if (n.color) {
      set_text_attr (attr);
      dputs (outstr);
   }
   else
      printf ("%s", outstr);
}

/******************************************************************/
void nputsw(uchar attr, const char *outstr, int wlen, int clen)
{
   if (n.color) {
      set_text_attr (attr);
      dputsiw(outstr, wlen, clen);
   }
   else {
      dputsiw(outstr, wlen, clen);
   }
}

/******************************************************************/
void nput_char (uchar attr, char chr, int count)
{
   if (n.color) {
      dputnchar (chr, attr, count);
   }
   else {
      for (int j = 0; j < count; j++)
         putchar (chr);
   }
}

/******************************************************************/
void nput_line (uchar attr, char chr)
{
   int j, wincols = get_window_cols() - 1 ;
   if (n.color) {
      dputnchar (chr, attr, wincols);
   }
   else {
      for (j = 0; j < wincols; j++)
         putchar (chr);
   }
   ncrlf ();
}

