//*****************************************************************
//  Copyright (c) 1995-2025  Daniel D Miller
//  TREELIST.CPP: Read directory tree                  
//*****************************************************************

//  remove this define to remove all the debug messages
// #define  DEBUG_LOG

#include <windows.h>
#include <stdio.h>
#include <conio.h>   //  kbhit()
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "treelist.h"

//************************************************************
#define STL_DIRPATH

#ifdef  STL_DIRPATH
static std::wstring dirpath {} ;
#else
static TCHAR dirpath[MAX_PATH_LEN];
#endif

unsigned level;

// std::vector<dirs> dlist {};
dirs dlist {};   //  top-level brothers will be unused

//*********************************************************
//  "waiting" pattern generator
//*********************************************************
static unsigned lrow, lcol, dircount ;

static void pattern_init(TCHAR *lstr)
{
   dircount = 0 ;
   lrow = _where_y() ;
   if (lstr == 0) {
      lcol = 0 ;
   } else {
      dprints(lrow, 0, lstr) ;
      lcol = _tcslen(lstr) ;
   }
}

static void pattern_reset(void)
{
   dgotoxy(0, lrow) ;
   dclreol() ;
   dgotoxy(0, lrow) ;
}

static void pattern_update(void)
{
   static TCHAR pucount[20] ;

   dircount++ ;
   _stprintf(pucount, _T("%u"), dircount) ;
   dprints(lrow, lcol, pucount) ;
}

//**********************************************************
//  recursive routine to read directory tree
//**********************************************************
static int read_dir_tree (dirs * cur_node)
{
   uint idx ;
   bool early_abort = false ;
   // TCHAR *strptr;
   HANDLE handle;
   int slen, done, result;
   DWORD err;
   ULONGLONG file_clusters, clusters;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATA fdata ; //  long-filename file struct

   if (((dircount % 50) == 0)  &&  _kbhit()) {
      result = _getch() ;
      //  check for ESCAPE character
      if (result == ESC) {
         // error_exit(DATA_OKAY, NULL) ;
         early_abort = true ;
      }
   }
   //  if early_abort flag gets set, return with "success" flag,
   //  but without reading anything further...
   if (early_abort) {
      return 0;
   }

#ifdef  DEBUG_LOG
   syslog(L"read_dir_tree: %s\n", cur_node->name.c_str()) ;
#endif

   pattern_update() ;
   //  Insert next subtree level.
   //  if level == 0, this is first call, 
   //  and dirpath is already complete
   //  
   //  NOTE: slen that comes out of this block,
   //  is used at the end to restore original dirpath contents.
   if (level > 0) {
#ifdef  STL_DIRPATH
      slen = dirpath.find_last_of(L'\\');
      slen++ ;
      // dirpath[slen] = 0 ;  // string class does not know you've done this
      dirpath.resize(slen);
      slen = dirpath.length() ;
      dirpath.append(cur_node->name.c_str());
      dirpath.append(L"\\*");
#else   
      //  insert new path name
      TCHAR *strptr;
      strptr = _tcsrchr (dirpath, _T('\\'));
      strptr++;
      *strptr = 0;
      slen = _tcslen (dirpath);
      _tcscat (dirpath, cur_node->name.c_str());
      _tcscat (dirpath, _T("\\*"));
#endif      
   }
   
   else {
      slen = dirpath.length() ;
   }

   //  first, build tree list for current level
   level++;

#ifdef  DEBUG_LOG
   syslog(_T("L%u %s: entry\n"), level, dirpath.c_str()) ;  //  debug dump
#endif
   err = 0;
   handle = FindFirstFile(dirpath.c_str(), &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      err = GetLastError ();
      if (err == ERROR_ACCESS_DENIED) {
#ifdef  DEBUG_LOG
         syslog(L"FindFirstFile access denied\n") ;
#endif
         ;                     //  continue reading
      }
      else {
#ifdef  DEBUG_LOG
         syslog(_T("%s: FindFindFirst: %s\n"), dirpath.c_str(), get_system_message (err));
#endif
         return err ;
      }
   }

   //  loop on find_next
   done = 0;
   while (!done) {
      if (!err) {
         //  we found a directory
         if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            bool cut_dot_dirs;

            //  skip '.' and '..', but NOT .ncftp (for example)
            if (_tcscmp(fdata.cFileName, _T("."))  == 0  ||
                _tcscmp(fdata.cFileName, _T("..")) == 0) {
               cut_dot_dirs = true;
            }
            else if (n.tree_no_git  &&  _tcscmp(fdata.cFileName, _T(".git")) == 0) {
               cut_dot_dirs = true;
            }
            else {
               cut_dot_dirs = false;
            }

            if (!cut_dot_dirs) {
               cur_node->directs++;
               cur_node->subdirects++;

               // dirs *dtemp = new_dir_node ();
               
               cur_node->brothers.emplace_back();
               // cur_node->son[0].brothers.emplace_back();
               idx = cur_node->brothers.size() - 1 ;
               dirs *dtemp = &cur_node->brothers[idx] ;
               dtemp->dirsecsize = clbytes;
               dtemp->subdirsecsize = clbytes;
               dtemp->name = fdata.cFileName ;
               dtemp->attrib = (uchar) fdata.dwFileAttributes;
               // dtail->directs++ ;
            }                   //  if this is not a DOT directory
         }                      //  if this is a directory

         //  we found a normal file
         else {
            // printf("%9ld %04X %s\n", fdata.file_size, fdata.attrib, fdata.fname) ;
            //  convert file size
            u64toul iconv;
            iconv.u[0] = fdata.nFileSizeLow;
            iconv.u[1] = fdata.nFileSizeHigh;
            cur_node->dirsize += iconv.i;
            cur_node->subdirsize += iconv.i;

            clusters = iconv.i / clbytes;
            if ((iconv.i % clbytes) > 0)  //lint !e79
               clusters++;

            file_clusters = clusters * clbytes;
            cur_node->dirsecsize += file_clusters;
            cur_node->subdirsecsize += file_clusters;

            cur_node->files++;
            cur_node->subfiles++;
         }                      //  if entry is a file
      }                         //  if no errors detected

      //******************************************
      //  search for another file
      //******************************************
      if (FindNextFile(handle, &fdata) == 0) {
         // done = 1;
         err = GetLastError ();
         if (err == ERROR_ACCESS_DENIED) {
#ifdef  DEBUG_LOG
            syslog(_T("%s: FindNextFile: access denied\n"), dirpath.c_str()));
#endif
            ;                     //  continue reading
         }
         else if (err == ERROR_NO_MORE_FILES) {
#ifdef  DEBUG_LOG
            syslog(_T("FindNextFile: no more files\n")) ;
#endif
            done = 1 ;
         }
         else {
#ifdef  DEBUG_LOG
            syslog(_T("FindNextFile: %s: %s\n"), dirpath.c_str()), get_system_message (err));
#endif
            done = 1 ;
         }
      } else {
         err = 0 ;
      }
   }  //  while reading files from directory

