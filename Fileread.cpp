//*****************************************************************
//  Copyright (c) 1998-2025 Daniel D. Miller                       
//  FILEREAD.CPP - NDIR file-reading routines
//                                                                 
//  Written by:  Daniel D. Miller
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "vector_res.h"

//lint -esym(759, getcolor) header declaration for symbol could be moved from header to module

//lint -esym(745, process_exclusions)  function has no explicit type or class, int assumed
//lint -esym(533, process_exclusions)  function should return a value (see line 200)

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

//*****************************************************************
//lint -esym(552, ftail)   Symbol not accessed
//lint -esym(759, ftail)   header declaration for symbol could be moved from header to module
ffdata *ftop = NULL ;
ffdata *ftail = NULL ;

//*********************************************************************
//lint -esym(759, add_element_to_file_list)  header declaration for symbol could be moved from header to module
void add_element_to_file_list(ffdata *ftemp)
{
   if (ftop == NULL)
      ftop = ftemp;
   else
      ftail->next = ftemp;
   ftail = ftemp;
}

//*********************************************************
//  This loops thru all files in one subdirectory,
//  calling update_filelist() to add files to the
//  global linked list.
//  This differs from read_files() in that it uses
//  the MSDOS 7.00 long-filename functions
//*********************************************************
static void const read_long_files (std::wstring& target_path)
{
   bool fn_okay ;  //, result;
   HANDLE handle;
   // TCHAR *strptr;
   ffdata *ftemp;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATA fdata ; //  long-filename file struct

   // syslog("%s\n", target[i]);
   handle = FindFirstFile (target_path.c_str(), &fdata);
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
         ftemp = (ffdata *) new ffdata ;
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

         ftemp->dirflag = ((ftemp->attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) ? true : false ;
         
         //  convert Unicode filenames to UTF8
         ftemp->mb_len = _tcslen(fdata.cFileName) ;
         ftemp->filename = fdata.cFileName ;
         
         //  find and extract the file extension, if valid
         size_t ext_length = ftemp->filename.length() ;
         size_t ext_dot = ftemp->filename.find_last_of(L".");
         if (ext_dot > 0) {
            ext_length = ext_length - ext_dot ;
         }
         else {
            ext_length = 0 ;
         }
         //  if ext_dot == npos (i.e. -1), then no extension is present
         //  if ext_dot == 0, then dot is at start of string, treat as no extension
         //  if extension length > MAX_EXT_SIZE, treat as no extension
         if (ext_dot == 0  ||  ext_dot == std::wstring::npos  ||  ext_length > MAX_EXT_SIZE) {
            ftemp->name = ftemp->filename;
            ftemp->ext = L"";
         }
         else {
            ftemp->name = ftemp->filename.substr(0, ext_dot);
            ftemp->ext  = ftemp->filename.substr(ext_dot);
         }
         // syslog(_T("%d/%d/%d: %s: [%s].[%s]\n"), 
         //    ext_dot, ext_length, ext_length-ext_dot, ftemp->filename.c_str(), ftemp->name.c_str(), ftemp->ext.c_str());
         
         //  If Steven Bensky's short filenames are requested,
         //  generate fully-qualified filenames so I can request the short name...
         //  V2.62, 01/08/24 - short-filename support removed

         //  look up color in table
         if (n.color) {
            getcolor (ftemp);
         }
         ftemp->next = NULL;

         //****************************************************
         //  add the structure to the file list
         //****************************************************
         add_element_to_file_list(ftemp);
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
   ffdata *ftemp ;
   for (int i = 0; i < exclcount; i++) {
      ftemp = ftop;
      ffdata *fprev = NULL;

      while (ftemp != NULL) { //lint !e449
         //  if we have a match, delete the second argument
         if (strcmpiwc (ftemp->ext.c_str(), excl[i]) != 0) {
            if (fprev == NULL) {
               ftop = ftop->next;
               // free(ftemp);
               delete ftemp ;
               ftemp = ftop;
            }
            else {
               fprev->next = ftemp->next;
               // free(ftemp);
               delete ftemp ;
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
   
   //  now update ftail to new end of list
   ftemp = ftop;
   while (ftemp != NULL) {
      ftail = ftemp ;
      ftemp = ftemp->next ;
   }
}

//*********************************************************
//                 File-listing routine
//*********************************************************
void file_listing (void)
{
   //***********************************************
   //  read all files matching one filespec
   //***********************************************
   // syslog(_T("start/finish: %u:%u, %u\n"), start, finish, target.size());
   //  n
   // [55896] start/finish: 0:0, 1
   //  n . ..
   // [76988] start/finish: 0:0, 2
   // [76988] start/finish: 1:1, 2
   //  n *.cpp *.h ..
   // [81152] start/finish: 0:0, 3
   // [81152] start/finish: 1:2, 3
   //   n *.cpp *.h .. c:\home   
   // [78752] start/finish: 0:0, 4
   // [78752] start/finish: 1:2, 4
   // [78752] start/finish: 3:3, 4
   
   //  this is called once for each folder in command list
   for (unsigned i = start; i <= finish; i++) {
      read_long_files (target[i]);
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

