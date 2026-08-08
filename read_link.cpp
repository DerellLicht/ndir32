// https://stackoverflow.com/questions/45140281/display-real-path-of-lnk-shortcut-file-using-c
//  build: g++ -Wall read_link.cpp -o read_link.exe -lole32 -luuid
//  uuid.lib reqd for IID_IPersistFile

// #define  STAND_ALONE    1

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>   /* For IShellLink */
#include <string>
#include <tchar.h>

//lint -e10  Expecting '}'
//lint -esym(534, lstrcpynA)

#ifndef STAND_ALONE
#include "common.h"
#include "ndir32.h"
#endif

#ifdef STAND_ALONE
typedef unsigned int          uint ;

//**********************************************************************
//lint -esym(714, strip_newlines)
//lint -esym(759, strip_newlines)
//lint -esym(765, strip_newlines)
void strip_newlines(char *rstr)
{
   int slen = (int) _tcslen(rstr) ;
   while (1) {
      if (slen == 0)
         break;
      if (*(rstr+slen-1) == '\n'  ||  *(rstr+slen-1) == '\r') {
         slen-- ;
         *(rstr+slen) = 0 ;
      } else {
         break;
      }
   }
}

//*************************************************************
//lint -esym(714, get_system_message)
//lint -esym(759, get_system_message)
//lint -esym(765, get_system_message)
char *get_system_message(DWORD errcode)
{
   // static char msg[261] ;
   static char msg[1024] ;
   // int slen ;
   int result = (int) errcode ;
   if (result < 0) {
      result = -result ;
      errcode = (DWORD) result ;
      // wsprintfA(msg, "Win32: unknown error code %d", result) ;
      // return msg;
   }
   // if (errcode >= IP_STATUS_BASE  &&  errcode <= MAX_ICMP_ERR_STRING) {
   //    return aszSendEchoErr[errcode - IP_STATUS_BASE];
   // }
   // if (errcode >= SNMP_ERROR_FIRST  &&  errcode <= SNMP_ERROR_LAST) {
   //    return snmp_error_msgs[errcode - SNMP_ERROR_FIRST];
   // }

   LPVOID lpMsgBuf;
   DWORD dresult = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      errcode,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   if (dresult == 0) {
      DWORD glError = GetLastError() ;
      if (glError == 317) {   //  see comment at start of function
         sprintf(msg, "FormatMessage(): no message for error code %d", result) ;
      } else {
         sprintf(msg, "FormatMessage() failed: [%u], errcode %d", (uint) GetLastError(), result) ;
      }
      
   } else
   if (lpMsgBuf == NULL) {
      sprintf(msg, "NULL buffer in response from FormatMessage() [%u]", (uint) GetLastError()) ;
   } else 
   {
      _tcsncpy(msg, (char *) lpMsgBuf, sizeof(msg)) ;
      // Free the buffer.
      LocalFree( lpMsgBuf );  //lint !e534
   }

   //  trim the newline off the message before copying it...
   strip_newlines(msg) ;

   return msg;
}

//*************************************************************
//  each subsequent call to this function overwrites
//  the previous report.
//*************************************************************
char *get_system_message(void)
{
   return get_system_message(GetLastError());
}
#endif

