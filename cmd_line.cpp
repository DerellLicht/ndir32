//*****************************************************************
//  CMD_LINE.CPP - NDIR command_line parsing                       
//                                                                 
//  Written by:   Daniel D. Miller  (the Derelict)                 
//                                                                 
//*****************************************************************

#include <windows.h>
#ifdef _lint
#include <stdlib.h>
#endif
#include <ctype.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h" //  is_redirected()

// static TCHAR const topchar      = _T(0xD1) ;  /*  �  */
// static TCHAR const bottomchar   = _T(0xCF) ;  /*  �  */
// static TCHAR const vlinechar    = _T(0xB3) ;  /*  �  */
// static TCHAR const xlinechar    = _T(0xD8) ;  /*  �  */
// static TCHAR const dvlinechar   = _T(0xCD) ;  /*  �  */
static TCHAR const crosschar    = _T('+') ;  /*  +  */
static TCHAR const altvlinechar = _T('|') ;  /*  |  */
static TCHAR const altdvlchar   = _T('=') ;  /*  =  */

//*************************************************************
//  Convert command-line switches into flags
//*************************************************************
static int update_switches (TCHAR *argstr)
{
   int slen = 1;
   TCHAR inchar = *argstr++;

   switch (inchar) {
      case 'a':  
         if (*argstr == '1') {
            n.long_attr = 1 ;
            n.show_all = 1;  
            argstr++;
            slen++ ;
         } else {
            n.show_all ^= 1;  
         }
         break;
      case 'b':  n.batch ^= 1;  break;
      case 'c':  n.clear ^= 1;  break;
      case 'd':
         if (*argstr == '2') {
            n.tree = 4;
            argstr++;
         }
         else if (*argstr == '3') {
            n.tree = 5;
            argstr++;
         }
         else {
            n.tree = 1;
         }
         break;
      case 'e':  n.sort = 0;  break;
      case 'f':  n.tree = 2;  break;
      case 'g':  n.dir_first ^= 1;  break;
      case 'h':  n.horz ^= 1;  break;
      case 'i':  
         if (*argstr == 'i') {
            n.drive_summary = DSUMMARY_USED;
            argstr++;
            slen++ ;
         } else {
            n.drive_summary = DSUMMARY_FREE;
         }         
         break;
      // case 'j':  n.low_ascii ^= 1;  break;
      case 'k':  n.color ^= 1;  break;     //  redirection flag
      //  V2.62, 01/08/24 - short-filename support removed
      // case 'l':  n.lfn_off ^= 1;  break;   //  toggle long_filename flag
      case 'm':
         n.minimize ^= 1;
         break;
      case 'n':
         n.sort = 1;
         break;

      case 'o':
         if (*argstr == '1')
            n.fdate_option = FDATE_LAST_ACCESS;
         else if (*argstr == '2')
            n.fdate_option = FDATE_CREATE_TIME;
         else
            n.fdate_option = FDATE_LAST_WRITE;
         slen = 2;
         break;
      case 'p':  n.pause ^= 1;  break;
      case 'q':  n.horz ^= 2;  break;
      case 'r':  n.reverse ^= 1;  break;
      case 's':  n.sort = 2;  break;
      case 'S':  //  our first case-sensitive switch!!
         //  check to see if special display options have been selected:
         //  uchar size_display ; //  0=bytes/normal, 1=KB, 2=MB
         switch (*argstr) {
         case '1':   n.size_display = 1 ;  break ;
         case '2':   n.size_display = 2 ;  break ;
         default:    n.size_display = 0 ;  break ;
         }
         argstr++;
         slen++ ;
         break;
      case 't':  n.sort = 3;  break;
      case 'u':  n.ucase ^= 1;  break;
      case 'v':  n.info = 1;  break;
      case 'w':  n.showSHRfiles ^= 1;  break;
      case 'x':  n.exec_only ^= 1;  break;
         // case 'y':
      case 'z':  n.sort = 4;  break;

      case '1':  n.format = 0;  break;
      case '2':  n.format = 1;  break;
      case '4':  n.format = 2;  break;
      case '6':  n.format = 3;  break;
      case '3':  n.format = 5;  break;
      // case '5':  n.ega_keep ^= 1;  break;

      case ',':  
         n.tree_short = 1;  
         tree_level_limit++ ;
         break;

      case '?':
         n.help = 1;
         break;

      case '[':   //  accept this with or without preceding '-'
         batch_set_left_str(argstr);
         slen = 0 ;  //  tell argument-parsing function to skip rest of this argument
         break;

      case ']':   //  accept this with or without preceding '-'
         batch_set_right_str(argstr);
         slen = 0 ;  //  tell argument-parsing function to skip rest of this argument
         break;

      default:
         break;                 //  make lint happy
   }                            /* end SWITCH      */
   return slen;   //lint !e438  argstr
}  

