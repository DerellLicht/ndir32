//*****************************************************************
//  Copyright (c) 1995-2018  Daniel D Miller
//  TREELIST.CPP: Read and display directory tree                  
//*****************************************************************

//  remove this define to remove all the debug messages
// #define  DESPERATE
// #undef  __STRICT_ANSI__

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <windows.h>
#ifdef _lint
#include <malloc.h>
#endif
//#include <tchar.h>
#include "ndir32.h"
#include "conio32.h"

static char const * const dhdr =
   "ÃÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄ";

static char const * const dhdrl =
   "+-----------+-----------+--------------+--------------";

//0         1         2
//012345678901234567890123456789
//=========================+===========+===========+==============+==============
static char formstr[50];
static char levelstr[PATH_MAX];
static uint wincols      = 80 ;
static uint name_end_col = 25 ;
static uint center_col   = 37 ;
static uint left_div     = 49 ;
static uint right_div    = 64 ;

//  instantiate ultosc class as required
//lint -e747 
// static i64tostr dsize (0);      //  dir size in bytes
// static char dsize[MAX_ULL_COMMA_LEN+1]; 
//lint +e747 

//*****************************************************************
//  from FILELIST.CPP
extern void put_disk_summary (void);

//************************************************************
static bool FAT32_present = false;

static char dirpath[PATH_MAX];
static unsigned level;

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
	dirs *brothers;
	dirs *sons;
	char *name;
	uchar attrib;
   ULONGLONG dirsize;
   ULONGLONG dirsecsize;
   ULONGLONG subdirsize;
   ULONGLONG subdirsecsize;
	unsigned files;
	unsigned directs;
	unsigned subfiles;
	unsigned subdirects;
};

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

static dirs *top = NULL;

//***************  function prototypes  ***************
static dirs *new_dir_node (void);

//*****************************************************************
//  this was used for debugging directory-tree read and build
//*****************************************************************
#ifdef  DESPERATE
void debug_dump(char *fname, char *msg)
{
   FILE *fd ;
   fd = fopen("debug.txt", "at") ;
   if (fd == 0) {
      return ;
   }
   fseek(fd, 0, SEEK_END) ;
   fprintf(fd, "l%u %s: %s\n", level, fname, msg) ;
   fclose(fd) ;
}
#endif

//*********************************************************
//  "waiting" pattern generator
//*********************************************************
static unsigned lrow, lcol, dircount ;

