//*****************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  FILEREAD.CPP - NDIR file-reading routines
//                                                                 
//  Written by:  Daniel D. Miller
//*****************************************************************

// #define  _WIN32_WINNT   0x0400
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"

//  from nsort.cpp
extern void sort_filelist (void);

//*****************  function prototypes  *****************
static void process_exclusions (void);
static void read_long_files (int i);

//*************************************************************
#define  MAX_EXCL_COUNT    20
static TCHAR excl[MAX_EXCL_COUNT][MAX_EXT_SIZE+1] ; //  allocate dynamically??
static int  exclcount = 0 ;     //  number of exclusion filespecs

//*************************************************************
void update_exclusion_list(TCHAR *extptr)
{
   if (exclcount < MAX_EXCL_COUNT) {
      _tcsncpy (excl[exclcount], extptr, MAX_EXT_SIZE);
      excl[exclcount][MAX_EXT_SIZE] = 0 ;
      exclcount++;
   }
}

//*********************************************************
//  This loops thru all files in one subdirectory,
//  calling update_filelist() to add files to the
//  global linked list.
//  This differs from read_files() in that it uses
//  the MSDOS 7.00 long-filename functions
//*********************************************************
static void read_long_files (int i)
{
   bool fn_okay ;  //, result;
   HANDLE handle;
   TCHAR *strptr;
   ffdata *ftemp;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATA fdata ; //  long-filename file struct

   // WCHAR wfilespec[MAX_PATH+1];
   // int result = MultiByteToWideChar(CP_ACP, 0, target[i], -1, wfilespec, (int) _tcslen(target[i])+1);
   // if (result == 0) {
   //    syslog("%s: a2u failed: %u\n", target[i], (unsigned) GetLastError());
   //    return ;
   // }
   
   // syslog("%s\n", target[i]);
   handle = FindFirstFile (target[i], &fdata);
   // handle = FindFirstFileW(wfilespec, &fdata);
   //  according to MSDN, Jan 1999, the following is equivalent to the preceding... 
   //  unfortunately, under Win98SE, it's not...
   // handle = FindFirstFileEx(target[i], FindExInfoStandard, &fdata, 
   //                      FindExSearchNameMatch, NULL, 0) ;
   if (handle == INVALID_HANDLE_VALUE) {
      syslog(_T("FindFirstFile: %s"), get_system_message());
      return;
   }

   //  loop on find_next
   bool done = false;
   while (!done) {
      if (n.show_all == 0) {
         if ((fdata.dwFileAttributes & 
            (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)) != 0) {
            // fn_okay = false ;
            goto search_next_file;
         }
      }
      //  filter out directories if not requested
      if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_VOLID) != 0)
         fn_okay = false;
      else if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
         fn_okay = true;
      //  everything past here is a folder   
      else if (n.tree == 2)     //  "files only" flag
         fn_okay = false;
      //  skip '.' and '..', but NOT .ncftp (for example)
      else if (_tcscmp(fdata.cFileName, _T("."))  == 0  ||
               _tcscmp(fdata.cFileName, _T("..")) == 0) {
         fn_okay = false;
      }
      else {
         fn_okay = true;
      }
         
      if (fn_okay) {
         filecount++;

         //****************************************************
         //  allocate and initialize the structure
         //****************************************************
         ftemp = (struct ffdata *) malloc(sizeof(ffdata)) ;
         if (ftemp == NULL) {
            error_exit (OUT_OF_MEMORY, NULL);
            return;             //  only to make lint happy
         }
         ZeroMemory((char *) ftemp, sizeof(ffdata));

         //  convert filename to lower case if appropriate
         // if (!n.ucase)
         //    strlwr(fblk.name) ;

         ftemp->attrib = (u16) fdata.dwFileAttributes;

         //  convert file time
         //luconv.l = IFF (n.fdate_option == 0)
         // ftemp->ft = IFF (n.fdate_option == 0)
         //       THENN fdata.ftLastAccessTime
         //       ELSSE fdata.ftCreationTime ;
         // ftemp->ft = fdata.ftLastWriteTime ;
         if (n.fdate_option == FDATE_LAST_ACCESS)
            ftemp->ft = fdata.ftLastAccessTime;
         else if (n.fdate_option == FDATE_CREATE_TIME)
            ftemp->ft = fdata.ftCreationTime;
         else
            ftemp->ft = fdata.ftLastWriteTime;

         //  convert file size
         u64toul iconv;
         iconv.u[0] = fdata.nFileSizeLow;
         iconv.u[1] = fdata.nFileSizeHigh;
         ftemp->fsize = iconv.i;

         //  convert Unicode filenames to UTF8
         ftemp->mb_len = _tcslen(fdata.cFileName) ;
         ftemp->filename = (TCHAR *) malloc((ftemp->mb_len + 1) * sizeof(TCHAR));  //lint !e732
         if (ftemp->filename == NULL) {
            error_exit (OUT_OF_MEMORY, NULL);
            // return;             //  only to make lint happy
         }
         _tcscpy (ftemp->filename, fdata.cFileName);  // NOLINT
         
         //  If Steven Bensky's short filenames are requested,
         //  generate fully-qualified filenames so I can request the short name...
         // if (n.lfn_off) {
         //    ftemp->filename = (char *) malloc(15) ;
         // 
         //    strptr = sfn_convert_filename((char *) fdata.cFileName);
         //    _tcscpy (ftemp->filename, strptr);
         // }
         // else {
         //    ftemp->filename = (char *) malloc(_tcslen ((char *) fdata.cFileName) + 1);
         //    _tcscpy (ftemp->filename, (char *) fdata.cFileName);
         // }

         //  find and extract the file extension, if valid
         // ftemp->name[0] = 0 ; //  don't use name at all
         uint fnlen = _tcslen (ftemp->filename);   // NOLINT
         ftemp->name = (TCHAR *) malloc((fnlen + 1) * sizeof(TCHAR)) ;
         if (ftemp->name == NULL) {
            error_exit (OUT_OF_MEMORY, NULL);
         }

         _tcscpy (ftemp->name, ftemp->filename);   // NOLINT
         strptr = _tcsrchr (ftemp->name, _T('.'));
         if (strptr != NULL && _tcslen (strptr) <= MAX_EXT_SIZE) {
            _tcscpy (ftemp->ext, strptr);
            *strptr = 0;        //  NULL-term name field
            
            //  12/12/23  Add handling for .lnk files
            if (_tcsicmp(ftemp->ext, _T(".lnk")) == 0) {
               ftemp->is_link_file = true ;
            }
         }
         else {
            ftemp->ext[0] = 0;  //  no extension found
         }

         //  look up color in table
         if (n.color)
            getcolor (ftemp);
         ftemp->dirflag = ftemp->attrib & FILE_ATTRIBUTE_DIRECTORY;
         ftemp->next = NULL;

         //****************************************************
         //  add the structure to the file list
         //****************************************************
         if (ftop == NULL)
            ftop = ftemp;
         else
            ftail->next = ftemp;
         ftail = ftemp;
      }  //  if file is parseable...

