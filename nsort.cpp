//*****************************************************************
//  NSORT.CPP -   NDIR data-sorting routines                       
//*****************************************************************

#include <windows.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"

//lint -esym(762, stable_sort)  Redundantly declared symbol 'stable_sort()' previously declared at line 26
//lint -esym(864, flist)  Expression involving variable 'flist' possibly depends on order of evaluation

//*****************************************************************
void sort_filelist(void)
{
   //  reverse sort
   if (n.reverse) {
      switch (n.sort) {
      case SORT_EXT:
         // sort_files(sort_lfn_name_rev) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(b.name.c_str(), a.name.c_str()) < 0) ;
            } ) ;
         // sort_files(sort_ext_rev) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(b.ext.c_str(), a.ext.c_str()) < 0) ;
            } ) ;
         break;
      case SORT_NAME:
         // sort_files(sort_ext_rev) ;
         // sort_files(sort_lfn_name_rev) ;
         // sort_files(sort_filename_rev) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(b.filename.c_str(), a.filename.c_str()) < 0) ;
            // return b.filename.compare(a.filename);
            } ) ;
         break;
      case SORT_SIZE:
         // sort_files(sort_size_rev) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (b.fsize < a.fsize) ;
            } ) ;
         break;
      case SORT_DATE:
         // sort_files(sort_date_time_rev) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            LARGE_INTEGER a64, b64 ;
            a64.LowPart  = a.ft.dwLowDateTime ;
            a64.HighPart = a.ft.dwHighDateTime ;
            b64.LowPart  = b.ft.dwLowDateTime ;
            b64.HighPart = b.ft.dwHighDateTime ;
            return (b64.QuadPart < a64.QuadPart) ;
            } ) ;
         break;
      default:  break ; // make lint happy
      }
   }

   //  normal sort
   else {
      switch (n.sort) {
      case SORT_EXT:
         // sort_files(sort_lfn_name) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(a.name.c_str(), b.name.c_str()) < 0) ;
            } ) ;
         // sort_files(sort_ext) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(a.ext.c_str(), b.ext.c_str()) < 0) ;
            } ) ;
         break;
      case SORT_NAME:
         // sort_files(sort_ext) ;
         // sort_files(sort_lfn_name) ;
         // sort_files(sort_filename) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (_tcsicmp(a.filename.c_str(), b.filename.c_str()) < 0) ;
            // return a.filename.compare(b.filename);
            } ) ;
         break;
      case SORT_SIZE:
         // sort_files(sort_size) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            return (a.fsize < b.fsize) ;
            } ) ;
         break;
      case SORT_DATE:
         // sort_files(sort_date_time) ;
         std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
            LARGE_INTEGER a64, b64 ;
            a64.LowPart  = a.ft.dwLowDateTime ;
            a64.HighPart = a.ft.dwHighDateTime ;
            b64.LowPart  = b.ft.dwLowDateTime ;
            b64.HighPart = b.ft.dwHighDateTime ;
            return (a64.QuadPart < b64.QuadPart) ;
            // return (a.fsize < b.fsize) ;
            } ) ;
         break;
      default:  break ; // make lint happy
      }
   }

   if (n.dir_first) {
      // sort_files(sort_dir) ;
      std::stable_sort(flist.begin(), flist.end(), [](const ffdata& a, const ffdata& b) { 
         return (a.dirflag && !(b.dirflag)); 
         } ) ;
   }
}

