//*****************************************************************
//  Copyright (c) 1995-2025  Daniel D Miller
//  tdisplay.cpp: display directory tree                  
//*****************************************************************
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "treelist.h"

//lint -esym(528, display_size, display_tree_filename, formstr)
//lint -esym(843, formstr) variable could be declared as const
//lint -esym(552, tree_level_limit)  Symbol not accessed

static TCHAR const * const dhdrl =
   _T("+-------------+-------------+--------------+--------------");

//0         1         2
//012345678901234567890123456789
//=========================+===========+===========+==============+==============
static TCHAR formstr[50];

static uint wincols      = 80 ;
static uint name_end_col = 25 ;

//**********************************************************
static void display_size(ULONGLONG dlen, unsigned slen, unsigned attr)
{
   if (n.size_display == 2) {
      dlen /= 1000000 ;
      _stprintf (tempstr, _T("%*s"), slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, _T('M')) ;
   } else 
   if (dlen > (ULONGLONG) 999999999  ||  n.size_display == 1) {
      dlen /= 1000 ;
      _stprintf (tempstr, _T("%*s"), slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, _T('K')) ;
   } else 
   {
      _stprintf (tempstr, _T("%*s"), slen, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
   }
}

//**********************************************************
static void display_tree_filename (TCHAR *frmstr, dirs *ktemp)
{
   int wlen = ktemp->name.length();
   
   //  calculate required padding spaces
   int frmlen = _tcslen(frmstr);
   uint namelen = wlen + (uint) frmlen ;
   int splen = 0 ;
   
   //  if name string overruns data spaces, skip to new line
   if (namelen >= name_end_col) {
      nputs (n.colorframe, frmstr);
      nputs(dtree_colors[level], ktemp->name.c_str());
      ncrlf ();
      _stprintf (tempstr, _T("%-*s"), name_end_col+1, frmstr);
      nputs (dtree_colors[level], tempstr);  //  spaces
   }
   else {
      nputs (n.colorframe, frmstr);
      nputs(dtree_colors[level], ktemp->name.c_str());
      // syslog(L"[%u] %s%s\n", level, frmstr, ktemp->name.c_str());
      
      //  pad gap between end of folder name and data area, with spaces
      splen = (name_end_col + 1) - namelen ;
      if (splen > 0) {
         nput_char(n.colorframe, _T(' '), splen) ;
      }
      else {
         syslog(_T("(other), splen: %d, nec: %u, flen: %u, slen: %u\n"), 
            splen,  name_end_col, frmlen, wlen);
      }
   }
}  //  slen

#ifdef  USE_VECTOR
//***********************************************************************************
//  recursive routine to display directory tree
//  do all subroutines, then go to next.
//  
//  vector mode:
//  Each brother passed to this function, will print his name and info, 
//  Then iterate over each of his children(brother->brothers),
//  and let them repeat the story.
//  
//  Thus, each folder listing will be followed by all lower folder listings...
//  AKA, depth-first traversal
//***********************************************************************************
static void display_dir_tree (std::vector<dirs> brothers)
{
   if (brothers.empty()) {
      return;
   }

   // dirs *cur_node = &brothers[0] ;   
   // uint num_folders = cur_node->brothers.size() ;
   uint num_folders = brothers.size() ;
   uint fcount = 0 ;
   // console->dputsf(L"[%u] %s\n", num_folders, parent_name) ;
   
   for(auto &file : brothers) {
      dirs *ktemp = &file;
      fcount++ ;
      //  first, build tree list for current level
      if (level == 0) {
         formstr[0] = (wchar_t) 0;
      }
      else {
         //  if we are at end of list of brothers, use 'last folder' character
         if (fcount == num_folders) {
            formstr[level - 1] = (wchar_t) '\\';   //lint !e743 
            formstr[level] = (wchar_t) NULL;
         }
         else {
            formstr[level - 1] = (wchar_t) '+';   //lint !e743 
            formstr[level] = (wchar_t) NULL;
         }
      }

      //*****************************************************************
      //                display data for this level                      
      //*****************************************************************
      display_tree_filename (formstr, ktemp);
      // console->dputsf(L"%s %s\n", formstr, ktemp->name.c_str()) ;
      switch (n.tree) {
         //  show file/directory sizes only
         case 1:
            if (ktemp->dirsize != ktemp->subdirsize ||   //lint !e777
               ktemp->dirsecsize != ktemp->subdirsecsize) {   //lint !e777
               // dsize.convert  (ktemp->dirsize);
               // dssize.convert (ktemp->dirsecsize);
               // sdsize.convert (ktemp->subdirsize);

               //  now, print the normal directory
               // _stprintf (tempstr, "%11s", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);

               // _stprintf(tempstr, "%11s %14s", dssize_ptr, sdsize_ptr) ;
               // _stprintf (tempstr, "%11s %14s", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->dirsize, 13, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
               display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
               nputc (dtree_colors[level], _T(' '));
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               nputs (dtree_colors[level], _T("             "));
               nputc (n.colorframe, vline);
               nputs (dtree_colors[level], _T("              "));
               // _stprintf(tempstr, "            %14s", sdsize_ptr) ;
               // sdsize.convert (ktemp->subdirsize);
               // _stprintf (tempstr, "%14s", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }                   /* end  else !(ktemp->nsdi) */

            // _stprintf(tempstr, "%14s", sdssize_ptr) ;
            // sdssize.convert (ktemp->subdirsecsize);
            // _stprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
            break;

         //  show file/directory counts only
         case 4:
            if ((ktemp->files == ktemp->subfiles) &&
               (ktemp->directs == ktemp->subdirects)) {
               //  now, print the normal directory
               nputs (dtree_colors[level], _T("             "));
               nputc (n.colorframe, vline);

               // sdsize.convert ((unsigned long long) ktemp->files);
               // _stprintf (tempstr, "            %12s  ", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               nputs (dtree_colors[level], _T("              "));
               display_size((ULONGLONG) ktemp->files, 12, dtree_colors[level]) ;

               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);
               // _stprintf(tempstr, "%14s", sdssize_ptr) ;
               // sdssize.convert ((unsigned long long) ktemp->directs);
               // _stprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 10, dtree_colors[level]) ;
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               // dsize.convert ((unsigned long long) ktemp->files);
               // _stprintf (tempstr, "%9s  ", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->files, 11, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);

               // dssize.convert ((unsigned long long) ktemp->directs);
               // sdsize.convert ((unsigned long long) ktemp->subfiles);
               // _stprintf (tempstr, "%9s   %12s  ", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 11, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("   "));
               display_size((ULONGLONG) ktemp->subfiles, 12, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);

               // sdssize.convert ((unsigned long long) ktemp->subdirects);
               // _stprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->subdirects, 10, dtree_colors[level]) ;
            }                   /* end  else !(ktemp->nsdi) */
            break;

         //  show mixed size, file counts, directory counts
         case 5:
            //  now, print the normal directory
            // dsize.convert ((unsigned long long) ktemp->subfiles);
            // _stprintf (tempstr, "%9s  ", dsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subfiles, 12, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T(" "));
                  nputc (n.colorframe, vline);

            // dssize.convert ((unsigned long long) ktemp->subdirects);
            // sdsize.convert ((unsigned long long) ktemp->dirsecsize);
            // _stprintf (tempstr, "%9s   %13s ", dssize.putstr (),
            //   sdsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subdirects, 12, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T("  "));
            display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T(" "));
                  nputc (n.colorframe, vline);

            // sdssize.convert ((unsigned long long) ktemp->subdirsecsize);
            // _stprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
            break;

         default:
            break;              // make lint happy
      }
      ncrlf ();

      //  build tree string for deeper levels
      if (level > 0) {
         if (fcount == num_folders) {
            formstr[level - 1] = ' ';
         }
         else {
            formstr[level - 1] = '|' ; //lint !e743 
         }
      }                         //  if level > 1
      
      //  process any sons
      level++;
      if (!n.tree_short || level <= tree_level_limit) {
         display_dir_tree (ktemp->brothers);
      }
      formstr[--level] = (TCHAR) 0; // NOLINT
   }                            //  while not done listing directories
}