#ifdef  DEBUG_LOG
   syslog(_T("%s: close\n"), dirpath.c_str());
#endif
   FindClose (handle);

   //  next, build tree lists for subsequent levels (recursive)
   for(auto &file : cur_node->brothers) {
      dirs *ktemp = &file;
      read_dir_tree (ktemp);
      cur_node->subdirsize    += ktemp->subdirsize;
      cur_node->subdirsecsize += ktemp->subdirsecsize;
      cur_node->subfiles      += ktemp->subfiles;
      cur_node->subdirects    += ktemp->subdirects;
   }

   //  when done, strip name from path and restore '\*.*'
#ifdef  STL_DIRPATH
   // dirpath[slen] = 0 ;  //  string class doesn't know this was done
   dirpath.resize(slen);
   dirpath.append(L"*");
#else
   _tcscpy (&dirpath[slen], _T("*"));  //lint !e669  string overrun??
#endif   

   //  restore the level number
   level--;
   return 0;   //lint !e438
}
//*********************************************************
static bool const tree_sort_name (dirs const a, dirs const b)
{
   return (_tcsicmp (a.name.c_str(), b.name.c_str()) < 0) ;
}

//*********************************************************
static bool const tree_sort_name_rev (dirs const a, dirs const b)
{
   return (_tcsicmp (b.name.c_str(), a.name.c_str()) < 0);
}

//*********************************************************
static bool const tree_sort_size (dirs const a, dirs const b)
{
   return (a.subdirsecsize < b.subdirsecsize) ;
}

//*********************************************************
static bool const tree_sort_size_rev (dirs const a, dirs const b)
{
   return (b.subdirsecsize < a.subdirsecsize) ;
}

//***********************************************************************************
//  debug function
//***********************************************************************************
// static void dump_brothers(std::vector<dirs> brothers, uint level, wchar_t *msg)
// {
//    if (brothers.size() < 2) {
//       return ;
//    }
//    if (msg != NULL) {
//       syslog(L"%u: %s\n", level, msg);
//    }
//    for(auto &file : brothers) {
//       dirs *ktemp = &file;
//       syslog(L"%10u %s\n", ktemp->subdirsecsize, ktemp->name.c_str()) ;
//    }  //  while not done traversing brothers
//    syslog(L"\n");
// }