static void pattern_init(char *lstr)
{
   dircount = 0 ;
   lrow = _where_y() ;
   if (lstr == 0) {
      lcol = 0 ;
   } else {
      dprints(lrow, 0, lstr) ;
      lcol = strlen(lstr) ;
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
   static char pucount[20] ;

   dircount++ ;
   sprintf(pucount, "%u", dircount) ;
   dprints(lrow, lcol, pucount) ;
}

//**********************************************************
//  recursive routine to read directory tree
//**********************************************************
extern char lfn_src[PATH_MAX + 1];
extern char lfn_dest[PATH_MAX + 1];
static int early_abort = 0 ;

static int read_dir_tree (dirs * cur_node)
{
	dirs *dtail = 0;
	char *strptr;
	HANDLE handle;
	int slen, done, result;
	DWORD err;
   ULONGLONG file_clusters, clusters;

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
		strptr = strrchr (dirpath, '\\');
		strptr++;
		*strptr = 0;
		slen = strlen (dirpath);
		strcat (dirpath, cur_node->name);
		strcat (dirpath, "\\*.*");
	}
	else
		slen = strlen (dirpath);

	//  first, build tree list for current level
	level++;

	char *p = 0;
	if (n.lfn_off) {
		//  we need the whole path before we can get short filename
		strcpy (lfn_src, dirpath);
		p = strrchr (lfn_src, '\\');	//  strip off wildcards or target name
		if (p != 0)
			*(++p) = 0;
	}

#ifdef  DESPERATE
debug_dump(dirpath, "entry") ;
#endif
   err = 0;
	handle = FindFirstFile (dirpath, &fdata);
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
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
debug_dump(dirpath, tempstr) ;
#endif
         // sprintf (tempstr, "path [%s]\n", dirpath);
         // nputs (0xA, tempstr);
         // sprintf (tempstr, "FindFirst: %s\n", get_system_message ());
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
				unsigned cut_dot_dirs;

				// printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.fname) ;
				//  skip '.' and '..', but NOT .ncftp (for example)
				if (fdata.cFileName[0] != '.')
					cut_dot_dirs = 0;
				else if (fdata.cFileName[1] == 0)
					cut_dot_dirs = 1;
				else if (fdata.cFileName[1] == '.' && fdata.cFileName[2] == 0)
					cut_dot_dirs = 1;
				else
					cut_dot_dirs = 0;

				if (cut_dot_dirs == 0) {
					cur_node->directs++;
					cur_node->subdirects++;

					dirs *dtemp = new_dir_node ();
					if (cur_node->sons == NULL)
						cur_node->sons = dtemp;
					else
						dtail->brothers = dtemp;	//lint !e613
					dtail = dtemp;
					// if (!n.ucase) 
					//    strlwr(ff.name) ;
					if (n.lfn_off) {
                  dtail->name = (char *) malloc(14) ;
						if (dtail->name == 0)
							error_exit (OUT_OF_MEMORY, NULL);
						// GetShortPathName((char *) fdata.cFileName, dtail->name, 14) ;

						if (p == 0) {
							strcpy (dtail->name, "No_path");
						}
						else {
							strcpy (p, (char *) fdata.cFileName);
							result = GetShortPathName (lfn_src, lfn_dest, PATH_MAX);
							if (result == 0) {
								sprintf (dtail->name, "error=%u",
									(unsigned) GetLastError ());
							}
							else {
								strptr = strrchr (lfn_dest, '\\');
								if (strptr == 0) {
									strcpy (dtail->name, "No_strrchr");
								}
								else {
									strptr++;
									strcpy (dtail->name, strptr);
								}
							}
						}
					}
					else {
                  dtail->name = (char *) malloc(strlen ((char *) fdata.cFileName) + 1);
						if (dtail->name == 0)
							error_exit (OUT_OF_MEMORY, NULL);
						strcpy (dtail->name, (char far *) fdata.cFileName);
					}
					dtail->attrib = (uchar) fdata.dwFileAttributes;
					// dtail->directs++ ;
				}						  //  if this is not a DOT directory
			}							  //  if this is a directory

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
			}							  //  if entry is a file
		}								  //  if no errors detected

      //******************************************
		//  search for another file
      //******************************************
		if (FindNextFile (handle, &fdata) == 0) {
         // done = 1;
         err = GetLastError ();
         if (err == ERROR_ACCESS_DENIED) {
#ifdef  DESPERATE
debug_dump(fdata.cFileName, "denied") ;
#else
            ;                     //  continue reading
#endif
         }
         else if (err == ERROR_NO_MORE_FILES) {
            done = 1 ;
         }
         else {
#ifdef  DESPERATE
sprintf (tempstr, "FindNext: %s\n", get_system_message (err));
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
if (ktemp == 0) 
   debug_dump("[NULL]", "call read_dir_tree") ;
else
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
	strcpy (&dirpath[slen], "*.*");	//lint !e669  string overrun??

	//  restore the level number
	level--;
	return 0;
}

//**********************************************************
//  allocate struct for dir listing                         
//  NOTE:  It is assumed that the caller will               
//         initialize the name[], ext[], attrib fields!!    
//**********************************************************
static dirs *new_dir_node (void)
{
	//lint -esym(613,dtemp) 
   dirs *dtemp = (dirs *) malloc(sizeof(dirs)) ;
	if (dtemp == NULL)
		error_exit (OUT_OF_MEMORY, NULL);
	memset ((char *) dtemp, 0, sizeof (struct dirs));	//lint !e668
	// if (n.tree == 1) {
	dtemp->dirsecsize = clbytes;
	dtemp->subdirsecsize = clbytes;
	// dtemp->directs = 1 ;
	// }
	// else {
	//   dtemp->dirsecsize = 1;
	//   dtemp->subdirsecsize = 1;
	// }
	return dtemp;
}

//**********************************************************
static void display_tree_filename (char *lstr, char *frmstr)
{
   uint slen = strlen (lstr);

	//  if directory name, etc., is too long, make separate line
   if (slen > center_col) {
      sprintf (tempstr, "%-*s", right_div, lstr); //  write filename
		nputs (dtree_colors[level], tempstr);

		//  insert blank line
		nputc (n.colorframe, vline);
		ncrlf ();

      sprintf (tempstr, "%-*s", name_end_col+1, frmstr);
		nputs (dtree_colors[level], tempstr);	//  spaces
	}
   else if (slen > name_end_col) {
      sprintf (tempstr, "%-*s", center_col, lstr); //  write filename
		nputs (dtree_colors[level], tempstr);

		//  insert blank line
		nputc (n.colorframe, vline);
      sprintf (tempstr, "%*s", name_end_col+1, "");
		nputs (dtree_colors[level], tempstr);	//  spaces

		nputc (n.colorframe, vline);
		ncrlf ();
      sprintf (tempstr, "%-*s", name_end_col+1, frmstr);
		nputs (dtree_colors[level], tempstr);	//  spaces
	}
	else {
      sprintf (tempstr, "%-*s", name_end_col+1, lstr);
		nputs (dtree_colors[level], tempstr);
	}
}

//**********************************************************
static void display_size(ULONGLONG dlen, unsigned slen, unsigned attr)
{
   if (n.size_display == 2) {
      dlen /= 1000000 ;
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, 'M') ;
   } else 
   if (dlen > (ULONGLONG) 999999999  ||  n.size_display == 1) {
      dlen /= 1000 ;
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen-1, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
      nputc(attr ^ 0x08, 'K') ;
   } else 
   {
      // dsize.convert (dlen);
      sprintf (tempstr, "%*s", slen, convert_to_commas(dlen, NULL));
      nputs (attr, tempstr);
   }
}