#else

//**********************************************************
//  recursive routine to display directory tree
//  do all subroutines, then go to next.
//**********************************************************
static void display_dir_tree (dirs * ktop)
{
   if (ktop == NULL)
      return ;

   //  next, build tree lists for subsequent levels (recursive)
   dirs *ktemp = ktop;
   while (ktemp != NULL) {
      //  first, build tree list for current level
      if (level == 0) {
         formstr[0] = (TCHAR) 0;
      }
      else {
         if (ktemp->brothers == (struct dirs *) NULL) {
            // formstr[level - 1] = (TCHAR) (n.low_ascii) ? '\\' : '�';   //lint !e743 
            formstr[level - 1] = (TCHAR) '\\' ;   //lint !e743 
            formstr[level] = (TCHAR) 0;
         }
         else {
            // formstr[level - 1] = (TCHAR) (n.low_ascii) ? '+' : '�';   //lint !e743 
            formstr[level - 1] = (TCHAR) '+' ;   //lint !e743 
            formstr[level] = (TCHAR) 0;
         }
      }

      //*****************************************************************
      //                display data for this level                      
      //*****************************************************************
      display_tree_filename (formstr, ktemp);
      switch (n.tree) {
         //  show file/directory sizes only
         case 1:
            if (ktemp->dirsize != ktemp->subdirsize ||   //lint !e777
               ktemp->dirsecsize != ktemp->subdirsecsize) {   //lint !e777
               // dsize.convert  (ktemp->dirsize);
               // dssize.convert (ktemp->dirsecsize);
               // sdsize.convert (ktemp->subdirsize);

               //  now, print the normal directory
               // _stprintf (tempstr, "%11s", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);

               // _stprintf(tempstr, "%11s %14s", dssize_ptr, sdsize_ptr) ;
               // _stprintf (tempstr, "%11s %14s", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->dirsize, 13, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
               display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
               nputc (dtree_colors[level], _T(' '));
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               nputs (dtree_colors[level], _T("           "));
               nputc (n.colorframe, vline);
               nputs (dtree_colors[level], _T("            "));
               // _stprintf(tempstr, "            %14s", sdsize_ptr) ;
               // sdsize.convert (ktemp->subdirsize);
               // _stprintf (tempstr, "%14s", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }                   /* end  else !(ktemp->nsdi) */

            // _stprintf(tempstr, "%14s", sdssize_ptr) ;
            // sdssize.convert (ktemp->subdirsecsize);
            // _stprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
            break;

         //  show file/directory counts only
         case 4:
            if ((ktemp->files == ktemp->subfiles) &&
               (ktemp->directs == ktemp->subdirects)) {
               //  now, print the normal directory
               nputs (dtree_colors[level], _T("           "));
               nputc (n.colorframe, vline);

               // sdsize.convert ((unsigned long long) ktemp->files);
               // _stprintf (tempstr, "            %12s  ", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               nputs (dtree_colors[level], _T("            "));
               display_size((ULONGLONG) ktemp->files, 12, dtree_colors[level]) ;

               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);
               // _stprintf(tempstr, "%14s", sdssize_ptr) ;
               // sdssize.convert ((unsigned long long) ktemp->directs);
               // _stprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 10, dtree_colors[level]) ;
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               // dsize.convert ((unsigned long long) ktemp->files);
               // _stprintf (tempstr, "%9s  ", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->files, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);

               // dssize.convert ((unsigned long long) ktemp->directs);
               // sdsize.convert ((unsigned long long) ktemp->subfiles);
               // _stprintf (tempstr, "%9s   %12s  ", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("   "));
               display_size((ULONGLONG) ktemp->subfiles, 12, dtree_colors[level]) ;
               nputs (dtree_colors[level], _T("  "));
               nputc (n.colorframe, vline);

               // sdssize.convert ((unsigned long long) ktemp->subdirects);
               // _stprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->subdirects, 10, dtree_colors[level]) ;
            }                   /* end  else !(ktemp->nsdi) */
            break;

         //  show mixed size, file counts, directory counts
         case 5:
            //  now, print the normal directory
            // dsize.convert ((unsigned long long) ktemp->subfiles);
            // _stprintf (tempstr, "%9s  ", dsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subfiles, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T("  "));
                  nputc (n.colorframe, vline);

            // dssize.convert ((unsigned long long) ktemp->subdirects);
            // sdsize.convert ((unsigned long long) ktemp->dirsecsize);
            // _stprintf (tempstr, "%9s   %13s ", dssize.putstr (),
            //   sdsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subdirects, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T("   "));
            display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
            nputs (dtree_colors[level], _T(" "));
                  nputc (n.colorframe, vline);

            // sdssize.convert ((unsigned long long) ktemp->subdirsecsize);
            // _stprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
            break;

         default:
            break;              // make lint happy
      }
      ncrlf ();

      //  build tree string for deeper levels
      if (level > 0) {
         if (ktemp->brothers == NULL)
            formstr[level - 1] = ' ';
         else
            // formstr[level - 1] = (n.low_ascii) ? '|' : '�'; //lint !e743 
            formstr[level - 1] = '|' ; //lint !e743 
      }                         //  if level > 1

      //  process any sons
      level++;
      if (!n.tree_short || level <= tree_level_limit) {
         display_dir_tree (ktemp->sons);
      }
      formstr[--level] = (TCHAR) 0; // NOLINT

      //  goto next brother
      ktemp = ktemp->brothers;
   }                            //  while not done listing directories
}
#endif

