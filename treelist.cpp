//*****************************************************************
//  Copyright (c) 1995-2025  Daniel D Miller
//  TREELIST.CPP: Read directory tree                  
//*****************************************************************

//  remove this define to remove all the debug messages
// #define  DESPERATE

#include <windows.h>
#include <stdio.h>
#include <conio.h>   //  kbhit()
#ifdef _lint
#include <malloc.h>
#endif
#include <tchar.h>

#include "common.h"
#include "ndir32.h"
#include "conio32.h"
#include "treelist.h"

//************************************************************
static TCHAR dirpath[PATH_MAX];
unsigned level;

//************************************************************
extern void draw_dir_tree (void);

//************************************************************
//  the following object is a dummy point structure
//  which is used by merge_sort.  The main code must
//  allocate a strucure for this to point to.
//  
//  A global function pointer is also required by the
//  sort routine.  This will point to a function which
//  accepts two structure pointers as arguments, and
//  return:
//  
//     >0 if a > b
//    ==0 if a == b
//     <0 if a < b
//  
//************************************************************
static struct dirs *z = NULL;
static int (*tree_sort_fcn) (struct dirs * a, struct dirs * b);
static int tree_init_sort (void);

dirs *top = NULL;

//*****************************************************************
//  this was used for debugging directory-tree read and build
//*****************************************************************
#ifdef  DESPERATE
void debug_dump(TCHAR *fname, TCHAR *msg)
{
   syslog(_T("L%u %s: %s\n"), level, fname, msg) ;  //  debug dump
}

#endif

//**********************************************************
//  allocate struct for dir listing                         
//  NOTE:  It is assumed that the caller will               
//         initialize the name[], ext[], attrib fields!!    
//**********************************************************
static dirs *new_dir_node (void)
{
   //  switching this statement from malloc() to new()
   //  changes total exe size from 70,144 to 179,200 !!!
   //   70144 ->     32256   45.99%    win64/pe     ndir64.exe
   dirs *dtemp = (dirs *) malloc(sizeof(dirs)) ;
   if (dtemp == NULL) {
      error_exit (OUT_OF_MEMORY, NULL);
   }
   //     179200 ->     72704   40.57%    win64/pe     ndir64.exe
   // dirs *dtemp = new dirs ;
   ZeroMemory(dtemp, sizeof (struct dirs));  //lint !e668 NOLINT
   // memset ((char *) dtemp, 0, sizeof (struct dirs));  //lint !e668
   dtemp->dirsecsize = clbytes;
   dtemp->subdirsecsize = clbytes;
   return dtemp;
}

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
//  from fileread.cpp
static int early_abort = 0 ;

