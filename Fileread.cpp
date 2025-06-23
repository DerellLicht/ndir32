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
#include "conio32.h"

//lint -esym(759, getcolor) header declaration for symbol could be moved from header to module

//lint -esym(745, process_exclusions)  function has no explicit type or class, int assumed
//lint -esym(533, process_exclusions)  function should return a value (see line 200)

//*************************************************************
#define  MAX_EXCL_COUNT    20
static TCHAR excl[MAX_EXCL_COUNT][MAX_EXT_SIZE+1] ; //  allocate dynamically??
static int  exclcount = 0 ;     //  number of exclusion filespecs

std::vector<ffdata> flist {};
//*************************************************************
//lint -esym(533, update_exclusion_list)
//lint -esym(745, update_exclusion_list)
void update_exclusion_list(TCHAR *extptr)
{
   if (exclcount < MAX_EXCL_COUNT) {
      _tcsncpy (excl[exclcount], extptr, MAX_EXT_SIZE);
      excl[exclcount][MAX_EXT_SIZE] = 0 ;
      exclcount++;
   }
}

//*********************************************************************
//  delete contents of existing file list
//  If tcount == 1, there *cannot* be multiple filespecs, can there??
//*********************************************************************
//lint -esym(532, clear_existing_file_list)
//lint -esym(533, clear_existing_file_list)
//lint -esym(714, clear_existing_file_list)
//lint -esym(745, clear_existing_file_list)
//lint -esym(759, clear_existing_file_list)
//lint -esym(765, clear_existing_file_list)
void clear_existing_file_list(void)
{
   flist.clear() ;
   filecount = 0 ;
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

         //****************************************************
         //  allocate and initialize the structure
         //****************************************************
         flist.emplace_back();
         uint idx = flist.size() - 1 ;
         ftemp = &flist[idx] ;
         filecount++;
         
         ftemp->attrib = (u16) fdata.dwFileAttributes;

         //  convert file time
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
   // dump_target(_T("sorted element(s)\n"));
   for (int idxExcl = 0; idxExcl < exclcount; idxExcl++) {
      
      unsigned idxHead ;
      unsigned ltcount = target.size() ;
      //  head index should iterate over all elements *except* the last one,
      //  since the last element in list would not have any others to compare against.
      for (idxHead=0 ; idxHead< ltcount ; idxHead++) {
         // ftemp = flist[idxHead];
try_next_element:
         if (strcmpiwc (flist[idxHead].ext.c_str(), excl[idxExcl])) {
            //  Scan file name and extension for equality.
            //  If both filename and extension are equal, delete one.
            flist.erase(flist.begin()+idxHead) ;
            ltcount-- ;
            //  we don't want to increment idxHead after deleting element;
            //  it is now pointing to the next element in array
            if (idxHead < ltcount) {
               goto try_next_element ;
            }
         }
      }  //lint !e850 
      // syslog(_T("target size: %u elements\n"), target.size());
   }
   // dump_target(_T("erased element(s)\n"));
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
   }

   //  now do the file-listing...
   display_files ();
}