//**********************************************************
//  recursive routine to display directory tree
//  do all subroutines, then go to next.
//**********************************************************
static void display_dir_tree (dirs * ktop)
{
	dirs *ktemp = ktop;
	if (ktop == NULL)
		return;

	//  next, build tree lists for subsequent levels (recursive)
	while (ktemp != NULL) {
		//  first, build tree list for current level
		if (level == 0) {
			formstr[0] = (char) NULL;
		}
		else {
			if (ktemp->brothers == (struct dirs *) NULL) {
            formstr[level - 1] = (char) (n.low_ascii) ? '\\' : 'À';   //lint !e743 
				formstr[level] = (char) NULL;
			}
			else {
            formstr[level - 1] = (char) (n.low_ascii) ? '+' : 'Ã';   //lint !e743 
				formstr[level] = (char) NULL;
			}
		}
		sprintf (levelstr, "%s%s", formstr, ktemp->name);
      // syslog("l%u %s\n", level, levelstr) ;

		//*****************************************************************
		//                display data for this level                      
		//*****************************************************************
		display_tree_filename (levelstr, formstr);
		switch (n.tree) {
         //  show file/directory sizes only
			case 1:
            if (ktemp->dirsize != ktemp->subdirsize ||   //lint !e777
               ktemp->dirsecsize != ktemp->subdirsecsize) {   //lint !e777
               // dsize.convert  (ktemp->dirsize);
               // dssize.convert (ktemp->dirsecsize);
               // sdsize.convert (ktemp->subdirsize);

					//  now, print the normal directory
               // sprintf (tempstr, "%11s", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);

					// sprintf(tempstr, "%11s %14s", dssize_ptr, sdsize_ptr) ;
               // sprintf (tempstr, "%11s %14s", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->dirsize, 11, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
               display_size(ktemp->dirsecsize, 11, dtree_colors[level]) ;
               nputc (dtree_colors[level], ' ');
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }

				/*  no subdirectories are under this one  */
				else {
					//  now, print the normal directory
					nputs (dtree_colors[level], "           ");
					nputc (n.colorframe, vline);
               nputs (dtree_colors[level], "            ");
					// sprintf(tempstr, "            %14s", sdsize_ptr) ;
               // sdsize.convert (ktemp->subdirsize);
               // sprintf (tempstr, "%14s", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size(ktemp->subdirsize, 14, dtree_colors[level]) ;
               nputc (n.colorframe, vline);
            }                   /* end  else !(ktemp->nsdi) */

            // sprintf(tempstr, "%14s", sdssize_ptr) ;
            // sdssize.convert (ktemp->subdirsecsize);
            // sprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
				break;

         //  show file/directory counts only
			case 4:
				if ((ktemp->files == ktemp->subfiles) &&
					(ktemp->directs == ktemp->subdirects)) {
					//  now, print the normal directory
					nputs (dtree_colors[level], "           ");
					nputc (n.colorframe, vline);

               // sdsize.convert ((unsigned long long) ktemp->files);
               // sprintf (tempstr, "            %12s  ", sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               nputs (dtree_colors[level], "            ");
               display_size((ULONGLONG) ktemp->files, 12, dtree_colors[level]) ;

               nputs (dtree_colors[level], "  ");
					nputc (n.colorframe, vline);
					// sprintf(tempstr, "%14s", sdssize_ptr) ;
               // sdssize.convert ((unsigned long long) ktemp->directs);
               // sprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 10, dtree_colors[level]) ;
				}

				/*  no subdirectories are under this one  */
				else {
					//  now, print the normal directory
               // dsize.convert ((unsigned long long) ktemp->files);
               // sprintf (tempstr, "%9s  ", dsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->files, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], "  ");
					nputc (n.colorframe, vline);

               // dssize.convert ((unsigned long long) ktemp->directs);
               // sdsize.convert ((unsigned long long) ktemp->subfiles);
               // sprintf (tempstr, "%9s   %12s  ", dssize.putstr (),
               //   sdsize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->directs, 9, dtree_colors[level]) ;
               nputs (dtree_colors[level], "   ");
               display_size((ULONGLONG) ktemp->subfiles, 12, dtree_colors[level]) ;
               nputs (dtree_colors[level], "  ");
					nputc (n.colorframe, vline);

               // sdssize.convert ((unsigned long long) ktemp->subdirects);
               // sprintf (tempstr, "%10s", sdssize.putstr ());
               // nputs (dtree_colors[level], tempstr);
               display_size((ULONGLONG) ktemp->subdirects, 10, dtree_colors[level]) ;
				}						  /* end  else !(ktemp->nsdi) */
				break;

         //  show mixed size, file counts, directory counts
			case 5:
            //  now, print the normal directory
            // dsize.convert ((unsigned long long) ktemp->subfiles);
            // sprintf (tempstr, "%9s  ", dsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subfiles, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], "  ");
                  nputc (n.colorframe, vline);

            // dssize.convert ((unsigned long long) ktemp->subdirects);
            // sdsize.convert ((unsigned long long) ktemp->dirsecsize);
            // sprintf (tempstr, "%9s   %13s ", dssize.putstr (),
            //   sdsize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size((ULONGLONG) ktemp->subdirects, 9, dtree_colors[level]) ;
            nputs (dtree_colors[level], "   ");
            display_size(ktemp->dirsecsize, 13, dtree_colors[level]) ;
            nputs (dtree_colors[level], " ");
                  nputc (n.colorframe, vline);

            // sdssize.convert ((unsigned long long) ktemp->subdirsecsize);
            // sprintf (tempstr, "%14s", sdssize.putstr ());
            // nputs (dtree_colors[level], tempstr);
            display_size(ktemp->subdirsecsize, 14, dtree_colors[level]) ;
				break;

			default:
				break;				  // make lint happy
		}
		ncrlf ();

		//  build tree string for deeper levels
		if (level > 0) {
			if (ktemp->brothers == NULL)
				formstr[level - 1] = ' ';
			else
            formstr[level - 1] = (n.low_ascii) ? '|' : '³'; //lint !e743 
		}								  //  if level > 1

		//  process any sons
		level++;
      if (!n.tree_short || level <= tree_level_limit) {
			display_dir_tree (ktemp->sons);
		}
		formstr[--level] = (char) NULL;

		//  goto next brother
		ktemp = ktemp->brothers;
	}									  //  while not done listing directories
}

