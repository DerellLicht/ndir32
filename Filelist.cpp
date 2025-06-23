//*****************************************************************
//  Copyright (c) 1995-2025  Daniel D Miller
//  FILELIST.CPP - NDIR file-listing handlers                      
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>
#endif
#include <tchar.h>

//lint -esym(530, fsize)  Symbol not initialized (yes, it is)

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
//  strings used with batch mode
//lint -esym(728, std::wstring)  Symbol not explicitly initialized
//lint -esym(551, std::wstring)  Symbol not accessed
// static TCHAR leftstr[37] = _T("");
// static TCHAR rightstr[37] = _T("");
static std::wstring leftstr {};
static std::wstring rightstr {};

//*********************************************************
void batch_set_left_str(TCHAR *instr)
{
   leftstr = instr ;
}

//*********************************************************
void batch_set_right_str(TCHAR *instr)
{
   rightstr = instr ;
}

//*********************************************************
static void display_batch_mode(void)
{
   for(auto &file : flist) {
      _tprintf(_T("%s%s%s\n"), leftstr.c_str(), file.filename.c_str(), rightstr.c_str()) ;
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
         nputs(n.colorxhead, volume_name.c_str()) ;
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
      nputs(n.colorxhead, volume_name.c_str()) ;
      ncrlf() ;

      if (flist.empty()) {
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
   for(auto &file : flist) {
      ftemp = &file ;
      dbytes += ftemp->fsize ;
      clusters = ftemp->fsize / clbytes ; //lint !e573  Signed-unsigned mix with divide
      if ((ftemp->fsize % clbytes) > 0) { //lint !e573  Signed-unsigned mix with divide
         clusters++ ; //lint !e79 !e530
      }

      // dirsecbytes = (clusters * (__int64) clbytes) ;
      dirsecbytes = (clusters * (u64) clbytes) ;
      dsbytes += dirsecbytes ;
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
      if (flist.empty()) {
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
   //  find length of longest filename
   unsigned max_name_len = 0 ;
   ffdata *ftemp ;
   for(auto &file : flist) {
      ftemp = &file ;
      unsigned cur_name_len = ftemp->mb_len ;
      if (max_name_len < cur_name_len) {
          max_name_len = cur_name_len ;
      }
   }

   unsigned wincols = get_window_cols() ;
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
   disp_cols = wincols / (line_len + 1) ;  //lint !e573

   //  shortcut solution for very long filenames
   if (disp_cols == 0)
      disp_cols = 1 ;
   //  shortcut solution for very short filenames
   // else if (disp_cols > (unsigned) columns)
   //    disp_cols = (unsigned) columns ;

   //  now find max width of filename listing
   // unsigned max_fcols = wincols / disp_cols ;
   line_len = wincols / disp_cols ;  //lint !e573
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

   //  see how many columns we can support with requested formats
   lfn_get_columns() ;  //  set disp_cols, name_width

   filehead() ;
   //  then list the files
   for(auto &file : flist) {
      ffdata *ftemp = &file ;
      lfn_fprint[columns](ftemp) ;  //  horizontal listing
      if (++j == disp_cols) {
         ncrlf() ;
         j = 0 ;
      } else {
         nput_char(n.colorframe, vline, 1) ;
      }
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
   int first_line = 1 ;
   TCHAR prev_ext[MAX_EXT_SIZE+1] ;

   prev_ext[0] = 0 ; //  make lint happy

   width = (is_redirected()) ? 80 : get_window_cols() ;
   // width = get_window_cols() ;

   //  first, find the longest extension in the current file list
   unsigned maxext = 0 ;
   for(auto &file : flist) {
      // ftemp = &file ;
      slen = file.ext.length() ;
      if (maxext < slen)
          maxext = slen ;
   }
   
   //  then list the files
   filehead() ;
   bool new_line = true ;
   for(auto &file : flist) {
      ftemp = &file ;
      //  see if file extention is changing
      if (first_line  ||  _tcsicmp(prev_ext, ftemp->ext.c_str()) != 0) {
         if (first_line)
            first_line = false ;
         else 
            ncrlf() ;
         _stprintf(tempstr, _T("%-*s: "), maxext, ftemp->ext.c_str()) ;   
         nputs(ftemp->color, tempstr) ;
         col = maxext+2 ;
         _tcscpy(prev_ext, ftemp->ext.c_str()) ;
         new_line = true ;
      }

      //  see if next filename is going to overrun line; 
      //  if so, start next line...
      // slen = (n.lfn_off) ? 9 : (_tcslen(ftemp->name) + 2) ;
      // slen = _tcslen(ftemp->name.c_str()) + 2 ;
      slen = ftemp->name.length() + 2 ;
      if (col + slen > width) {
         nputs((ftemp->dirflag) ? n.colordir : ftemp->color, _T(", ")) ;
         ncrlf() ;
         _stprintf (tempstr, _T("%*s  "), maxext, _T(" ")) ;
         nputs(ftemp->color, tempstr) ;
         col = maxext+2 ;
         new_line = true ;
      } 
      
      //  if not starting new line, add comma separator
      if (new_line) {
         new_line = false ;
      } else {
         nputs((ftemp->dirflag) ? n.colordir : ftemp->color, _T(", ")) ;
      }
      //  select appropriate color and print the filename
      // _stprintf(tempstr, (n.lfn_off) ? "%-8s " : "%s", ftemp->name) ;
      // nputs((ftemp->dirflag) ? n.colordir : ftemp->color, tempstr) ;
      nputs((ftemp->dirflag) ? n.colordir : ftemp->color, ftemp->name.c_str()) ;
      col += slen ;
      
   }
   ncrlf() ;
   fileend() ;
}

//*****************************************************************
//  DDM  06/20/25
//  Maybe we can do this more cleanly, and also make it more amenable
//  to conversion to vector class, by storing pointer to top of column
//  and number of elements in each column, in a struct.
//  then the pointer could be changed to a vector index later.
//  
//  On my full-screen console (474 columns x 82 rows)
//  (on 3840 x 1600 monitor)
//  we indeed got an error here, with MAX_COLUMNS: 10
//  disp_cols is too large (24 vs 10)
//*****************************************************************
struct vcolumn_elements_s {
   ffdata *ftemp {nullptr};
   uint rows {};
   uint top_col_idx {};
};
#define  MAX_COLUMNS    25
//lint -esym(551, vcolumns)  Symbol not accessed
static vcolumn_elements_s vcolumns[MAX_COLUMNS] ;

static void list_files_vertically(void)
{
   unsigned rows, partrows, j ;
   //************************************************
   //  now, start displaying files
   //************************************************
   lfn_get_columns() ;  //  set disp_cols, name_width
   
   if (disp_cols > MAX_COLUMNS) {
      _stprintf(tempstr, L"disp_cols is too large (%u vs %u)\n", disp_cols, MAX_COLUMNS);
      nputs(0x65, tempstr);
      syslog(L"disp_cols is too large (%u vs %u)\n", disp_cols, MAX_COLUMNS);
      return ;
   }
   
   // syslog(_T("filecount: %u\n"), filecount);
   rows = (unsigned) filecount / disp_cols ;
   partrows = (unsigned) filecount % disp_cols ;

   for (j=0; j< disp_cols ; j++) {
      vcolumns[j].rows = rows ;
   }
   for (j=0; j<partrows; j++) {
      vcolumns[j].rows++ ;
   }

   //  after partrows causes rows to be incremented for some columns,
   //  then actual overall value of rows needs to be incremented to reflect this.
   //  If there were no partrows, then rows is already correct.
   if (partrows > 0) {
      rows++ ;
   }
   
   //  new method: store top-of-column index
   vcolumns[0].top_col_idx = 0 ;
   for (j=1; j< disp_cols ; j++) {
      vcolumns[j].top_col_idx = vcolumns[j-1].top_col_idx + vcolumns[j-1].rows;
   }

   //************************************************
   // row 0:  0  10  20  30  39  48
   // row 1:  1  11  21  31  40  49
   // row 2:  2  12  22  32  41  50
   // row 3:  3  13  23  33  42  51
   // row 4:  4  14  24  34  43  52
   // row 5:  5  15  25  35  44  53
   // row 6:  6  16  26  36  45  54
   // row 7:  7  17  27  37  46  55
   // row 8:  8  18  28  38  47  56
   // row 9:  9  19  29

   vcolumns[0].ftemp = &flist[0] ;
   vcolumns[1].ftemp = &flist[0] ;
   filehead() ;   //  uses rows, columns
   j = 0 ;
   ffdata *ftemp ;
   uint idxFile ;
   unsigned fcount = 0 ;
   unsigned row_num = 0 ;
   // console->dputsf(L"\nrow %u  ", row_num);
   while (LOOP_FOREVER) {
      if (fcount < filecount) {
         idxFile = vcolumns[j].top_col_idx + row_num ;
         // console->dputsf(L"%u  ", idxFile);
         ftemp = &flist[idxFile] ;
         lfn_fprint[columns](ftemp) ; //  vertical listing  NOLINT
      }
      //  if no files left to display, fill in row with spaces
      else {
         // nput_char(n.colorframe, ' ', col_width[disp_cols]) ;
         nput_char(n.colorframe, _T(' '), line_len) ;
      }   

      //  draw separator characters as required
      fcount++ ;
      if (++j == disp_cols) {
         if (--rows == 0) {
            ncrlf() ;
            break;
         }
         j = 0 ;
         row_num++ ;
         // console->dputsf(L"\nrow %u  ", row_num);
         ncrlf() ;
            
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
   if (flist.empty()) {
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