static int read_dir_tree (dirs * cur_node)
{
   dirs *dtail = 0;
   TCHAR *strptr;
   HANDLE handle;
   int slen, done, result;
   DWORD err;
   ULONGLONG file_clusters, clusters;
   // WIN32_FIND_DATA fdata ; //  long-filename file struct
   WIN32_FIND_DATA fdata ; //  long-filename file struct

   if (((dircount % 50) == 0)  &&  _kbhit()) {
      result = _getch() ;
      //  check for ESCAPE character
      if (result == 27) {
         // error_exit(DATA_OKAY, NULL) ;
         early_abort = 1 ;
      }
   }
   //  if early_abort flag gets set, return with "success" flag,
   //  but without reading anything further...
   if (early_abort) 
      return 0;

   pattern_update() ;
   //  Insert next subtree level.
   //  if level == 0, this is first call, and
   //  dirpath is already complete
   if (level > 0) {
      //  insert new path name
      strptr = _tcsrchr (dirpath, _T('\\'));
      strptr++;
      *strptr = 0;
      slen = _tcslen (dirpath);
      _tcscat (dirpath, cur_node->name);
      _tcscat (dirpath, _T("\\*"));
   }
   else {
      slen = _tcslen (dirpath);
   }

   //  first, build tree list for current level
   level++;

#ifdef  DESPERATE
debug_dump(dirpath, "entry") ;
#endif
   err = 0;
   // TCHAR wfilespec[MAX_PATH+1];
   // result = MultiByteToWideChar(CP_ACP, 0, dirpath, -1, wfilespec, (int) _tcslen(dirpath)+1);
   // if (result == 0) {
   //    syslog("%s: a2u failed: %u\n", dirpath, (unsigned) GetLastError());
   //    return -1;
   // }
   
   handle = FindFirstFile(dirpath, &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      err = GetLastError ();
      if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(dirpath, "FindFirstFile denied") ;
#endif
         ;                     //  continue reading
      }
      else {
#ifdef  DESPERATE
syslog(_T("%s: FindFindFirst: %s\n"), dirpath, get_system_message (err));
#endif
         // _stprintf (tempstr, "path [%s]\n", dirpath);
         // nputs (0xA, tempstr);
         // _stprintf (tempstr, "FindFirst: %s\n", get_system_message ());
         // nputs (0xA, tempstr);
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
            else {
               cut_dot_dirs = false;
            }

            if (!cut_dot_dirs) {
               cur_node->directs++;
               cur_node->subdirects++;

               dirs *dtemp = new_dir_node ();
               if (cur_node->sons == NULL)
                  cur_node->sons = dtemp;
               else
                  dtail->brothers = dtemp;   //lint !e613  NOLINT
               dtail = dtemp;
               // if (!n.ucase) 
               //    strlwr(ff.name) ;
               
               //  convert Unicode filenames to UTF8
               dtemp->mb_len = _tcslen(fdata.cFileName) ;
               dtemp->name = (TCHAR *) malloc((dtemp->mb_len + 1) * sizeof(TCHAR));  //lint !e732
               if (dtemp->name == NULL) {
                  error_exit(OUT_OF_MEMORY, NULL);
               }
               _tcscpy (dtemp->name, (TCHAR *) fdata.cFileName);  //  NOLINT

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
#ifdef  DESPERATE
debug_dump(fdata.cFileName, L"denied") ;
#else
            ;                     //  continue reading
#endif
         }
         else if (err == ERROR_NO_MORE_FILES) {
#ifdef  DESPERATE
syslog(_T("FindNextFileW: no more files\n")) ;
#endif
            done = 1 ;
         }
         else {
#ifdef  DESPERATE
_stprintf (tempstr, _T("FindNext: %s\n"), get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
            done = 1 ;
         }
      } else {
         err = 0 ;
      }
   }  //  while reading files from directory

#ifdef  DESPERATE
debug_dump(dirpath, "close") ;
#endif
   FindClose (handle);

   //  next, build tree lists for subsequent levels (recursive)
   dirs *ktemp = cur_node->sons;
   while (ktemp != NULL) {
#ifdef  DESPERATE
debug_dump(ktemp->name, "call read_dir_tree") ;
#endif
      read_dir_tree (ktemp);
      cur_node->subdirsize += ktemp->subdirsize;
      cur_node->subdirsecsize += ktemp->subdirsecsize;

      cur_node->subfiles += ktemp->subfiles;
      cur_node->subdirects += ktemp->subdirects;
      ktemp = ktemp->brothers;
   }

   //  when done, strip name from path and restore '\*.*'
   _tcscpy (&dirpath[slen], _T("*"));  //lint !e669  string overrun??

   //  restore the level number
   level--;
   return 0;
}

//****************************************************
//  allocate a dummy structure for merge_sort()
//****************************************************
static int tree_init_sort (void)
{
   //  switching this statement from malloc() to new()
   //  changes total exe size from 70,144 to 179,200 !!!
   //   70144 ->     32256   45.99%    win64/pe     ndir64.exe
   z = (dirs *) malloc(sizeof(dirs)) ;
   if (z == NULL)
      error_exit (OUT_OF_MEMORY, NULL);
   //     179200 ->     72704   40.57%    win64/pe     ndir64.exe
   // z = (dirs *) new dirs ;
   z->sons = NULL;
   z->brothers = NULL;
   return DATA_OKAY;
}

//****************************************************
// static void free_tree_structs(void)
//    {
//    if (z != NULL)  delete z ;
//    }

//*********************************************************
static int tree_sort_name (struct dirs *a, struct dirs *b)
{
   return (_tcsicmp (a->name, b->name));
}

//*********************************************************
static int tree_sort_name_rev (struct dirs *a, struct dirs *b)
{
   return (_tcsicmp (b->name, a->name));
}

//*********************************************************
static int tree_sort_size (struct dirs *a, struct dirs *b)
{
   if (a->subdirsecsize > b->subdirsecsize)
      return (1);
   else if (b->subdirsecsize > a->subdirsecsize)
      return (-1);
   else
      return (0);
}

//*********************************************************
static int tree_sort_size_rev (struct dirs *a, struct dirs *b)
{
   if (b->subdirsecsize > a->subdirsecsize)
      return (1);
   else if (a->subdirsecsize > b->subdirsecsize)
      return (-1);
   else
      return (0);
}

//*********************************************************
//  This routine merges two sorted linked lists.
//*********************************************************
static struct dirs *tree_merge (struct dirs *a, struct dirs *b)
{
   struct dirs *c;
   c = z;

