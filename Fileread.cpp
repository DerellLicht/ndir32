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

#include "common.h"
#include "ndir32.h"

//  from nsort.cpp
extern void sort_filelist (void);

//*****************  function prototypes  *****************
static void process_exclusions (void);
static void read_long_files (int i);

//*********************************************************
//  This loops thru all files in one subdirectory,
//  calling update_filelist() to add files to the
//  global linked list.
//  This differs from read_files() in that it uses
//  the MSDOS 7.00 long-filename functions
//*********************************************************
char lfn_src[PATH_MAX + 1];
char lfn_dest[PATH_MAX + 1];

static void read_long_files (int i)
{
   int done, fn_okay, result;
   HANDLE handle;
   char *strptr;
   ffdata *ftemp;
   char *p = 0;

   if (n.lfn_off) {
      //  we need the whole path before we can get short filename
      strcpy (lfn_src, target[i]);
      p = strrchr (lfn_src, '\\');  //  strip off wildcards or target name
      if (p != 0)
         *(++p) = 0;
   }

   syslog("%s\n", target[i]);
   handle = FindFirstFile (target[i], &fdata);
   //  according to MSDN, Jan 1999, the following is equivalent
   //  to the preceding... unfortunately, under Win98SE, it's not...
   // handle = FindFirstFileEx(target[i], FindExInfoStandard, &fdata, 
   //                      FindExSearchNameMatch, NULL, 0) ;
   if (handle == INVALID_HANDLE_VALUE)
      return;

// FILE *dbg ;
// dbg = fopen("\\debug", "wt") ;
// fprintf(dbg, "lfn_off=%d\n", n.lfn_off) ;

   //  loop on find_next
   done = 0;
   while (!done) {
      if (n.show_all == 0) {
         if ((fdata.dwFileAttributes & 
            (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)) != 0) {
            fn_okay = 0 ;
            goto search_next_file;
         }
      }
      //  filter out directories if not requested
      if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_VOLID) != 0)
         fn_okay = 0;
      else if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
         FILE_ATTRIBUTE_DIRECTORY)
         fn_okay = 1;
      else if (n.tree == 2)     //  "files only" flag
         fn_okay = 0;
      //  For directories, filter out "." and ".."
      else if (fdata.cFileName[0] != '.') //  fn=".something"
         fn_okay = 1;
      else if (fdata.cFileName[1] == 0)   //  fn="."
         fn_okay = 0;
      else if (fdata.cFileName[1] != '.') //  fn="..something"
         fn_okay = 1;
      else if (fdata.cFileName[2] == 0)   //  fn=".."
         fn_okay = 0;
      else
         fn_okay = 1;

      if (fn_okay) {
         // printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.cFileName) ;
         // printf("%9ld %04X %s\n", fdata.file_size, fdata.attrib, fdata.cFileName) ;
         filecount++;

         //****************************************************
         //  allocate and initialize the structure
         //****************************************************
         // ftemp = new ffdata;
         ftemp = (struct ffdata *) malloc(sizeof(ffdata)) ;
         if (ftemp == NULL) {
            error_exit (OUT_OF_MEMORY, NULL);
            return;             //  only to make lint happy
         }

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

         //  If Steven Bensky's short filenames are requested,
         //  generate fully-qualified filenames so I can
         //  request the short name... <sigh> 
         if (n.lfn_off) {
            ftemp->filename = (char *) malloc(15) ;

            if (p == 0) {
               strcpy (ftemp->filename, "No_path");
            }
            else {
               strcpy (p, (char *) fdata.cFileName);
               // strcpy(ftemp->filename, (char *) fdata.cAlternateFileName) ;
               result = GetShortPathName (lfn_src, lfn_dest, PATH_MAX);
               if (result == 0) {
                  sprintf (ftemp->filename, "error=%u",
                     (unsigned) GetLastError ());
               }
               else {
                  strptr = strrchr (lfn_dest, '\\');
                  if (strptr == 0) {
                     strcpy (ftemp->filename, "No_strrchr");
                  }
                  else {
                     strptr++;
                     strcpy (ftemp->filename, strptr);
                  }
               }
            }
         }
         else {
            ftemp->filename = (char *) malloc(strlen ((char *) fdata.cFileName) + 1);
            strcpy (ftemp->filename, (char *) fdata.cFileName);
         }

         //  find and extract the file extension, if valid
         // ftemp->name[0] = 0 ; //  don't use name at all
         ftemp->name = (char *) malloc(strlen (ftemp->filename) + 1) ;
         if (ftemp->name == NULL)
            error_exit (OUT_OF_MEMORY, NULL);

         strcpy (ftemp->name, ftemp->filename);
         strptr = strrchr (ftemp->name, '.');
         if (strptr != 0 && strlen (strptr) <= MAX_EXT_SIZE) {
            strcpy (ftemp->ext, strptr);
            *strptr = 0;        //  NULL-term name field
         }
         else
            ftemp->ext[0] = 0;  //  no extension found

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
      if (FindNextFile (handle, &fdata) == 0)
         done = 1;
   }

   FindClose (handle);
}

//*************************************************************
#define  MAX_EXCL_COUNT    20
static char excl[MAX_EXCL_COUNT][MAX_EXT_SIZE+1] ; //  allocate dynamically??
static int  exclcount = 0 ;     //  number of exclusion filespecs

//*************************************************************
void update_exclusion_list(char *extptr)
{
   if (exclcount < MAX_EXCL_COUNT) {
      strncpy (excl[exclcount], extptr, MAX_EXT_SIZE);
      excl[exclcount][MAX_EXT_SIZE] = 0 ;
      exclcount++;
   }
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

