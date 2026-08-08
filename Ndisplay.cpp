//*****************************************************************
//  Copyright (c) 1998-2026 Derell Licht                       
//  NDISPLAY.CPP - NDIR display handlers                           
//*****************************************************************

//lint -esym(767, _WIN32_WINNT)
//lint -e113  Inconsistent enum declaration

//  With tdm64 10.3.0, this is defined as 0x502
//  this constant is required in fileapi.h, for
//  GetFinalPathNameByHandle() function
// #define  _WIN32_WINNT   0x0600

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

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK 0x8000001BL
#endif

// On-disk layout for an IO_REPARSE_TAG_APPEXECLINK reparse point
// (UWP/MSIX "Application Execution Alias", used heavily under
// WindowsApps). NOT officially documented by Microsoft - this
// mirrors the AppExecLinkReparseBuffer member of REPARSE_DATA_BUFFER
// from the kernel-mode ntifs.h header, reverse-engineered/confirmed
// by various tool authors. Defined locally (rather than pulling in
// the full REPARSE_DATA_BUFFER union) to avoid any name collision
// and to keep this self-contained. StringList holds StringCount
// consecutive null-terminated WCHAR strings; empirically:
//   [0] package family name
//   [1] "<package family name>!<app id>"  (AUMID)
//   [2] the actual target executable path - what we want
// Since this is undocumented and could change, treat anything that
// doesn't look sane (too few/too many strings, bad lengths) as
// unparseable and fall back to a generic message rather than
// guessing.
typedef struct _APPEXECLINK_REPARSE_BUFFER {
   ULONG  ReparseTag;
   USHORT ReparseDataLength;
   USHORT Reserved;
   ULONG  StringCount;
   WCHAR  StringList[1];
} APPEXECLINK_REPARSE_BUFFER;

#define MAX_REPARSE_BUFFER_SIZE (16 * 1024)

// Try to pull the real target path out of an APPEXECLINK reparse
// point's raw data. Returns true and fills out_target on success;
// returns false (leaving out_target untouched) if the buffer can't
// be read or doesn't look like the layout we expect.
static bool GetAppExecLinkTarget(TCHAR const * const symlink_name,
                                  TCHAR *out_target, size_t out_target_chars)
{
   bool ok = false;

   // Need FILE_FLAG_OPEN_REPARSE_POINT to open the reparse point
   // ITSELF rather than have the OS try (and fail) to follow it,
   // plus FILE_FLAG_BACKUP_SEMANTICS which CreateFile requires for
   // opening this kind of special file.
   HANDLE hReparse = CreateFile(symlink_name, 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                        NULL);
   if (hReparse == INVALID_HANDLE_VALUE) {
      return false;
   }

   BYTE buffer[MAX_REPARSE_BUFFER_SIZE];
   DWORD bytesReturned = 0;
   if (DeviceIoControl(hReparse, FSCTL_GET_REPARSE_POINT, NULL, 0,
                        buffer, sizeof(buffer), &bytesReturned, NULL)) {

      APPEXECLINK_REPARSE_BUFFER *rb = (APPEXECLINK_REPARSE_BUFFER *) buffer;

      // Sanity checks before trusting any of this undocumented layout.
      const size_t headerSize = offsetof(APPEXECLINK_REPARSE_BUFFER, StringList);
      if (rb->ReparseTag == IO_REPARSE_TAG_APPEXECLINK &&
          bytesReturned > headerSize &&
          rb->StringCount >= 3 && rb->StringCount <= 16) {

         WCHAR *p = rb->StringList;
         BYTE  *bufEnd = buffer + bytesReturned;
         ULONG idx = 0;
         while (idx < rb->StringCount && (BYTE*)p < bufEnd) {
            // wcsnlen bounded by remaining buffer, in WCHARs
            size_t maxLen = ((BYTE*)bufEnd - (BYTE*)p) / sizeof(WCHAR);
            size_t len = wcsnlen(p, maxLen);
            if (len == maxLen) {
               // ran off the end without a null terminator - bail out
               break;
            }
            if (idx == 2) {
               // this is the string we want: the actual target path
#ifndef _UNICODE
               WideCharToMultiByte(CP_ACP, 0, p, -1, out_target,
                                    (int) out_target_chars, NULL, NULL);
#else
               wcsncpy(out_target, p, out_target_chars);
#endif
               ok = true;
               break;
            }
            p += len + 1;  // skip string + its null terminator
            idx++;
         }
      }
   }

   CloseHandle(hReparse);
   return ok;
}

