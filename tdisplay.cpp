//*****************************************************************
//  Copyright (c) 1995-2023  Daniel D Miller
//  tdisplay.cpp: display directory tree                  
//*****************************************************************
#include <windows.h>
#include <stdio.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "treelist.h"

static char const * const dhdr =
   "ÃÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄ";

static char const * const dhdrl =
   "+-----------+-----------+--------------+--------------";

//0         1         2
//012345678901234567890123456789
//=========================+===========+===========+==============+==============
static char formstr[50];

static uint wincols      = 80 ;
static uint name_end_col = 25 ;
static uint center_col   = 37 ;
static uint left_div     = 49 ;
static uint right_div    = 64 ;

//**********************************************************
static void display_size(ULONGLONG dlen, unsigned slen, unsigned attr)
{
   if (n.size_display == 2) {
      dlen /= 1000000 ;
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, 'M') ;
   } else 
   if (dlen > (ULONGLONG) 999999999  ||  n.size_display == 1) {
      dlen /= 1000 ;
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, 'K') ;
   } else 
   {
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
   }
}

//**********************************************************
//  Note: lstr contains form string plus filename
//**********************************************************
static void display_tree_filename (char *frmstr, dirs *ktemp)
{
   // char levelstr[PATH_MAX];
   // sprintf (levelstr, "%s%s", frmstr, ktemp->name);
   int wlen = _tcslen(ktemp->name);
   //  why is mb_len == 0 on base folder??
   if (ktemp->mb_len == 0) {
      //  this won't work for Unicode targets
      ktemp->mb_len = wlen ;
   }
   // uint slen = ktemp->mb_len;
   
   //  calculate required padding spaces
   int frmlen = _tcslen(frmstr);
   uint namelen = ktemp->mb_len + (uint) frmlen ;
   int splen = 0 ;
   
   //  if name string overruns data spaces, skip to new line
   if (namelen >= name_end_col) {
      nputs (n.colorframe, frmstr);
      nputsw(dtree_colors[level], ktemp->name, wlen, ktemp->mb_len);
      
      //  skip the vlines on name row
      // splen = (center_col) - frmlen - slen ;
      // splen = name_end_col - namelen ;
      // if (splen > 0) {
      //    nput_char(n.colorframe, ' ', splen) ;
      // }
      // else {
      //    syslog("(s > nec), splen: %d, ccol: %u, flen: %u, slen: %u, nec: %u, rdiv: %u\n", 
      //       splen, center_col, frmlen, slen, name_end_col, right_div);
      // }
      // nputc (n.colorframe, vline);
      // nput_char(dtree_colors[level], ' ', name_end_col+1) ;
      // nputc (n.colorframe, vline);
      ncrlf ();
      sprintf (tempstr, "%-*s", name_end_col+1, frmstr);
      nputs (dtree_colors[level], tempstr);  //  spaces
   }
   else {
      nputs (n.colorframe, frmstr);
      nputsw(dtree_colors[level], ktemp->name, wlen, ktemp->mb_len);
      
      //  pad gap between end of folder name and data area, with spaces
      splen = (name_end_col + 1) - namelen ;
      if (splen > 0) {
         nput_char(n.colorframe, ' ', splen) ;
      }
      else {
         syslog("(other), splen: %d, nec: %u, flen: %u, slen: %u\n", 
            splen,  name_end_col, frmlen, ktemp->mb_len);
      }
   }
}  //  slen

