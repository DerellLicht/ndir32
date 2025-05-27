//*****************************************************************
//  Copyright (c) 1995-2024  Daniel D Miller
//  FILELIST.CPP - NDIR file-listing handlers                      
//*****************************************************************

// #undef __STRICT_ANSI__
#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>
#endif
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"  //  _where_x()

//*****************************************************************
static unsigned list_count = 0 ;

//lint -esym(715,fptr)
static void print_null(ffdata *fptr) 
{
}

extern void lfn_print2(ffdata *fptr);
extern void lfn_print4(ffdata *fptr);
extern void lfn_print6(ffdata *fptr);

//lint -esym(843, fprint, lfn_fprint)  could be declared as const
static void (*lfn_fprint[7])(ffdata *fptr) = 
{
   print_null,
   print1,
   lfn_print2,
   print_null,
   lfn_print4,
   print_null,
   lfn_print6,
} ;

static unsigned disp_cols ;    //  calculated screen columns for listings
static unsigned line_len ; //  width of column
unsigned name_width = 0 ;

static const int FILE_SIZE_LEN =  6 ;
static const int DATE_TIME_LEN = 15 ;

//*********************************************************
static void display_batch_mode(void)
{
   ffdata *ftemp = ftop ;
   while (ftemp != NULL) {
      _tprintf(_T("%s%s%s\n"), leftstr, ftemp->filename, rightstr) ;
      ftemp = ftemp->next ;
   }
}

/*****************************************************************/
static void filehead(void)
{
   // unsigned j ;
   unsigned k ;
   int wincols = get_window_cols() - 1 ;
   if (is_redirected()) {
      wincols = 80 ;
   }

   //****************************************************
   //  If minimize is selected, display short header/footer
   //****************************************************
   if (n.minimize) {
      if (list_count == 0)
         _stprintf(tempstr, _T("%-38s   "), base_path) ;
      else
         _stprintf(tempstr, _T("%-49s   "), base_path) ;
      nputs(n.colornhead, tempstr) ;

      if (_where_x() < (wincols - 28)) {
         nputs(n.colornhead, _T("Volume label is ")) ;
         nputs(n.colorxhead, volume_name) ;
      }
      ncrlf() ;

      list_count++ ; //  only do the indent on first pass
   }
   //****************************************************
   //  If minimize is not selected, display full header/footer
   //****************************************************
   else {
      nput_line(n.colorframe, '*') ;

      _stprintf(tempstr, _T("Directory of %-38s "), base_path) ;
      nputs(n.colornhead, tempstr) ;
      if (_tcslen(base_path) > 43)
         ncrlf() ;

      nputs(n.colornhead, _T("Volume label is ")) ;
      nputs(n.colorxhead, volume_name) ;
      ncrlf() ;

      if (ftop == NULL) {
         nput_line(n.colorframe, dline) ;
      } else {
         // memset(&tempstr[0], dline, wincols) ;
         // tempstr[wincols] = 0 ;
         // if (col_width[disp_cols] > 0) {
         if (line_len > 0) {
            // syslog(_T("line_len: %u, disp_cols: %u\n"), line_len, disp_cols);
            // j = col_width[disp_cols] ;
            // j = line_len ;
            for (k=1; k<disp_cols; k++) {
               // tempstr[j] = tline ;
               nput_char(n.colorframe, dline, line_len) ;
               nput_char(n.colorframe, tline, 1) ;
               // j += col_width[disp_cols] + 1 ;
               // j += line_len + 1 ;
            }
            //  last, draw remaining lines
            nput_char(n.colorframe, dline, line_len) ;
         }
         ncrlf() ;
      } 
   }
}

/*****************************************************************/
void put_disk_summary(void)
{
   nputs(n.colornhead, _T("Disk capacity = ")) ;
   nputs(n.colorxhead, convert_to_commas(diskbytes, NULL)) ;
   nputs(n.colornhead, _T(" bytes; Free disk space = ")) ;
   nputs(n.colorxhead, convert_to_commas(diskfree, NULL)) ;
   nputs(n.colornhead, _T(" bytes")) ;
   ncrlf() ;
}

