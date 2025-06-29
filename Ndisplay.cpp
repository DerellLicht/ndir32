//*****************************************************************
//  Copyright (c) 1998-2025 Derell Licht                       
//  NDISPLAY.CPP - NDIR display handlers                           
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

//  from NDIR.CPP
extern TCHAR *Version ;
extern TCHAR *ShortVersion;
extern unsigned name_width ;

//  this array is 64 elements, not 32 (== 2^5) 
//  because the volume_label bit (0x80) is not displayed.  
//  There are actually six bits in the field.
//  NOTE: This data array is not used any more, as we now support file attributes
//        which are not part of the conventional DOS set.
// static char attrstr[64][6] = {
//   "__s__", "__s_r", "__sh_", "__shr", /*  04,05,06,07  */
//   ...
// };

static TCHAR const monthstr[12][4] = { 
   _T("Jan"), _T("Feb"), _T("Mar"), _T("Apr"), _T("May"), _T("Jun"),
   _T("Jul"), _T("Aug"), _T("Sep"), _T("Oct"), _T("Nov"), _T("Dec")
};

/*****************************************************************/
static void ngotoxy (int x, int y)
{
   if (n.color) {
      dgotoxy (x, y);
   }
}

//**************************************************
//  linecnt = number of lines written (non-color).
//**************************************************
void display_logo (void)
{
   if (n.batch) {
      return;
   }

   if (n.clear && !is_redirected ()) {
      nclrscr ();
   }

   TCHAR vtemp[50];
   if (n.minimize) {
      _stprintf(vtemp, _T("%s/%u "), ShortVersion, get_build_size());
      ngotoxy (0, _where_y ());
      nputs (n.colorlogo, vtemp);
   }
   else {
      // ngotoxy ((80 - _tcslen (Version)) / 2, _where_y ());
      _stprintf(vtemp, _T("%s/%u "), Version, get_build_size());
      ngotoxy (0, _where_y ());
      nputs (n.colorlogo, vtemp);
      ncrlf ();
   }
   // nputs(n.colordefalt, ShortVersion) ;
   set_text_attr (n.colordefalt);
}

//************************************************************************
//  return final filename from symlink
//  this will only work with a 64-bit build
//************************************************************************
#ifdef USE_64BIT