/*****************************************************************/
static void printdirheader (void)
{
   // int slen, freelen, vnlen, blen;

   if (!n.color) 
      display_logo();
   nputs (n.colornhead, "Directory of "); //  len = 13
	sprintf (tempstr, "%s", base_path);
	nputs (n.colorxhead, tempstr);

   uint vnlen = 16 + strlen(volume_name) ; //  16 is length of 'Volume label is '
   uint blen  = 13 + strlen(base_path) ;   //  13 is length of 'Directory of '
   if ((blen + vnlen) >= wincols) 
      ncrlf ();
   else
      nput_char (n.colornhead, ' ', (wincols - blen - vnlen - 1));
   nputs (n.colornhead, "Volume label is "); //  len = 16
	nputs (n.colorxhead, volume_name);
	ncrlf ();

	//**************************************
	//  Heading line 1
	//**************************************
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[name_end_col] = tempstr[left_div] = tline;
	nputs (n.colorframe, tempstr);
	ncrlf ();

	switch (n.tree) {
		case 1:
			//**************************************
			//  Heading line 2
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "   size of requested   ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "    total size,  including");
			ncrlf ();

			//**************************************
			//  Heading line 3
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			// nputs (n.colornhead, "Subdirectory names       ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "       directory       ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "     lower subdirectories");
			ncrlf ();
			break;

		case 4:
			//**************************************
			//  Heading line 2
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " files and directories ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "   files and directories ");
			ncrlf ();

			//**************************************
			//  Heading line 3
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " in current directory  ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " in current and lower dirs");
			ncrlf ();
			break;

		case 5:
			//**************************************
			//  Heading line 2
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " files and directories ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "    files and directories");
			ncrlf ();

			//**************************************
			//  Heading line 3
			//**************************************
         nput_char (n.colornhead, ' ', name_end_col);
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "   cumulative counts   ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "       cumulative sizes   ");
			ncrlf ();
			break;

		default:
			break;					  // make lint happy
	}

	//**************************************
	//  Heading line 4
	//**************************************
   nput_char (n.colornhead, ' ', name_end_col);
   nputs (n.colorframe, (n.low_ascii) ? dhdrl : dhdr);
	ncrlf ();

	//**************************************
	//  Heading line 5
	//**************************************
   // nputs (n.colornhead, "Subdirectory names       ");
   sprintf(tempstr,"%-*s", name_end_col, "Subdirectory names");
   nputs (n.colornhead, tempstr);
	nputc (n.colorframe, vline);
	switch (n.tree) {
		case 1:
			nputs (n.colornhead, "  in bytes ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " disk space");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "   in bytes   ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "  disk space");
			ncrlf ();
			break;

		case 4:
			nputs (n.colornhead, "    files  ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "directories");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "       files  ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, " directories");
			ncrlf ();
			break;

		case 5:
			nputs (n.colornhead, "    files  ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "directories");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "  disk space  ");
			nputc (n.colorframe, vline);
			nputs (n.colornhead, "  disk space");
			ncrlf ();
			break;

		default:
			break;					  // make lint happy
	}

	//**************************************
	//  Heading line 6
	//**************************************
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[name_end_col] = tempstr[left_div] = bline;
   tempstr[center_col] = tempstr[right_div] = xline;
	nputs (n.colorframe, tempstr);
	ncrlf ();
}

//*********************************************************
static void print_dir_end (void)
{
	//  draw divider line for bottom of data
   memset (&tempstr[0], dline, wincols-1);
   tempstr[wincols-1] = 0;
   tempstr[center_col] = tempstr[right_div] = bline;
	nputs (n.colorframe, tempstr);
	ncrlf ();

	//  now show disk totals
   //  NOTE: FAT32_present is *always* false now,
   //  so the resulting function will never be called
	if (FAT32_present) {
		put_disk_summary ();
	}
	else {
		// i64tostr dtbytes (diskbytes);
		// i64tostr dfbytes (diskfree);

		nputs (n.colornhead, "Disk capacity: ");
		nputs (n.colorxhead, convert_to_commas(diskbytes, NULL));
		nputs (n.colornhead, " bytes, Free disk space: ");
		nputs (n.colorxhead, convert_to_commas(diskfree, NULL));
		nputs (n.colornhead, " bytes");
		ncrlf ();
	}
}

//****************************************************
//  allocate a dummy structure for merge_sort()
//****************************************************
static int tree_init_sort (void)
{
   // z = new dirs;
   z = (dirs *) malloc(sizeof(dirs)) ;
	if (z == NULL)
		error_exit (OUT_OF_MEMORY, NULL);
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
	return (stricmp (a->name, b->name));
}

//*********************************************************
static int tree_sort_name_rev (struct dirs *a, struct dirs *b)
{
	return (stricmp (b->name, a->name));
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
		prev->brothers = NULL;	  //lint !e771

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
static int build_dir_tree (char *tpath)
{
   int result ;
   char *strptr;
   level = 0;

   //  Extract base path from first filespec,
   //  and strip off filename
   strcpy (base_path, tpath);
   strptr = strrchr (base_path, '\\');
   if (strptr != 0)
      *(++strptr) = 0;          //  strip off filename

   FAT32_present = get_disk_info (base_path);

   //  allocate struct for dir listing
   top = new_dir_node ();

   //  Extract base path from first filespec,
   //  and strip off filename
   strcpy (base_path, tpath);
   strptr = strrchr (base_path, '\\');
   strptr++;                    //  skip past backslash, to filename
   *strptr = 0;                 //  strip off filename
   base_len = strlen (base_path);

   //  derive root path name
   if (strlen (base_path) == 3) {
      top->name = (char *) malloc(8) ;
      if (top->name == 0)
         error_exit (OUT_OF_MEMORY, NULL);
      strcpy (top->name, "<root>");
   }
   else {
      strcpy (tempstr, base_path);
      tempstr[base_len - 1] = 0; //  strip off tailing backslash
      strptr = strrchr (tempstr, '\\');
      strptr++;                 //  skip past backslash, to filename

      top->name = (char *) malloc(strlen (strptr) + 1);
      if (top->name == 0)
         error_exit (OUT_OF_MEMORY, NULL);
      strcpy (top->name, strptr);
   }

   // top->attrib = 0 ;   //  top-level dir is always displayed

   if (n.ucase)
      strupr (top->name);

   strcpy (dirpath, tpath);

   pattern_init("wait; reading directory ") ;
   result = read_dir_tree (top);
#ifdef  DESPERATE
debug_dump("exit", "returned from read_dir_tree") ;
#endif
   pattern_reset() ;
   return result ;
}

//*********************************************************
static void draw_dir_tree (void)
{
   level = 0;
   wincols = get_window_cols() ;
   //  this is necessary because if redirection is in place,
   //  'console width' may not be valid`
   if (is_redirected()) {
      wincols = 80 ;
   }

   if (wincols != 80) {
      name_end_col = wincols - (80-25) ;
      center_col   = wincols - (80-37) ;
      left_div     = wincols - (80-49) ;
      right_div    = wincols - (80-64) ;
      // 135: 80, 92, 104, 119
      // syslog("%u: %u, %u, %u, %u\n", wincols,
      //    name_end_col, center_col, left_div, right_div);
   }

   // syslog("level before displaying: %u\n", level);
   printdirheader ();
   display_dir_tree (top);
   print_dir_end ();
}

//*****************************************************************
//  no filename may be specified here...
//*****************************************************************
void tree_listing (void)
{
   if (z == 0)
      tree_init_sort ();

   for (unsigned l = 0; l < tcount; l++) {
      //  check for validity of long_filename functions
      dname[0] = *target[l];

      lfn_supported = 1 - n.lfn_off;

      //  read and build the dir tree
      build_dir_tree (target[l]) ;

      //  sort the tree list
      sort_trees ();

      //  now display the resulting directory tree
      draw_dir_tree ();
   }
}