/*****************************************************************/
static void fileend(void)
{
   unsigned k ;
   ULONGLONG dirsecbytes, clusters ;
   ULONGLONG dbytes, dsbytes ;
   ffdata *ftemp ;
   int wincols = get_window_cols() - 1 ;

   //  get directory info
   dbytes = 0 ;
   dsbytes = 0 ;
   ftemp = ftop ;
   while (ftemp != NULL) {
      dbytes += ftemp->fsize ;
      clusters = ftemp->fsize / clbytes ;
      if ((ftemp->fsize % clbytes) > 0)  clusters++ ; //lint !e79 bad type for % operator

      // dirsecbytes = (clusters * (__int64) clbytes) ;
      dirsecbytes = (clusters * (u64) clbytes) ;
      dsbytes += dirsecbytes ;

      ftemp = ftemp->next ;
   }

   //**************************************************************
   //  If minimize is selected, display short header/footer
   //**************************************************************
   if (n.minimize) {
      _stprintf(tempstr, _T("%d"), filecount) ;
      nputs(n.colorxhead, tempstr) ;

      nputs(n.colornhead, _T(" files: ")) ;

      // itemp64.convert(dbytes) ;
      nputs(n.colorxhead, convert_to_commas(dbytes, NULL)) ;
      nputs(n.colornhead, _T(" <")) ;
      // itemp64.convert(dsbytes) ;
      nputs(n.colorxhead, convert_to_commas(dsbytes, NULL)) ;

      nputs(n.colornhead, _T(">; Disk: ")) ;
//    diskbytes = totals1.QuadPart ;
//    diskfree = frees1.QuadPart ;
      // itemp64.convert(diskfree) ;
      nputs(n.colorxhead, convert_to_commas(diskfree, NULL)) ;
      nputs(n.colornhead, _T(" <")) ;
      // itemp64.convert(diskbytes) ;
      nputs(n.colorxhead, convert_to_commas(diskbytes, NULL)) ;
      nputs(n.colornhead, _T(">")) ;
      ncrlf() ;
   }
   //**************************************************************
   //  If minimize is not selected, display full header/footer
   //**************************************************************
   else {
      //  draw the bottom framing line
      if (ftop == NULL) {
         nput_line(n.colorframe, dline) ;
      } else {
         memset(&tempstr[0], dline, wincols) ;
         // tempstr[wincols] = 0 ;
         // if (col_width[disp_cols] > 0) {
         if (line_len > 0) {
            // j = col_width[disp_cols] ;
            // j = line_len ;
            for (k=1; k<disp_cols; k++) {
               // tempstr[j] = bline ;
               nput_char(n.colorframe, bline, line_len) ;
               nput_char(n.colorframe, tline, 1) ;
               // j += col_width[disp_cols] + 1 ;
               // j += line_len + 1 ;
            }
            //  last, draw remaining lines
            nput_char(n.colorframe, dline, line_len) ;
         }
         // nputs(n.colorframe, tempstr) ;
         ncrlf() ;
      }

      //  draw the ending labels
      _stprintf(tempstr, _T("%d"), filecount) ;
      nputs(n.colorxhead, tempstr) ;
      nputs(n.colornhead, _T(" files total ")) ;
      // itemp64.convert(dbytes) ;
      nputs(n.colorxhead, convert_to_commas(dbytes, NULL)) ;
      nputs(n.colornhead, _T(" bytes, using ")) ;
      // itemp64.convert(dsbytes) ;
      nputs(n.colorxhead, convert_to_commas(dsbytes, NULL)) ;
      nputs(n.colornhead, _T(" bytes of disk space.")) ;
      ncrlf() ;

      put_disk_summary() ;
   }
}