//***********************************************************************************
//  recursive routine to traverse all branches of folder tree.
//  
//  vector mode:
//  Each brother passed to this function, will print his name and info, 
//  Then iterate over each of his children(brother->brothers),
//  and let them repeat the story.
//  
//  Thus, each folder listing will be followed by all lower folder listings...
//  AKA, depth-first traversal
//  
//  Note: parent_name is only used for debugging
//***********************************************************************************
//lint -esym(715, parent_name)   Symbol not referenced
static void sort_trees (std::vector<dirs>& brothers, TCHAR *parent_name)
{
   if (brothers.empty()) {
      return;
   }
   
   // dump_brothers(brothers, level, L"before sort");
   //  if there is only one brother in this folder, skip the sorting step,
   //  because there is nothing to do...
   //  However, the subsequent iteration over brothers still needs to run,
   //  as there may be larger folder sets down there.
   uint num_folders = brothers.size() ;
   // console->dputsf(L"found branch with %u brothers, under %s\n", num_folders, parent_name) ;
   // syslog(_T("L%u %s sort, %u elements\n"), level, parent_name, brothers.size()) ;
   if (num_folders > 1) {
      // std::sort(brothers.begin(), brothers.end(), comp);
      if (n.reverse) {
         if (n.sort == SORT_SIZE) {
            std::sort(brothers.begin(), brothers.end(), tree_sort_size_rev);
         }
         else {
            std::sort(brothers.begin(), brothers.end(), tree_sort_name_rev);
         }
      }

      //  normal sort
      else {
         if (n.sort == SORT_SIZE) {
            std::sort(brothers.begin(), brothers.end(), tree_sort_size);
         }
         else {
            std::sort(brothers.begin(), brothers.end(), tree_sort_name);
         }
      }
   }
   // dump_brothers(brothers, level, L"after sort");
   
   //  recurse over the brothers below this folder
   for(auto &file : brothers) {
      dirs *ktemp = &file;
      // console->dputsf(L"%s %s\n", formstr, ktemp->name.c_str()) ;

      level++;
      sort_trees(ktemp->brothers, (TCHAR *) ktemp->name.c_str());
      level-- ;
   }  //  while not done traversing brothers
   
   // dump_brothers(brothers, level, L"after recursion");
}

//**********************************************************
static int build_dir_tree (TCHAR *tpath)
{
   int result ;
   level = 0;

   //  Extract base path from first filespec,
   //  and strip off filename
   base_path = tpath ;
   size_t slen = base_path.find_last_of(L'\\');
   if (slen != std::wstring::npos) {
      slen++ ;
      base_path.resize(slen);
   }
   get_disk_info ((TCHAR *) base_path.c_str());

   //  allocate struct for dir listing
   dlist.brothers.emplace_back();
   dirs *dtemp = &dlist.brothers[0] ;
   dtemp->dirsecsize = clbytes;
   dtemp->subdirsecsize = clbytes;

   //  derive root path name
   if (base_path.length() == 3) {
      dtemp->name = L"<root>" ;
   }
   else {
      std::wstring ltempstr {};
      ltempstr = base_path ;
      slen = ltempstr.find_last_of(L'\\');
      if (slen != std::wstring::npos) {
         slen++ ;
         ltempstr.resize(slen);
      }
      dtemp->name = ltempstr ;
   }
   // syslog(L"top: %s\n", dtemp->name.c_str());

   // if (n.ucase)
   //    _tcsupr (top->name);

   dirpath = tpath ;

   pattern_init(_T("wait; reading directory ")) ;
   result = read_dir_tree (dtemp);
#ifdef  DEBUG_LOG
syslog(_T("read_dir_tree exit\n")) ;
#endif
   pattern_reset() ;
   return result ;
}

//*****************************************************************
void tree_listing (unsigned total_filespec_count)
{
   for (unsigned l = 0; l < total_filespec_count; l++) {
      //  read and build the dir tree
      build_dir_tree ((TCHAR *) target[l].c_str()) ;

      //  sort the tree list
      //  show the tree that we read
      dirs *temp = &dlist.brothers[0] ;   //  only needed for (debug) name display
      sort_trees(dlist.brothers, (TCHAR *) temp->name.c_str());

      //  now display the resulting directory tree
      draw_dir_tree ();
   }
}