//**********************************************************
//  recursive routine to display directory tree
//  do all subroutines, then go to next.
//**********************************************************
static void display_dir_tree (dirs * ktop)
{
   dirs *ktemp = ktop;
   if (ktop == NULL)
      return;

   //  next, build tree lists for subsequent levels (recursive)
   while (ktemp != NULL) {
      //  first, build tree list for current level
      if (level == 0) {
         formstr[0] = (char) NULL;
      }
      else {
         if (ktemp->brothers == (struct dirs *) NULL) {
            formstr[level - 1] = (char) (n.low_ascii) ? '\\' : 'À';   //lint !e743 
            formstr[level] = (char) NULL;
         }
         else {
            formstr[level - 1] = (char) (n.low_ascii) ? '+' : 'Ã';   //lint !e743 
            formstr[level] = (char) NULL;
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
               // sprintf (tempstr, "%11s", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);

               // sprintf(tempstr, "%11s %14s", dssize_ptr, sdsize_ptr) ;
               // sprintf (tempstr, "%11s %14s", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->dirsize, 11, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
               display_size(ktemp->dirsecsize, 11, dtree_colors[level]) ;
               nputc (dtree_colors[level], ' ');
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               nputs (dtree_colors[level], "           ");
               nputc (n.colorframe, vline);
               nputs (dtree_colors[level], "            ");
               // sprintf(tempstr, "            %14s", sdsize_ptr) ;
               // sdsize.convert (ktemp->subdirsize);
               // sprintf (tempstr, "%14s", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }                   /* end  else !(ktemp->nsdi) */

            // sprintf(tempstr, "%14s", sdssize_ptr) ;
            // sdssize.convert (ktemp->subdirsecsize);
            // sprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
            break;

         //  show file/directory counts only
         case 4:
            if ((ktemp->files == ktemp->subfiles) &&
               (ktemp->directs == ktemp->subdirects)) {
               //  now, print the normal directory
               nputs (dtree_colors[level], "           ");
               nputc (n.colorframe, vline);

               // sdsize.convert ((unsigned long long) ktemp->files);
               // sprintf (tempstr, "            %12s  ", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               nputs (dtree_colors[level], "            ");
               display_size((ULONGLONG) ktemp->files, 12, dtree_colors[level]) ;

               nputs (dtree_colors[level], "  ");
               nputc (n.colorframe, vline);
               // sprintf(tempstr, "%14s", sdssize_ptr) ;
               // sdssize.convert ((unsigned long long) ktemp->directs);
               // sprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 10, dtree_colors[level]) ;
            }

            /*  no subdirectories are under this one  */
            else {
               //  now, print the normal directory
               // dsize.convert ((unsigned long long) ktemp->files);
               // sprintf (tempstr, "%9s  ", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->files, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], "  ");
               nputc (n.colorframe, vline);

               // dssize.convert ((unsigned long long) ktemp->directs);
               // sdsize.convert ((unsigned long long) ktemp->subfiles);
               // sprintf (tempstr, "%9s   %12s  ", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], "   ");
               display_size((ULONGLONG) ktemp->subfiles, 12, dtree_colors[level]) ;
               nputs (dtree_colors[level], "  ");
               nputc (n.colorframe, vline);

               // sdssize.convert ((unsigned long long) ktemp->subdirects);
               // sprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->subdirects, 10, dtree_colors[level]) ;
            }                   /* end  else !(ktemp->nsdi) */
            break;

         //  show mixed size, file counts, directory counts
         case 5:
            //  now, print the normal directory
            // dsize.convert ((unsigned long long) ktemp->subfiles);
            // sprintf (tempstr, "%9s  ", dsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subfiles, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], "  ");
                  nputc (n.colorframe, vline);

            // dssize.convert ((unsigned long long) ktemp->subdirects);
            // sdsize.convert ((unsigned long long) ktemp->dirsecsize);
            // sprintf (tempstr, "%9s   %13s ", dssize.putstr (),
            //   sdsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subdirects, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], "   ");
            display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
            nputs (dtree_colors[level], " ");
                  nputc (n.colorframe, vline);

            // sdssize.convert ((unsigned long long) ktemp->subdirsecsize);
            // sprintf (tempstr, "%14s", sdssize.putstr ());
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
            formstr[level - 1] = (n.low_ascii) ? '|' : '³'; //lint !e743 
      }                         //  if level > 1

      //  process any sons
      level++;
      if (!n.tree_short || level <= tree_level_limit) {
         display_dir_tree (ktemp->sons);
      }
      formstr[--level] = (char) NULL;

      //  goto next brother
      ktemp = ktemp->brothers;
   }                            //  while not done listing directories
}

/*****************************************************************/
static void printdirheader (void)
{
   // int slen, freelen, vnlen, blen;

   if (!n.color) 
      display_logo();
   nputs (n.colornhead, "Directory of "); //  len = 13
   sprintf (tempstr, "%s", base_path);
   nputs (n.colorxhead, tempstr);

   uint vnlen = 16 + _tcslen(volume_name) ; //  16 is length of 'Volume label is '
   uint blen  = 13 + _tcslen(base_path) ;   //  13 is length of 'Directory of '
   if ((blen + vnlen) >= wincols) 
      ncrlf ();
   else
      nput_char (n.colornhead, ' ', (wincols - blen - vnlen - 1));
   nputs (n.colornhead, "Volume label is "); //  len = 16
   nputs (n.colorxhead, volume_name);
   ncrlf ();

   //**************************************
   //  Heading line 1
   //**************************************
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[name_end_col] = tempstr[left_div] = tline;
   nputs (n.colorframe, tempstr);
   ncrlf ();

   switch (n.tree) {
      case 1:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "   size of requested   ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "    total size,  including");
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         // nputs (n.colornhead, "Subdirectory names       ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "       directory       ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "     lower subdirectories");
         ncrlf ();
         break;

      case 4:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " files and directories ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "   files and directories ");
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " in current directory  ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " in current and lower dirs");
         ncrlf ();
         break;

      case 5:
         //**************************************
         //  Heading line 2
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " files and directories ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "    files and directories");
         ncrlf ();

         //**************************************
         //  Heading line 3
         //**************************************
         nput_char (n.colornhead, ' ', name_end_col);
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "   cumulative counts   ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "       cumulative sizes   ");
         ncrlf ();
         break;

      default:
         break;                 // make lint happy
   }

   //**************************************
   //  Heading line 4
   //**************************************
   nput_char (n.colornhead, ' ', name_end_col);
   nputs (n.colorframe, (n.low_ascii) ? dhdrl : dhdr);
   ncrlf ();

   //**************************************
   //  Heading line 5
   //**************************************
   // nputs (n.colornhead, "Subdirectory names       ");
   sprintf(tempstr,"%-*s", name_end_col, "Subdirectory names");
   nputs (n.colornhead, tempstr);
   nputc (n.colorframe, vline);
   switch (n.tree) {
      case 1:
         nputs (n.colornhead, "  in bytes ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " disk space");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "   in bytes   ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "  disk space");
         ncrlf ();
         break;

      case 4:
         nputs (n.colornhead, "    files  ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "directories");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "       files  ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, " directories");
         ncrlf ();
         break;

      case 5:
         nputs (n.colornhead, "    files  ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "directories");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "  disk space  ");
         nputc (n.colorframe, vline);
         nputs (n.colornhead, "  disk space");
         ncrlf ();
         break;

      default:
         break;                 // make lint happy
   }

   //**************************************
   //  Heading line 6
   //**************************************
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[name_end_col] = tempstr[left_div] = bline;
   tempstr[center_col] = tempstr[right_div] = xline;
   nputs (n.colorframe, tempstr);
   ncrlf ();
}

//*********************************************************
static void print_dir_end (void)
{
   //  draw divider line for bottom of data
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[center_col] = tempstr[right_div] = bline;
   nputs (n.colorframe, tempstr);
   ncrlf ();

   //  now show disk totals
   put_disk_summary();
}

//*********************************************************
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
      name_end_col = wincols - (80-25) ;
      center_col   = wincols - (80-37) ;
      left_div     = wincols - (80-49) ;
      right_div    = wincols - (80-64) ;
      // 135: 80, 92, 104, 119
      // syslog("%u: %u, %u, %u, %u\n", wincols,
      //    name_end_col, center_col, left_div, right_div);
   }

   // syslog("level before displaying: %u\n", level);
   printdirheader ();
   display_dir_tree (top);
   print_dir_end ();
}