//*****************************************************************
static void lfn_get_columns(void)
{
   ffdata *ftemp = ftop ;
   unsigned wincols ;

   //  find length of longest filename
   unsigned max_name_len = 0 ;
   while (ftemp != NULL) {
      unsigned cur_name_len = ftemp->mb_len ;
      if (cur_name_len > max_name_len) {
         max_name_len = cur_name_len ;
      }
      ftemp = ftemp->next ;
   }

   wincols = get_window_cols() ;
   //  this is necessary because if redirection is in place,
   //  'console width' may not be valid`
   if (is_redirected()) {
      wincols = 80 ;
   }
   //  compute line length (don't forget space between items
   switch (columns) {
   case 1:
      line_len = wincols-1 ;
      break;

   case 2:
      // line_len = max_name_len + filesize_len + date_time_len ;
      //  date_time_len = 8 + 1 + 5 = 14
      line_len = max_name_len + 1 + FILE_SIZE_LEN + 1 + DATE_TIME_LEN ;
      break;

   case 4:
      line_len = max_name_len + 1 + FILE_SIZE_LEN ;
      break;

   case 6:
      line_len = max_name_len ;
      break;

   default: line_len = max_name_len ;  break ;  //  make lint happy
   }
   // [66188] win_cols: 135, max_name_len: 39, line_len: 61
   // syslog("win_cols: %u, max_name_len: %u, line_len: %u\n", wincols, max_name_len, line_len);

   //  compute line count:
   //   (disp_cols * line_len) + (disp_cols-1)*1 < row_len
   //   (disp_cols * line_len) + disp_cols - 1 < 79
   //   disp_cols * (line_len + 1) < 80
   //                    80
   //   disp_cols = ------------
   //               line_len + 1
   disp_cols = wincols / (line_len + 1) ;

   //  shortcut solution for very long filenames
   if (disp_cols == 0)
      disp_cols = 1 ;
   //  shortcut solution for very short filenames
   // else if (disp_cols > (unsigned) columns)
   //    disp_cols = (unsigned) columns ;

   //  now find max width of filename listing
   // unsigned max_fcols = wincols / disp_cols ;
   line_len = wincols / disp_cols ;
   line_len-- ;   //  subtract out column separator
   switch (columns) {
   case 1:
      name_width = line_len - 43 ;
      break;

   case 2:
      name_width = line_len - (FILE_SIZE_LEN + 1 + DATE_TIME_LEN + 1) ;
      break;

   case 4:
      name_width = line_len - (FILE_SIZE_LEN + 1) ;
      break;

   case 6:
      name_width = line_len ;
      break;

   default: 
      name_width = line_len ;
      break ;
   }
   // [66900] win_cols: 135, max_name_len: 39, line_len: 66, name_width: 43
   // syslog(_T("columns: %u, win_cols: %u, line_len: %u, disp_cols: %u, max_name_len: %u, name_width: %u\n"), 
   //     columns, wincols, line_len, disp_cols, max_name_len, name_width);
   // _stprintf(tempfmtstr, "%c-%us", '%', name_width) ;
}

//*****************************************************************
static void list_files_horizontally(void)
{
   unsigned j = 0 ;

   ffdata *ftemp = ftop ;

   //  see how many columns we can support with requested formats
   lfn_get_columns() ;  //  set disp_cols, name_width

   filehead() ;
   //  then list the files
   while (ftemp != NULL) {
      lfn_fprint[columns](ftemp) ;  //  horizontal listing
      if (++j == disp_cols) {
         ncrlf() ;
         j = 0 ;
      } else {
         nput_char(n.colorframe, vline, 1) ;
      }

      ftemp = ftemp->next ;
   }

   //  put in closing newline, if needed
   if (j != 0)
      ncrlf() ;
   fileend() ;
}

