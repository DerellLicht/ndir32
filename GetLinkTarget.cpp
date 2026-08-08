//************************************************************************
//  GetLinkTarget.cpp - Resolve target filespecs from Windows symlink files
//  
//  <truth_in_advert>
//  This superb code was entirely written and researched by ClaudeAI, 
//  though Derell Licht helped extensively with debugging and analysis.
// </truth_in_advert>
//  
//************************************************************************
#include <windows.h>
#ifdef USE_64BIT
#include <fileapi.h>
#endif
#include <tchar.h>

#include "common.h"
#include "ndir32.h"

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
// unparseable and fall back to a generic message rather than guessing.
typedef struct _APPEXECLINK_REPARSE_BUFFER {
   ULONG  ReparseTag;
   USHORT ReparseDataLength;
   USHORT Reserved;
   ULONG  StringCount;
   WCHAR  StringList[1];
} APPEXECLINK_REPARSE_BUFFER;

#define MAX_REPARSE_BUFFER_SIZE (16 * 1024)

//*****************************************************************************
// Try to pull the real target path out of an APPEXECLINK reparse
// point's raw data. Returns true and fills out_target on success;
// returns false (leaving out_target untouched) if the buffer can't
// be read or doesn't look like the layout we expect.
//*****************************************************************************
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
            size_t maxLen = ((BYTE*)bufEnd - (BYTE*)p) / sizeof(WCHAR);  //  NOLINT(readability-redundant-casting) NOLINT(clang-analyzer-security.PointerSub)
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

//*****************************************************************************
TCHAR *GetLinkTarget(TCHAR const * const symlink_name) 
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