   do {
      int x = tree_sort_fcn (a, b);
      if (x <= 0) {
         c->brothers = a;
         c = a;
         a = a->brothers;
      }
      else {
         c->brothers = b;
         c = b;
         b = b->brothers;
      }
   }
   while ((a != NULL) && (b != NULL));

   if (a == NULL)
      c->brothers = b;
   else
      c->brothers = a;
   return z->brothers;
}

//*********************************************************
//  This routine recursively splits linked lists
//  into two parts, passing the divided lists to
//  merge() to merge the two sorted lists.
//*********************************************************
static struct dirs *tree_merge_sort (struct dirs *c)
{
   struct dirs *a, *b, *prev;
   int pcount = 0;
   int j = 0;

   if ((c != NULL) && (c->brothers != NULL)) {
      a = c;
      while (a != NULL) {
         pcount++;
         a = a->brothers;
      }
      a = c;
      b = c;
      prev = b;
      while (j < pcount / 2) {
         j++;
         prev = b;
         b = b->brothers;
      }
      prev->brothers = NULL;    //lint !e771

      return tree_merge (tree_merge_sort (a), tree_merge_sort (b));
   }
   return c;
}

//*********************************************************
//  recursive routine that sorts the sons of each brother
//*********************************************************
static struct dirs *tree_sort_walk (struct dirs *t)
{
   struct dirs *dptr = t;
   while (dptr != 0) {
      dptr->sons = tree_sort_walk (dptr->sons);
      dptr = dptr->brothers;
   }
   return tree_merge_sort (t);
}

//*********************************************************
//  This intermediate function is used because I want
//  merge_sort() to accept a passed parameter,
//  but in this particular application the initial
//  list is global.  This function sets up the global
//  comparison-function pointer and passes the global
//  list pointer to merge_sort().
//*********************************************************
static void sort_trees (void)
{
   // tree_sort_fcn = current_sort ;

   if (n.reverse) {
      if (n.sort == SORT_SIZE)
         tree_sort_fcn = tree_sort_size_rev;
      else
         tree_sort_fcn = tree_sort_name_rev;
   }

   //  normal sort
   else {
      if (n.sort == SORT_SIZE)
         tree_sort_fcn = tree_sort_size;
      else
         tree_sort_fcn = tree_sort_name;
   }

   //  now, sort the data
   top = tree_sort_walk (top);
}

//**********************************************************
static int build_dir_tree (TCHAR *tpath)
{
   int result ;
   TCHAR *strptr;
   level = 0;

   //  Extract base path from first filespec,
   //  and strip off filename
   _tcscpy (base_path, tpath);
   strptr = _tcsrchr (base_path, _T('\\'));
   if (strptr != 0)
      *(++strptr) = 0;          //  strip off filename

   get_disk_info (base_path);

   //  allocate struct for dir listing
   top = new_dir_node ();

   //  Extract base path from first filespec,
   //  and strip off filename
   _tcscpy (base_path, tpath);
   strptr = _tcsrchr (base_path, _T('\\'));
   strptr++;                    //  skip past backslash, to filename
   *strptr = 0;                 //  strip off filename
   base_len = _tcslen (base_path);

   //  derive root path name
   if (_tcslen (base_path) == 3) {
      top->name = (TCHAR *) malloc(8 * sizeof(TCHAR)) ;
      if (top->name == 0)
         error_exit (OUT_OF_MEMORY, NULL);
      _tcscpy (top->name, _T("<root>"));
   }
   else {
      _tcscpy (tempstr, base_path);
      tempstr[base_len - 1] = 0; //  strip off tailing backslash
      strptr = _tcsrchr (tempstr, _T('\\'));
      strptr++;                 //  skip past backslash, to filename

      top->name = (TCHAR *) malloc((_tcslen (strptr) + 1) * sizeof(TCHAR));
      if (top->name == 0)
         error_exit (OUT_OF_MEMORY, NULL);
      _tcscpy (top->name, strptr);
   }

   // top->attrib = 0 ;   //  top-level dir is always displayed

   if (n.ucase)
      _tcsupr (top->name);

   _tcscpy (dirpath, tpath);

   pattern_init(_T("wait; reading directory ")) ;
   result = read_dir_tree (top);
#ifdef  DESPERATE
debug_dump("exit", "returned from read_dir_tree") ;
#endif
   pattern_reset() ;
   return result ;
}

//*****************************************************************
//  no filename may be specified here...
//*****************************************************************
void tree_listing (unsigned total_filespec_count)
{
   if (z == 0)
      tree_init_sort ();

   for (unsigned l = 0; l < total_filespec_count; l++) {
      //  read and build the dir tree
      build_dir_tree (target[l]) ;

      //  sort the tree list
      sort_trees ();

      //  now display the resulting directory tree
      draw_dir_tree ();
   }
}