search_next_file:
      //  search for another file
      if (FindNextFile(handle, &fdata) == 0) {
         done = 1;
      }
   }

   FindClose (handle);
}

//*************************************************************
//  Compare file list against exclusion list, then remove 
//  excluded files.  For now, this will seek only extensions.
//*************************************************************
static void process_exclusions (void)
{
   for (int i = 0; i < exclcount; i++) {
      ffdata *ftemp = ftop;
      ffdata *fprev = NULL;

      while (ftemp != NULL) {
         //  if we have a match, delete the second argument
         if (strcmpiwc (ftemp->ext, excl[i]) != 0) {
            if (fprev == NULL) {
               ftop = ftop->next;
               free(ftemp);
               ftemp = ftop;
            }
            else {
               fprev->next = ftemp->next;
               free(ftemp);
               ftemp = fprev->next;
            }
            filecount--;
         }

         //  otherwise, just move to the next file
         else {
            fprev = ftemp;
            ftemp = ftemp->next;
         }
      }
   }
}

//*********************************************************
//                 File-listing routine
//*********************************************************
void file_listing (void)
{
   unsigned i;
   // merge_sort<struct ffdata> ms ;

   //***********************************************
   //  read all files matching one filespec
   //***********************************************
   for (i = start; i <= finish; i++) {
      read_long_files (i);
   }

   //***********************************************
   process_exclusions ();

   //***********************************************
   //              Sort directory data
   //***********************************************
   if (filecount > 0) {
      sort_filelist ();

      // if (n.dir_first)
      //    // sort_elements(sort_dir) ;
      //    ftop = sort_elements(sort_dir, ftop) ;
   }

   //  now do the file-listing...
   display_files ();
}

