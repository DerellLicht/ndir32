//************************************************************
//  Copyright (c) 1998-2025 Daniel D. Miller                  
//  ndir32.h - Header file for NDIR directory lister.         
//                                                            
//  Written by:  Daniel D. Miller                             
//************************************************************

//lint -e537   Repeated include file 'd:\tdm32\lib\gcc\mingw32\10.3.0\include\c++\string'
#include <string>
#include <vector>

#define  MAX_EXT_SIZE      8
#define  MAXLINE           1024

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
   uchar colorlogo   {0x13};
   uchar colornhead  {0x02};
   uchar colorxhead  {0x0A};
   uchar colorframe  {0x08};
   uchar colorattr   {0x01};
   uchar colorsize   {0x05};
   uchar colordate   {0x0C};
   uchar colortime   {0x04};
   uchar colordir    {0x09};
   uchar colorSHR    {0x18};
   uchar colordefalt {0x06};

   uchar ucase {};                                                                              
   uchar sort {1};        //  0=ext, 1=name, 2=size, 3=date/time, 4=none
   uchar reverse {};      //  reverse normal sort order
   uchar clear {1};                                                                              
   uchar pause {1};                                                                              
   uchar format {2};      //  0=1col, 1=2col, 2=4col, 3=6col, 4=dirtree, 5=3col
   uchar minimize {};                                                                           
   uchar tree {};         //  0=all, 1=dir tree, 2=files only, 3=unused
   uchar show_all {1};                                                                           
   uchar dir_first {1};                                                                          
   uchar exec_only {};                                                                          
   uchar help {};                                                                               
   uchar info {};                                                                               
   uchar horz {};                                                                               
   uchar low_ascii {};    //  upper ascii no longer supported
   uchar batch {};                                                                              
   uchar unused1 {};                                                                            
   uchar color {1};                                                                              
   uchar showSHRfiles {};                                                                       
   // uchar ega_keep_unused {};     // deprecated on 01.17.23
   uchar drive_summary {DSUMMARY_NONE};//  0 = no, 1 = show free space, 2 = show used space
   uchar fdate_option {};  //  0 = LAST_MODIFY, 1 = FILE_CREATE (MSDOS 7.00+ only)
   // uchar lfn_off ;      //  0 = force DOS 8.3 filenames (removed in V2.62)
   uchar tree_short {};    //  0=normal tree, 1=show limited level of children
   uchar long_attr {};                                                                          
   uchar size_display {};  //  0=bytes/normal, 1=KB, 2=MB
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
extern TCHAR tline, bline, vline, dline ;

// extern TCHAR* target[20] ;
extern std::vector<std::wstring> target ;
extern std::wstring volume_name ;

/************************************************************/
struct ffdata {
   u16            attrib {}; //  expand this to allow for extended file attributes
   FILETIME       ft {};
   ULONGLONG      fsize {};
   std::wstring   filename {};
   std::wstring   name {};
   std::wstring   ext {};
   uchar          color {};
   bool           dirflag {};
   bool           is_link_file {};
   uint           mb_len {};
} ;

extern std::vector<ffdata> flist ;

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
//  ndir32.cpp
//************************************************************
//  name of drive+path without filenames
// extern TCHAR volume_name[MAX_PATH_LEN] ;
extern TCHAR base_path[MAX_PATH_LEN] ;
extern unsigned base_len ;  //  length of base_path
extern TCHAR ininame[MAX_PATH_LEN] ;

extern TCHAR tempstr[MAXLINE] ;
extern unsigned filecount ;             //  number of files found

extern unsigned columns ;           //  number of display columns on screen

extern unsigned start, finish ;
//*****************  function prototypes**********************

//  ndir32.cpp
bool strcmpiwc(const TCHAR *onestr, const TCHAR *twostr);
void dump_target(TCHAR *msg);
void insert_target_filespec(TCHAR *fstr);

//  cmd_line.cpp
void parse_command_string(TCHAR *cmdstr) ;
void parse_command_args(int start, int argc, TCHAR** argv);
void verify_flags(void);

//  config.cpp
void getcolor (ffdata * fnew);
void read_config_file(void);

//  from nsort.cpp
void sort_filelist (void);

//  filelist.cpp
void display_files(void);
void put_disk_summary(void);
void batch_set_left_str(TCHAR *instr);
void batch_set_right_str(TCHAR *instr);

//  fileread.cpp
void clear_existing_file_list(void);
//lint -esym(532, update_exclusion_list)  Return mode of function inconsistent with line 28, file Fileread.cpp
void update_exclusion_list(TCHAR *extptr);
void file_listing(void);

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);

//  ndisplay.cpp
void display_logo(void);
void print1(ffdata *fptr);
void nclrscr(void);
void ncrlf(void);
void nputc(uchar attr, const uchar outchr);
void nputs(uchar attr, const TCHAR* outstr);
void nput_char(uchar attr, TCHAR chr, int count);
void nput_line(uchar attr, TCHAR chr);

//  diskparm.cpp
bool get_disk_info(TCHAR *dstr);
void display_drive_summary(void);

//  read_link.cpp
bool read_shortcut_file(ffdata *fptr, TCHAR *shortcut_path);