/*****************************************************************/
static void printdirheader (void)
{
   nputs (n.colornhead, _T("Directory of ")); //  len = 13
   
   // _stprintf (tempstr, _T("%s"), base_path.c_str());
   // nputs (n.colorxhead, tempstr);
   nputs (n.colorxhead, base_path.c_str());

   // uint vnlen = 16 + _tcslen(volume_name) ; //  16 is length of 'Volume label is '
   uint vnlen = 16 + volume_name.length() ; //  16 is length of 'Volume label is '
   // uint blen  = 13 + _tcslen(base_path) ;   //  13 is length of 'Directory of '
   uint blen  = 13 + base_path.length() ;   //  13 is length of 'Directory of '
   if ((blen + vnlen) >= wincols) 
      ncrlf ();
   else
      nput_char (n.colornhead, _T(' '), (wincols - blen - vnlen - 1));
   nputs (n.colornhead, _T("Volume label is ")); //  len = 16
   nputs (n.colorxhead, volume_name.c_str());
   ncrlf ();

   //**************************************
   //  Heading line 1
   //**************************************
//  01234567890123456789012 01234567890123456789012345678
// +=======================+=============================                                                                                |   size of requested   |    total size,  including
// |       directory       |     lower subdirectories
// +-----------+-----------+--------------+--------------

   nput_char(n.colorframe, dline, name_end_col) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 27) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 29) ;
   ncrlf ();

   switch (n.tree) {
      case 1:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("     size of requested     "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("    total size,  including"));
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         // nputs (n.colornhead, "Subdirectory names       ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("         directory         "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("     lower subdirectories"));
         ncrlf ();
         break;
         

      case 4:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("   files and directories   "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("   files and directories "));
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("   in current directory    "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T(" in current and lower dirs"));
         ncrlf ();
         break;

      case 5:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("   files and directories   "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("    files and directories"));
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, _T(' '), name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("     cumulative counts     "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("       cumulative sizes   "));
         ncrlf ();
         break;

      default:
         break;                 // make lint happy
   }

   //**************************************
   //  Heading line 4
   //**************************************
   nput_char (n.colornhead, _T(' '), name_end_col);
   // nputs (n.colorframe, (n.low_ascii) ? dhdrl : dhdr);
   nputs (n.colorframe, dhdrl);
   ncrlf ();

   //**************************************
   //  Heading line 5
   //**************************************
   // nputs (n.colornhead, "Subdirectory names       ");
   _stprintf(tempstr, _T("%-*s"), name_end_col, _T("Subdirectory names"));
   // syslog(L"[%u] %u: %s\n", n.tree, name_end_col, tempstr);
   // hex_dump((u8 *)tempstr, 80);
   //  53 00 20 00 20 00 20 00 20 00 20 00 20 00 20 00  | S. . . . . . . . |
   //  wsprintf() just is not working in tdm64 V10.3.0
   nputs (n.colornhead, tempstr);
      
   nputc (n.colorframe, vline);
   switch (n.tree) {
      case 1:
         nputs (n.colornhead, _T("   in bytes  "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("  disk space "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("   in bytes   "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("  disk space"));
         ncrlf ();
         break;

      case 4:
         nputs (n.colornhead, _T("     files   "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T(" directories "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("       files  "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T(" directories"));
         ncrlf ();
         break;

      case 5:
         nputs (n.colornhead, _T("     files   "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T(" directories "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("  disk space  "));
         nputc (n.colorframe, vline);
         nputs (n.colornhead, _T("  disk space"));
         ncrlf ();
         break;

      default:
         break;                 // make lint happy
   }

   //**************************************
   //  Heading line 6
   //**************************************
//  01234567890123456789012 01234567890123456789012345678
// +=======================+=============================                                                                                |   size of requested   |    total size,  including
// |       directory       |     lower subdirectories
// +-----------+-----------+--------------+--------------
//  01234567890 01234567890 01234567890123 01234567890123
   nput_char(n.colorframe, dline, name_end_col) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 13) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 13) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 14) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 14) ;
   ncrlf ();
}

//*********************************************************
// +===========+===========+==============+==============
//    6,144,636|  6,152,192    945,328,679|   945,377,280
// ============+==========================+==============
//  01234567890 01234567890123456789012345 01234567890123
static void print_dir_end (void)
{
   //  draw divider line for bottom of data
   nput_char(n.colorframe, dline, name_end_col) ;
   nput_char(n.colorframe, dline, 14) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 28) ;
   nput_char(n.colorframe, tline, 1) ;
   nput_char(n.colorframe, dline, 14) ;
   ncrlf ();

   //  now show disk totals
   put_disk_summary();
   ncrlf ();
}

//*********************************************************
//================================================================================
void draw_dir_tree (void)
{
   level = 0;
   wincols = get_window_cols() ;
   //  this is necessary because if redirection is in place,
   //  'console width' may not be valid`
   if (is_redirected()) {
      wincols = 80 ;
   }

   if (wincols != 80) {
      name_end_col = wincols - (80-21) ;
   }

   // syslog("level before displaying: %u\n", level);
   printdirheader ();
   
#ifdef  USE_VECTOR
   // dirs *temp = &dlist.brothers[0] ;
   display_dir_tree(dlist.brothers);
#else   
   display_dir_tree (top);
#endif   
   print_dir_end ();
}