/*********************************************************************
* Function......: ResolveShortcut
* Parameters....: lpszShortcutPath - string holding path and file name of a shortcut
*                 lpszFilePath - string that will hold a file name
*                 lpszArgsPath - string that will hold a arguments string
* Returns.......: S_OK on success, error code on failure
* Description...: Resolves a Shell link object (shortcut)
*********************************************************************/
HRESULT ResolveShortcut(/*in*/ LPCTSTR lpszShortcutPath,
                        /*out*/ LPTSTR lpszFilePath,
                        /*out*/ LPTSTR lpszArgsPath
                        )
{
   IShellLink* psl = NULL;

       // buffer that receives the null-terminated string
       // for the drive and path
   TCHAR szPath[MAX_PATH];
       // buffer that receives the null-terminated
       // string for the description
   // TCHAR szDesc[MAX_PATH];
   TCHAR szArgs[MAX_PATH];
       // structure that receives the information about the shortcut
   WIN32_FIND_DATA wfd;
   WCHAR wszTemp[MAX_PATH];

   lpszFilePath[0] = '\0';
   lpszArgsPath[0] = '\0';

   // Get a pointer to the IShellLink interface
   HRESULT hRes = CoCreateInstance(CLSID_ShellLink,
                           NULL,
                           CLSCTX_INPROC_SERVER,  //lint !e641
                           IID_IShellLink,
                           (void**)&psl);   //lint !e838

   if (SUCCEEDED(hRes)) {
      // Get a pointer to the IPersistFile interface
      IPersistFile*  ppf     = NULL;
      psl->QueryInterface(IID_IPersistFile, (void **) &ppf); //lint !e534

      // IPersistFile is using LPCOLESTR,
      // so make sure that the string is Unicode
#ifndef _UNICODE
      MultiByteToWideChar(CP_ACP, 0, lpszShortcutPath, -1, wszTemp, MAX_PATH); //lint !e534
#else
      wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
#endif
      // Open the shortcut file and initialize it from its contents
      hRes = ppf->Load(wszTemp, STGM_READ);
      if (SUCCEEDED(hRes)) {
         // Try to find the target of a shortcut,
         // even if it has been moved or renamed
         // hRes = psl->Resolve(NULL, SLR_UPDATE); //lint !e641
         // even if it has been moved or renamed.
         // NOTE: SLR_UPDATE alone can make Resolve() pop up a
         // "browse for target" dialog when the target is missing,
         // and since this is a console app with no message pump,
         // that dialog silently blocks forever. SLR_NO_UI suppresses
         // it; the timeout in the high word of fFlags (here 500ms)
         // bounds how long Resolve() will silently search before
         // giving up and returning a failure code instead of hanging.
         hRes = psl->Resolve(NULL, SLR_UPDATE | SLR_NO_UI | (500 << 16)); //lint !e641
         if (SUCCEEDED(hRes)) {
            // Get the path to the shortcut target.
            // NOTE: deliberately NOT passing SLGP_RAWPATH here.
            // Per docs, SLGP_RAWPATH returns "the raw path name...
            // something that might not exist" - i.e. it just echoes
            // back whatever path is stored in the .lnk's binary data,
            // with no filesystem verification at all. Combined with
            // Resolve() above being able to return S_OK even when it
            // could not actually verify/relocate the target (it just
            // gives up cleanly under SLR_NO_UI), that raw-echo path
            // is not trustworthy proof the target exists.
            hRes = psl->GetPath(szPath, MAX_PATH, &wfd, 0); //lint !e641
            if (FAILED(hRes)) {
               // printf("GetPath error\n");
               goto error_exit;
               // return hRes;
            }

            // Copy the path out to the caller BEFORE the existence
            // check below. Even if the target turns out to be missing,
            // the caller still gets the resolved path so it can be
            // reported (e.g. tagged as a dead/"LOST" link) rather than
            // discarded just because hRes ends up indicating failure.
            lstrcpyn(lpszFilePath, szPath, MAX_PATH);

            // Belt-and-suspenders: explicitly confirm the resolved
            // path exists on disk rather than trusting the HRESULT
            // chain above. GetFileAttributesW is the standard WinAPI
            // call for this.
            if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
               hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
               goto error_exit;
            }
            // printf("path: %s\n", szPath);

            // Get the description of the target
            // hRes = psl->GetDescription(szDesc, MAX_PATH);
            // if (FAILED(hRes))
            //     return hRes;
            // printf("desc: %s\n", szDesc);

            // Get the arguments of the target
            hRes = psl->GetArguments(szArgs, MAX_PATH);
            if (FAILED(hRes)) {
               // printf("GetArguments error\n");
               goto error_exit;
            }
            // printf("args: %s\n", szArgs);
            lstrcpyn(lpszArgsPath, szArgs, MAX_PATH);
         }
         // else {
         //    printf("Resolve error\n");
         // }
      }
      // else {
      //    printf("Load error\n");
      // }
   }
   // else {
   //    printf("CoCreateInstance error\n");
   // }
error_exit:   
   return hRes;
}

