//************************************************************
//    NDATA.HPP - Header file for NDIR directory lister.      
//                                                            
//    Written by:  Daniel D. Miller                           
//                                                            
//************************************************************

// #include <windows.h>
// #include <stdlib.h>  //  PATH_MAX
#include <limits.h>

// extern int stricmp(const char *string1, const char *string2);
// extern int strnicmp(const char *string1, const char *string2, size_t count);
// extern char *strupr(char *str);
// extern int _tolower(int c);

// #ifndef PATH_MAX
// #define PATH_MAX  260
// #endif

typedef  unsigned long long   u64 ;
typedef  unsigned char        uchar ;
typedef  unsigned long        ulong ;
typedef  unsigned int         uint ;
typedef  unsigned char        u8  ;
typedef  unsigned short       u16 ;
typedef  unsigned int         u32 ;


#define MAXLINE   260

#define  LOOP_FOREVER   true

//*********************************************************
#define  MAX_EXT        200
#define  MAX_EXT_SIZE   8
struct attrib_list {
   uchar  attr ;
   char  ext[MAX_EXT_SIZE+1] ;
} ;
extern attrib_list attr_table[MAX_EXT] ;

extern unsigned attrib_count ;

#define  MAX_DIR_ENTRY     20
extern uchar dtree_colors[MAX_DIR_ENTRY] ;

//*********************************************************
// union i64tol {
//    __int64 i ;
//    ulong u[2] ;
// };

union u64toul {
   ULONGLONG i ;
   ulong u[2] ;
};

//*********************************************************************
typedef union ul2uc_u {
   unsigned       ul ;
   unsigned short us[2] ;
   unsigned char  uc[4] ;
} ul2uc_t;

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
   uchar ega_keep ;
   uchar drive_summary ;//  0 = no, 1 = show free space, 2 = show used space
   uchar fdate_option ; //  0 = LAST_MODIFY, 1 = FILE_CREATE (MSDOS 7.00+ only)
   uchar lfn_off ;      //  0 = force DOS 8.3 filenames (MSDOS 7.00+ only)
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
//  display strings for FAT32 disk totals
//***************************************************************
extern ULONGLONG diskbytes, diskfree, clbytes ;

//***************************************************************
extern WIN32_FIND_DATA fdata ;

// extern char fsn_bfr[32] ;  //  buffer for name of lfn file system
extern char * const dname ;       //  used by is_lfn_supported() call

extern int lfn_supported ; //  always true for 32-bit version


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

//************************************************************
//  this struct is declared by Borland in IO.H, but is
//  not declared by MSC at all.
//  It is used to extract useful info from DOS file time/date
//************************************************************

struct  ftime   {
    unsigned    ft_tsec  : 5;   /* Two second interval */
    unsigned    ft_min   : 6;   /* Minutes */
    unsigned    ft_hour  : 5;   /* Hours */
    unsigned    ft_day   : 5;   /* Days */
    unsigned    ft_month : 4;   /* Months */
    unsigned    ft_year  : 7;   /* Year */
};

//*********************************************************
//  Convert date and time fields from file struct
//  into separate date and time elements.
//  This union requires io.h for struct ftime.
//*********************************************************
union parse_time {
   unsigned short dtime[2] ;
   ftime outdata ;
   } ;

/************************************************************/
struct ffdata {
   //  uchar          attrib ;
   u16            attrib ; //  expand this to allow for extended file attributes
   // unsigned       ftime ;
   // unsigned       fdate ;
   FILETIME       ft ;
   ULONGLONG      fsize ;
   char           *filename ;
   // char           name[13] ;
   char           *name ;
   char           ext[MAX_EXT_SIZE+1] ;
   uchar          color ;
   uchar          dirflag ;
   struct ffdata  *next ;
   } ;
extern ffdata *ftop ;
extern ffdata *ftail ;

//  this definition was excluded by WINNT.H
#define FILE_ATTRIBUTE_VOLID  0x00000008

//**********************************************************
extern char* target[20] ;
// extern char excl[20][PATH_MAX] ;         //  20 * 260
extern char volume_name[PATH_MAX] ;

//*********************************************************
//                NSORT.CPP
//*********************************************************
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

void error_exit(int index, char* outstr);

//************************************************************
//  ntypes.cpp
//************************************************************
extern char *idtxt[] ;
extern char *helptxt[] ;

//  name of drive+path without filenames
extern char base_path[PATH_MAX] ;
extern unsigned base_len ;  //  length of base_path

//************************************************************
//  ndata.cpp
//************************************************************
// extern struct _find_t fblk;

extern char tempstr[MAXLINE] ;
extern int  filecount ;             //  number of files found
// extern uchar findattr ;
// extern int  exclcount ;             //  number of exclusion filespecs
extern char leftstr[37], rightstr[37] ; //  used by batch mode
extern unsigned tcount ;            //  number of target filespecs

extern unsigned lines ;
extern unsigned curlines ;          //  are both this AND (lines) required??
extern int columns ;                //  number of display columns on screen
extern unsigned linecnt ;           //  non-color display line counter

extern unsigned start, finish ;

extern double total_ptime ;
//*****************  function prototypes**********************

//  system.cpp
int syslog(const char *fmt, ...);
char *get_system_message(void);
char *get_system_message(DWORD errcode);
void dshow_row_info(char *msg);

//  ndir.cpp
int  strcmpiwc(const char *onestr, const char *twostr);

// ULLONG_MAX = 18,446,744,073,709,551,615
#define  MAX_ULL_COMMA_LEN  26
char *convert_to_commas(ULONGLONG uli, char *outstr);

//  cmd_line.cpp
void verify_flags(void);

//  filelist.cpp
void display_files(void);
bool get_disk_info(char *dstr);

//  fileread.cpp
void file_listing(void);

//  treelist.cpp
void tree_listing(void);

//  ndisplay.cpp
void info(char *data[]);
void display_logo(void);
void print1(ffdata *fptr);
void print2(ffdata *fptr);
void print3(ffdata *fptr);
void print4(ffdata *fptr);
void print6(ffdata *fptr);
void ngotoxy(int x, int y);
void ncrlf(void);
void nputc(uchar attr, const uchar outchr);
void nputs(uchar attr, const char* outstr);
void nput_char(uchar attr, char chr, int count);
void nput_line(uchar attr, char chr);

//  diskparm.cpp
void display_drive_summary(void);