//*****************************************************************
//  XTDIR mode
//*****************************************************************
static void list_files_qwise(void)
{
   // int j = 0 ;
   ffdata *ftemp ;
   unsigned width, col = 0, slen ;
   int first_line = 1, new_line ;
   TCHAR prev_ext[10] ;
   unsigned maxext ;

   prev_ext[0] = 0 ; //  make lint happy

   width = (is_redirected()) ? 80 : get_window_cols() ;
   // width = get_window_cols() ;

   //  first, find the longest extension in the current file list
   ftemp = ftop ;
   maxext = 0 ;
   while (ftemp != NULL) {
      slen = _tcslen(ftemp->ext) ;
      if (maxext < slen)
          maxext = slen ;
      
      //  get next filename
      ftemp = ftemp->next ;
   }
   
   //  then list the files
   ftemp = ftop ;
   filehead() ;
   new_line = 1 ;
   while (ftemp != NULL) {
      //  see if file extention is changing
      if (first_line  ||  _tcsicmp(prev_ext, ftemp->ext) != 0) {
         if (first_line)
            first_line = 0 ;
         else 
            ncrlf() ;
         _stprintf(tempstr, _T("%-*s: "), maxext, ftemp->ext) ;
         nputs(ftemp->color, tempstr) ;
         col = maxext+2 ;
         _tcscpy(prev_ext, ftemp->ext) ;
         new_line = 1 ;
      }

      //  see if next filename is going to overrun line; 
      //  if so, start next line...
      // slen = (n.lfn_off) ? 9 : (_tcslen(ftemp->name) + 2) ;
      slen = _tcslen(ftemp->name) + 2 ;
      if (col + slen > width) {
         nputs((ftemp->dirflag) ? n.colordir : ftemp->color, _T(", ")) ;
         ncrlf() ;
         _stprintf (tempstr, _T("%*s  "), maxext, _T(" ")) ;
         nputs(ftemp->color, tempstr) ;
         col = maxext+2 ;
         new_line = 1 ;
      } 
      //  if not starting new line, add comma separator
      if (new_line) {
         new_line = 0 ;
      // } else if (!n.lfn_off) {
      } else {
         nputs((ftemp->dirflag) ? n.colordir : ftemp->color, _T(", ")) ;
      }
      //  select appropriate color and print the filename
      // _stprintf(tempstr, (n.lfn_off) ? "%-8s " : "%s", ftemp->name) ;
      // nputs((ftemp->dirflag) ? n.colordir : ftemp->color, tempstr) ;
      nputs((ftemp->dirflag) ? n.colordir : ftemp->color, ftemp->name) ;
      col += slen ;
      
      //  get next filename
      ftemp = ftemp->next ;
   }
   ncrlf() ;
   fileend() ;
}

//*****************************************************************
static void list_files_vertically(void)
{
   unsigned rows, partrows, j, k ;
   //  these arrays must have enough entries for
   //  the maximum number of columns on the screen
   unsigned pcols[20] ;
   ffdata *ftemps[20] ;

   //************************************************
   //  now, start displaying files
   //************************************************
   unsigned fcount = 0 ;
   lfn_get_columns() ;  //  set disp_cols, name_width
   
   // syslog(_T("filecount: %u\n"), filecount);

   rows = (unsigned) filecount / disp_cols ;
   partrows = (unsigned) filecount % disp_cols ;

   for (j=0; j< disp_cols ; j++)  pcols[j] = rows ;
   for (j=0; j<partrows; j++)  pcols[j]++ ;  //lint !e771

   if (partrows > 0)
      rows++ ;

   //************************************************
   //  split the  file list into (columns) lists.
   //  However, remember to re-combine the lists
   //  before ftemps[] goes out of scope, or
   //  we won't be able to free them later!!
   //************************************************
   // ffdata *fprev ;
   ftemps[0] = ftop ;
   ftemps[1] = ftop ;
   for (j=1; j< disp_cols; j++) {
      //  find end of current list
      for (k=0; k<pcols[j-1]; k++)  //lint !e771
         ftemps[j] = ftemps[j]->next ;

      //  now, break the list
      ftemps[j+1] = ftemps[j] ;
   }

   filehead() ;   //  uses rows, columns
   j = 0 ;
   while (LOOP_FOREVER) {
      if (fcount < filecount) {
         lfn_fprint[columns](ftemps[j]) ; //  vertical listing  NOLINT
         ftemps[j] = ftemps[j]->next ;
      }
      //  if no files left to display, fill in row with spaces
      else {
         // nput_char(n.colorframe, ' ', col_width[disp_cols]) ;
         nput_char(n.colorframe, _T(' '), line_len) ;
      }   

      //  draw separator characters as required
      fcount++ ;
      if (++j == disp_cols) {
         ncrlf() ;
         j = 0 ;
         if (--rows == 0)
            break;
      } else {
         nputc(n.colorframe, vline) ;
      }
   }  //  loop forever
   fileend() ;
}

/****************************************************************/
/*  Display directory data in "files" mode.                     */
/****************************************************************/
void display_files(void)
{
   if (n.batch) {       /*  batch mode  */
      display_batch_mode() ;
      return ;
   }

   //****************************************************
   //  normal display mode
   //****************************************************
   if (ftop == NULL) {
      filehead() ;
      nputs(n.colordefalt, _T("No matching files found.\n\r")) ;
      fileend() ;
      return ;
   }

   //************************************************
   //  present normal file listings
   //************************************************
   if (n.horz & 2)
      list_files_qwise() ; //  XTDIR mode
   else if (n.horz & 1)
      list_files_horizontally() ;
   else {
      list_files_vertically() ;
   }
}