//*********************************************************************
#ifdef STAND_ALONE
int main(int argc, char** argv)
{
   char lnkfile[MAX_PATH+1] = "" ;
   int idx ;
   for (idx=1; idx<argc; idx++) {
      char *p = argv[idx];
      _tcsncpy(lnkfile, p, MAX_PATH);
      lnkfile[MAX_PATH] = 0 ;
   }
   
   if (lnkfile[0] == 0) {
      printf("Usage: read_link link_filename\n");
      return 1;
   }

   char szFilePath[MAX_PATH];
   char szArgsPath[MAX_PATH];

   CoInitialize(NULL);  //lint !e534
   // NOTE: HRESULT is an unsigned value!!
   // The HRESULT data type is the same as the SCODE data type.
   // An HRESULT value consists of the following fields:
   //  > A 1-bit code indicating severity, where zero represents success and 1 represents failure.
   //    Note: this means that this bit means HRESULT as signed int is negative on error.
   //  > A 4-bit reserved value.
   //  > An 11-bit code indicating responsibility for the error or warning, 
   //    also known as a facility code.
   //  > A 16-bit code describing the error or warning.
   
   HRESULT hRes = ResolveShortcut(lnkfile, szFilePath, szArgsPath);
   CoUninitialize();  //lint !e534
   
   if (hRes == 0) {
      if (szArgsPath[0] == 0) {
         printf("szFilePath: %s\n", szFilePath);
      }
      //  see if destination should be extracted from arguments
      // szArgsPath: /e,"D:\photos (all)\photos"
      else if (_tcsncmp(szArgsPath, "/e,", 3) == 0) {
         // printf("szArgsPath: %s\n", szArgsPath);
         //  see if quotes need to be removed from path
         char *fptr = &szArgsPath[3] ;
         if (*fptr == '"') {
            // printf("found a quote char\n");
            fptr++ ;
            char *tl = _tcsrchr(fptr, '"');
            if (tl != NULL) {
               *tl = 0 ;
            }
            
         }
         printf("szFilePath: %s\n", fptr);
      }
      else {
         printf("szFilePath: %s\n", szFilePath);
      }
   }
   else {
            // HRESULT: 0x80070002      
      // printf("HRESULT: 0x%08X\n", (unsigned) hRes);
      printf("%s: %s\n", lnkfile, get_system_message(hRes & 0xFFFF)) ;
   }
   puts("");
   return 0;
}  //lint !e818
#else
//*********************************************************************
bool read_shortcut_file(ffdata * fptr, TCHAR *shortcut_path)
{
   bool target_exists = false ;
   TCHAR szFilePath[MAX_PATH];
   TCHAR szArgsPath[MAX_PATH];
   *shortcut_path = 0 ; //  initially empty string
   
   //  we need to build a full path+filename string here
   TCHAR szFullSrcPath[MAX_PATH];
   _stprintf(szFullSrcPath, _T("%s\\%s"), base_path.c_str(), fptr->filename.c_str());   //lint !e560: argument no. 4 should be a pointer (and it is...)
   // syslog(_T("[%s]\n"), szFullSrcPath);

   CoInitialize(NULL);  //lint !e534
   // NOTE: HRESULT is an unsigned value!!
   // The HRESULT data type is the same as the SCODE data type.
   // An HRESULT value consists of the following fields:
   //  > A 1-bit code indicating severity, where zero represents success and 1 represents failure.
   //    Note: this means that this bit means HRESULT as signed int is negative on error.
   //  > A 4-bit reserved value. 
   //  > An 11-bit code indicating responsibility for the error or warning, 
   //    also known as a facility code.
   //  > A 16-bit code describing the error or warning.
   
   HRESULT hRes = ResolveShortcut(szFullSrcPath, szFilePath, szArgsPath);
   CoUninitialize();  //lint !e534
   
   if (hRes == 0) {
      if (szArgsPath[0] == 0) {
         // printf("szFilePath: %s\n", szFilePath);
         _tcscpy(shortcut_path, szFilePath);
      }
      //  see if destination should be extracted from arguments
      // szArgsPath: /e,"D:\photos (all)\photos"
      else if (_tcsncmp(szArgsPath, _T("/e,"), 3) == 0) {
         // printf("szArgsPath: %s\n", szArgsPath);
         //  see if quotes need to be removed from path
         TCHAR *sptr = &szArgsPath[3] ;
         if (*sptr == '"') {
            // printf("found a quote char\n");
            sptr++ ;
            TCHAR *tl = _tcsrchr(sptr, '"');
            if (tl != NULL) {
               *tl = 0 ;
            }
            
         }
         // printf("szFilePath: %s\n", sptr);
         _tcscpy(shortcut_path, sptr);
      }
      else {
         // printf("szFilePath: %s\n", szFilePath);
         _tcscpy(shortcut_path, szFilePath);
      }
      target_exists = true ;
      // syslog("link: %s\n", shortcut_path);
   }
   else if (szFilePath[0] != 0) {
      // Resolve/GetPath ran far enough to produce a path, but the
      // target failed the existence check (or some other step after
      // that failed) - hand the dead path back anyway so the caller
      // can report it (e.g. tagged "LOST") instead of losing the
      // information that a target used to be here.
      _tcscpy(shortcut_path, szFilePath);
      // target_exists stays false
   }
   return target_exists ;
}
#endif

