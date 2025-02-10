//************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                  
//  ndir32.h - Header file for NDIR directory lister.         
//                                                            
//  Written by:  Daniel D. Miller                             
//************************************************************

#include <limits.h>
// #include <tchar.h>

//*********************************************************

#define  MAX_EXT_SIZE   8

#define  MAX_DIR_ENTRY     20
extern uchar dtree_colors[MAX_DIR_ENTRY] ;

//  n.drive_summary constants
#define DSUMMARY_NONE   0
#define DSUMMARY_FREE   1
#define DSUMMARY_USED   2

//*****************************************************************
//                  CONFIGURATION VARIABLES (new style)
//
//  NOTE: These must stay in this order, in order for
//        NDIRMOD and NDINSTAL to work properly!!
//
//*****************************************************************
struct ndir_data {
   uchar colorlogo   ;
   uchar colornhead  ;
   uchar colorxhead  ;
   uchar colorframe  ;
   uchar colorattr   ;
   uchar colorsize   ;
   uchar colordate   ;
   uchar colortime   ;
   uchar colordir    ;
   uchar colorSHR ;
   uchar colordefalt ;

   uchar ucase ;
   uchar sort ;        //  0=ext, 1=name, 2=size, 3=date/time, 4=none,
   uchar reverse ;     //  reverse normal sort order
   uchar clear ;
   uchar pause ;
   uchar format ;      //  0=1col, 1=2col, 2=4col, 3=6col, 4=dirtree, 5=3col
   uchar minimize ;
   uchar tree ;        //  0=all, 1=dir tree, 2=files only, 3=unused
   uchar show_all ;
   uchar dir_first ;
   uchar exec_only ;
   uchar help ;
   uchar info ;
   uchar horz ;
   uchar low_ascii ;
   uchar batch ;
   uchar unused1 ;
   uchar color ;
   uchar showSHRfiles ;
   uchar ega_keep_unused ;     // deprecated on 01.17.23
   uchar drive_summary ;//  0 = no, 1 = show free space, 2 = show used space
   uchar fdate_option ; //  0 = LAST_MODIFY, 1 = FILE_CREATE (MSDOS 7.00+ only)
   // uchar lfn_off ;      //  0 = force DOS 8.3 filenames (removed in V2.62)
   uchar tree_short ;   //  0=normal tree, 1=show limited level of children
   uchar long_attr ;
   uchar size_display ; //  0=bytes/normal, 1=KB, 2=MB
};
extern ndir_data n ;

extern unsigned tree_level_limit ;

//  fdate_option flags
#define  FDATE_LAST_WRITE     0
#define  FDATE_LAST_ACCESS    1
#define  FDATE_CREATE_TIME    2

//*****************************************************************
//              END OF CONFIGURATION VARIABLES
//*****************************************************************

//***************************************************************
//  display strings for disk-size totals
//***************************************************************
extern ULONGLONG diskbytes, diskfree, clbytes ;

//************************************************************
#define topchar       0xD1   /*  Ñ  */
#define bottomchar    0xCF   /*  Ï  */
#define vlinechar     0xB3   /*  ³  */
#define xlinechar     0xD8   /*  Ø  */
#define dvlinechar    0xCD   /*  Í  */
#define crosschar     0x2B   /*  +  */
#define altvlinechar  0x7C   /*  |  */
#define altdvlchar    0x3D   /*  =  */

extern uchar tline, bline, vline, xline, dline ;

/************************************************************/
struct ffdata {
   u16            attrib ; //  expand this to allow for extended file attributes
   // unsigned       ftime ;
   // unsigned       fdate ;
   FILETIME       ft ;
   ULONGLONG      fsize ;
   TCHAR          *filename ;
   TCHAR          *name ;
   TCHAR          ext[MAX_EXT_SIZE+1] ;
   uchar          color ;
   uchar          dirflag ;
   bool           is_link_file ;
   bool           is_multi_byte ;
   uint           mb_len ;
   struct ffdata  *next ;
   } ;
extern ffdata *ftop ;
extern ffdata *ftail ;

//  this definition was excluded by WINNT.H
#define FILE_ATTRIBUTE_VOLID  0x00000008

//**********************************************************
// #ifdef UNICODE
extern TCHAR* target[20] ;
extern TCHAR volume_name[PATH_MAX] ;
// #else
// extern char* target[20] ;
// extern char volume_name[PATH_MAX] ;
// #endif

//*********************************************************
//                NSORT.CPP
//*********************************************************
//lint -esym(769, sort_modes::SORT_EXT_REV , sort_modes::SORT_NAME_REV, sort_modes::SORT_SIZE_REV)
//lint -esym(769, sort_modes::SORT_DATE_REV, sort_modes::SORT_INV_TYPE)

enum sort_modes {
SORT_EXT   = 0,
SORT_NAME     ,
SORT_SIZE     ,
SORT_DATE     ,
SORT_EXT_REV  ,
SORT_NAME_REV ,
SORT_SIZE_REV ,
SORT_DATE_REV ,
SORT_INV_TYPE } ;

/************************************************************/
/*  error codes and messages.                               */
/************************************************************/
enum error_values { DATA_OKAY = 0, OUT_OF_MEMORY, INV_DRIVE } ;

void error_exit(int index, TCHAR* outstr);

//************************************************************
//  ndata.cpp
//************************************************************
//  name of drive+path without filenames
extern TCHAR base_path[PATH_MAX] ;
extern unsigned base_len ;  //  length of base_path

//************************************************************
//  ndata.cpp
//************************************************************

extern TCHAR tempstr[MAXLINE] ;
extern unsigned filecount ;             //  number of files found
extern TCHAR leftstr[37], rightstr[37] ; //  used by batch mode

extern unsigned columns ;           //  number of display columns on screen

extern unsigned start, finish ;
//*****************  function prototypes**********************

//  ndir.cpp
void getcolor (ffdata * fnew);

//  cmd_line.cpp
void verify_flags(void);

//  config.cpp
#define  MAX_EXT        200
struct attrib_list {
   uchar  attr ;
   TCHAR  ext[MAX_EXT_SIZE+1] ;
} ;
extern attrib_list const attr_default_list[] ;
extern attrib_list attr_table[MAX_EXT] ;
extern unsigned attrib_count ;

int write_default_ini_file(TCHAR *ini_str);
void parse_ini_line(TCHAR *iniptr);

//  filelist.cpp
void display_files(void);
void put_disk_summary(void);

//  fileread.cpp
void file_listing(void);

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);

//  ndisplay.cpp
void info(char *data[]);
void display_logo(void);
void print1(ffdata *fptr);
void nclrscr(void);
void ncrlf(void);
void nputc(uchar attr, const uchar outchr);
void nputs(uchar attr, const char* outstr);
void nput_char(uchar attr, char chr, int count);
void nput_line(uchar attr, char chr);
void nputsw(uchar attr, const char *outstr, int wlen, int clen);

//  diskparm.cpp
bool get_disk_info(TCHAR *dstr);
void display_drive_summary(void);

//  read_link.cpp
bool read_shortcut_file(ffdata * fptr, char *shortcut_path);