static TCHAR *GetLinkTarget(TCHAR const * const symlink_name) 
{
   static TCHAR final_file[MAX_FILE_LEN+1] = _T("");
   // Define smart pointer type for automatic HANDLE cleanup.
   // typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,
   //                         decltype( &::CloseHandle )> FileHandle;

   // Reparse points come in several flavors with completely different
   // resolution mechanisms. Symlinks/junctions are transparently
   // followed by CreateFile(); UWP/MSIX "Application Execution
   // Aliases" (heavily used under WindowsApps) are NOT - they use a
   // separate tag, IO_REPARSE_TAG_APPEXECLINK, that's only resolved
   // by the process-creation reparse filter at launch time, not by
   // generic file opens. Check the tag first (via FindFirstFile,
   // which is cheap and doesn't require actually opening the file)
   // and, for that tag, pull the real target out of the reparse
   // point's raw data instead of trying (and failing) to CreateFile
   // through it.
   WIN32_FIND_DATA fd;
   HANDLE hFind = FindFirstFile(symlink_name, &fd);
   if (hFind != INVALID_HANDLE_VALUE) {
      FindClose(hFind);
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
          fd.dwReserved0 == IO_REPARSE_TAG_APPEXECLINK) {
         if (GetAppExecLinkTarget(symlink_name, final_file, MAX_FILE_LEN)) {
            return final_file;
         }
         // parsing didn't pan out (unexpected buffer shape) - fall
         // back to a clear, honest label rather than guessing
         _stprintf(final_file, _T("<app execution alias>"));
         return final_file;
      }
   }

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

      // NOTE: original code never closed this handle - small leak,
      // fixed here while touching this function.
      CloseHandle(hdl);
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
      //*****************************************************************************
      //  Steps for creating a Reparse Point for testing this
      // 
      // [in User console] mklink rev_link.txt ..\revisions.txt
      // This does *not* create a visible link, but...
      // [in Admin console] mklink rev_link.txt ..\revisions.txt
      // This creates the symlink, but it is only visible to Admin,
      // unless the User step was executed first.
      // 
      // Later note:
      // Actually, all of the above stuff may be completely irrelevant...
      // It may have all been caused by passing incomplete path to GetLinkTarget().
      // I should probably test it someday
      //*****************************************************************************
      if ((fptr->attrib & FILE_ATTRIBUTE_REPARSE_POINT)) {
         // syslog(L"We found reparse_point\n");
         ncrlf() ;
         nputs (n.colorsize, _T("               "));
         nputs (attrclr, _T("=====> "));
         // NOTE: GetLinkTarget() needs a full path, not just the bare
         // filename - it calls CreateFile()/FindFirstFile() on it
         // directly, which resolve relative to the process's cwd, not
         // the folder being listed. Without this, results were only
         // correct when ndir happened to be invoked FROM the folder
         // containing the reparse point (see read_shortcut_file(),
         // which already builds a full path the same way for .lnk files).
         TCHAR szFullSrcPath[MAX_PATH];
         _stprintf(szFullSrcPath, _T("%s\\%s"), base_path.c_str(), fptr->filename.c_str());
         TCHAR *lptr = GetLinkTarget(szFullSrcPath) ;
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
         else {
            ncrlf() ;
            nputs (n.colorsize, _T("               "));
            nputs (attrclr, _T("LOST=> "));
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