static TCHAR *GetLinkTarget(TCHAR const * const symlink_name) 
{
   static TCHAR final_file[MAX_FILE_LEN+1] = _T("");
   // Define smart pointer type for automatic HANDLE cleanup.
   // typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,
   //                         decltype( &::CloseHandle )> FileHandle;
   // Open file for querying only (no read/write access).
   HANDLE hdl = CreateFile( symlink_name, 0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                 // &::CloseHandle );
   if ( hdl == INVALID_HANDLE_VALUE ) {
      // h.release();
      // throw std::runtime_error( "CreateFileW() failed." );
      _stprintf(final_file, _T("cannot create file"));
   }
   else {

      //  this function is only supported by 64-bit gcc toolchains
      const size_t requiredSize = GetFinalPathNameByHandle( hdl, NULL, 0,
                                                             FILE_NAME_NORMALIZED );
      if ( requiredSize == 0 ) {
         _stprintf(final_file, _T("dest file size is 0"));
      }
      else {
      // std::vector<wchar_t> buffer( requiredSize );
      GetFinalPathNameByHandle( hdl, final_file,
                                MAX_FILE_LEN,
                                FILE_NAME_NORMALIZED );
      }

   }
   return final_file;
}
#endif

//************************************************************************
void print1 (ffdata * fptr)
{
   // char mlstr[30] ;
   // int show_normal_info ;
   uchar SHRattr = fptr->attrib & 7;
   FILETIME lft;
   FileTimeToLocalFileTime (&(fptr->ft), &lft);
   SYSTEMTIME sdt ;
   FileTimeToSystemTime(&lft, &sdt) ;
   int secs  = sdt.wSecond ;
   int mins  = sdt.wMinute ;
   int hour  = sdt.wHour   ;
   int day   = sdt.wDay    ;
   int month = sdt.wMonth  ;
   long year = sdt.wYear   ;
   // int wlen = _tcslen(fptr->filename.c_str());
   // int slen = name_width - fptr->mb_len ;

   TCHAR attr[12];
   if (n.long_attr) {
      _stprintf(attr, _T("%08X "), fptr->attrib) ;
   } else {
      attr[0] = (fptr->attrib & FILE_ATTRIBUTE_ARCHIVE)        ? 'a' : '_' ;
      attr[1] = (fptr->attrib & FILE_ATTRIBUTE_DIRECTORY)      ? 'd' : '_' ;
      attr[2] = (fptr->attrib & FILE_ATTRIBUTE_SYSTEM)         ? 's' : '_' ;
      attr[3] = (fptr->attrib & FILE_ATTRIBUTE_HIDDEN)         ? 'h' : '_' ;
      attr[4] = (fptr->attrib & FILE_ATTRIBUTE_READONLY)       ? 'r' : '_' ;
      attr[5] = (fptr->attrib & FILE_ATTRIBUTE_REPARSE_POINT)  ? 'l' : '_' ;
      attr[6] = ' ';
      attr[7] = '\0';
   }
   uchar attrclr = n.colorattr;

   //  display directory entry
   if (fptr->dirflag) {
      _stprintf (tempstr, _T("%14s "), "");
      nputs (n.colorsize, tempstr);
      nputs (attrclr, attr);
      //_stprintf(tempstr, "%02d-%02d-%04lu ", month, day, year);
      _stprintf (tempstr, _T("%3s %02d, %04lu "), monthstr[month - 1], day, year);
      nputs (n.colordate, tempstr);
      _stprintf (tempstr, _T("%02d:%02d:%02d "), hour, mins, secs);
      nputs (n.colortime, tempstr);
         
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles) {
         // nputsw(n.colorSHR | SHRattr, fptr->filename.c_str(), wlen, fptr->mb_len);
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         // nputsw(n.colordir,           fptr->filename.c_str(), wlen, fptr->mb_len);
         nputs(n.colordir,           fptr->filename.c_str());
      }
   }

   //  display file entry
   else {
      //  show file size
      switch (n.size_display) {
      case 2:
         // fsize.convert(fptr->fsize / 1000000) ;
         _stprintf (tempstr, _T("%13s"), convert_to_commas(fptr->fsize / 1000000, NULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("M "));
         break;
      case 1:
         // fsize.convert(fptr->fsize / 1000) ;
         _stprintf (tempstr, _T("%13s"), convert_to_commas(fptr->fsize / 1000, NULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("K "));
         break;
      default:
         // fsize.convert(fptr->fsize) ;
         _stprintf (tempstr, _T("%14s "), convert_to_commas(fptr->fsize, NULL));
         nputs (n.colorsize, tempstr);
         break;
      }

      nputs (attrclr, attr);
      _stprintf (tempstr, _T("%3s %02d, %04lu "), monthstr[month - 1], day, year);
      nputs (n.colordate, tempstr);
      _stprintf (tempstr, _T("%02d:%02d:%02d "), hour, mins, secs);
      nputs (n.colortime, tempstr);

      //  display filename in appropriate color...
      //  _stprintf (tempstr, "%s ", fptr->filename);
      // if (SHRattr != 0 && n.showSHRfiles)
      //    nputs (n.colorSHR | SHRattr, tempstr);
      // else
      //    nputs (fptr->color, tempstr);
      if (SHRattr != 0 && n.showSHRfiles) {
         // nputsw(n.colorSHR | SHRattr, fptr->filename, wlen, fptr->mb_len);
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         // nputsw(fptr->color, fptr->filename, wlen, fptr->mb_len);
         nputs(fptr->color, fptr->filename.c_str());
      }
         
#ifdef USE_64BIT
      //  if this file is a symlink, try to display the actual file
      if ((fptr->attrib & FILE_ATTRIBUTE_REPARSE_POINT)) {
         ncrlf() ;
         nputs (n.colorsize, _T("               "));
         nputs (attrclr, _T("=====> "));
         TCHAR *lptr = GetLinkTarget(fptr->filename.c_str()) ;
         _stprintf (tempstr, _T("%s "), lptr);
         nputs (fptr->color, tempstr);
      }
#endif      
      if (fptr->is_link_file) {
         TCHAR szFilePath[MAX_PATH_LEN];
         if (read_shortcut_file(fptr, szFilePath)) {
            ncrlf() ;
            nputs (n.colorsize, _T("               "));
            nputs (attrclr, _T("=====> "));
            nputs (fptr->color, szFilePath);
         }
      }
   }
}

//*********************************************************
//  from FILELIST.CPP
//  global vars for file size calculations
//*********************************************************

//*********************************************************
void lfn_print2 (ffdata * fptr)
{
   uchar SHRattr = fptr->attrib & 7;

   FILETIME lft;
   FileTimeToLocalFileTime (&(fptr->ft), &lft);
   SYSTEMTIME sdt ;
   FileTimeToSystemTime(&lft, &sdt) ;
   // int secs  = sdt.wSecond ;
   int mins  = sdt.wMinute ;
   int hour  = sdt.wHour   ;
   int day   = sdt.wDay    ;
   int month = sdt.wMonth  ;
   long year = sdt.wYear   ;
   //  length of spaces in column, after filename
   int slen = name_width - fptr->mb_len ;

   ULONGLONG fsize = fptr->fsize;

   if (fptr->dirflag) {
      nputs (n.colordir, _T(" [DIR] "));
      _stprintf (tempstr, _T("%02d-%3s-%02d "), day, monthstr[month - 1],
         (int) (year % 100));
      nputs (n.colordate, tempstr);
      _stprintf (tempstr, _T("%02d:%02d "), hour, mins);
      nputs (n.colortime, tempstr);

      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(n.colordir, fptr->filename.c_str());
      }
   }
   else {
      //  print file size
      if (fsize > 99999999L  ||  n.size_display == 2) {
         _stprintf (tempstr, _T("%5u"), (uint) (fsize / 1000000ULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("M "));
      }
      else if (fsize > 999999L  ||  n.size_display == 1) {
         _stprintf (tempstr, _T("%5u"), (uint) (fsize / 1000ULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("K "));
      }
      else {
         _stprintf (tempstr, _T("%6u "), (uint) fsize);
         nputs (n.colorsize, tempstr);
      }

      _stprintf (tempstr, _T("%02d-%3s-%02d "), day, monthstr[month - 1],
         (int) (year % 100));
      nputs (n.colordate, tempstr);
      _stprintf (tempstr, _T("%02d:%02d "), hour, mins);
      nputs (n.colortime, tempstr);

      //  generate filename
      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(fptr->color, fptr->filename.c_str());
      }
   }
   nput_char(n.colorframe, _T(' '), slen) ;
}

/*****************************************************************/
void lfn_print4 (ffdata * fptr)
{
   ULONGLONG fsize = fptr->fsize;
   uchar SHRattr = fptr->attrib & 7;
   //  length of spaces in column, after filename
   int slen = name_width - fptr->mb_len ;
   
   // syslog(_T("%u, %u=%u-%u %s\n"), wlen, slen, name_width, fptr->mb_len, fptr->filename);

   if (fptr->dirflag) {
      nputs (n.colordir, _T(" [DIR] "));
      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(n.colordir, fptr->filename.c_str());
      }
   }
   else {
      //  print file size
      if (fsize > 99999999L  ||  n.size_display == 2) {
         _stprintf (tempstr, _T("%5u"), (uint) (fsize / 1000000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("M "));
      }
      else if (fsize > 999999L  ||  n.size_display == 1) {
         _stprintf (tempstr, _T("%5u"), (uint) (fsize / 1000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, _T("K "));
      }
      else {
         _stprintf (tempstr, _T("%6u "), (uint) fsize);
         nputs (n.colorsize, tempstr);
      }

      //  generate filename
      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(fptr->color, fptr->filename.c_str());
      }
   }
   nput_char(n.colorframe, _T(' '), slen) ;
}

/*****************************************************************/
void lfn_print6 (ffdata * fptr)
{
   //  length of spaces in column, after filename
   int slen = name_width - fptr->mb_len ;
   uchar SHRattr = fptr->attrib & 7;
   if (fptr->dirflag) {
      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(n.colordir, fptr->filename.c_str());
      }
   }
   else {
      if (SHRattr != 0 && n.showSHRfiles) {
         nputs(n.colorSHR | SHRattr, fptr->filename.c_str());
      }
      else {
         nputs(fptr->color, fptr->filename.c_str());
      }
   }
   nput_char(n.colorframe, _T(' '), slen) ;
}