//**********************************************************
void parse_command_string (TCHAR *cmdstr)
{
   TCHAR *extptr;
   int slen;

   switch (*cmdstr) {
      case '-':
      case '/':
         cmdstr++;              //  skip the switch char
         while (LOOP_FOREVER) {
            if (*cmdstr == 13 || *cmdstr == 0)
               break;

            slen = update_switches (cmdstr);
            if (slen == 0) {
               break ;
            }
            cmdstr += slen;
         }
         break;

      case '[':   //  accept this with or without preceding '-'
         cmdstr++;              //  skip the switch char
         batch_set_left_str(cmdstr);
         break;

      case ']':   //  accept this with or without preceding '-'
         cmdstr++;              //  skip the switch char
         batch_set_right_str(cmdstr);
         break;

      case '!':
         cmdstr++;              //  skip the switch char
         _tcscpy (tempstr, cmdstr);

         //  process exclusion extentions...
         extptr = _tcschr (tempstr, '.');
         //  why worry about length of extension??
         // if (extptr != 0 && _tcslen (extptr) <= 4) 
         if (extptr != 0) 
         {
            update_exclusion_list(extptr) ;
         }
         break;

      default:
         _tcscpy (tempstr, cmdstr);
         insert_target_filespec (tempstr);
         break;
   }                            //  switch (*cmdstr)
}

//**********************************************************
void parse_command_args (int startIdx, int argc, TCHAR **argv)
{
   TCHAR *argvptr, *spcptr;

   //  see if we have an environment variable to deal with
   if (startIdx == 0) {
      argvptr = argv[0];
      while (LOOP_FOREVER) {
         spcptr = _tcschr (argvptr, ' ');
         if (spcptr == 0) {
            parse_command_string (argvptr);
            break;
         }

         //  fall thru here if space is found
         *spcptr++ = 0;         //  NULL-term first string, point to next string
         parse_command_string (argvptr);
         argvptr = spcptr;
      }
      startIdx = 1;
   }

   //  deal with normal command-line arguments
   for (int j = startIdx; j < argc; j++) {
      //  fix an obscure bug under win32 and 4DOS7;
      //  for some reason, "/?" is getting changed to "/~" ...
      // argvptr = argv[j];
      // if (_tcscmp(argvptr, _T("/~")) == 0) {
      //    *(argvptr+1) = _T('/') ;
      // }
      parse_command_string (argv[j]);
   }
}

//************************************************************
//  Test for, and correct, inconsistent flags
//************************************************************
void verify_flags (void)
{
   if (is_redirected ()) {
      n.color = 0 ;
   }


   /************************************************/
   if (n.tree == 1 || n.tree == 4 || n.tree == 5) {
      n.exec_only = 0;
   }

   //******************************************************
   //  at this point, if exec_only is set, stuff the       
   //  executable filespec into target[]                   
   //******************************************************
   //  The drawback to this technique is that the user     
   //  cannot apply -x to multiple drives simultaneously.  
   //******************************************************
   if (n.exec_only) {
      insert_target_filespec (_T("*.exe"));
      insert_target_filespec (_T("*.com"));
      insert_target_filespec (_T("*.bat"));
      insert_target_filespec (_T("*.btm"));
      insert_target_filespec (_T("*.cmd"));
   }

   //*********************************************
   //  If -3 or -5 was used, set 43/50-line mode
   //  deprecated on 01.17.23
   //*********************************************
   // if (n.ega_keep) {
   //    set_lines (50);
   //    curlines = lines;
   //    lines = 50;
   // }

   /*  Set 'dir tree' conditions  */
   if (n.tree == 1 || n.tree == 4 || n.tree == 5 || n.drive_summary > DSUMMARY_NONE)
      n.minimize = 0;

   /* If not 'find all'  then don't use attr bits = 0x27  */
   // findattr = IFF (n.show_all)  THENN 0xF7  ELSSE 0x10 ;

   if (n.horz & 2) {
      n.horz &= ~1;
      n.reverse = 0;
      n.sort = 0;
   }

   //****************************************************
   //  fix up the format specifiers
   //****************************************************
   if (n.tree == 1 || n.tree == 4) {
      columns = 0;
   }
   else if (n.horz == 2) {
      columns = 1;
      n.sort = 0;               //  force sort-by-extension
   }
   else {
      switch (n.format) {
         case 0:  columns = 1;  break;
         case 1:  columns = 2;  break;
         case 2:  columns = 4;  break;
         case 3:  columns = 6;  break;
         case 5:  columns = 3;  break;
         default:
            n.format = 2;
            columns = 4;
            break;
      }
   }

   //******************************************************
   //  Initialize line-drawing variables
   //  03/17/25 - as part of Unicode conversion,
   //  just use low ASCII in all cases
   //******************************************************
   tline = bline = crosschar;
   vline = altvlinechar;
   dline = altdvlchar;
}

