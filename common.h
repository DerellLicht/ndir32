//************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                  
//  common.h - common functions, used in various applications
//                                                            
//  Written by:  Daniel D. Miller                             
//************************************************************

#include <tchar.h>

//lint -esym(756, ul2uc_t, u32, u8)

typedef  unsigned long long   u64 ;
typedef  unsigned char        uchar ;
typedef  unsigned long        ulong ;
typedef  unsigned int         uint ;
typedef  unsigned char        u8  ;
typedef  unsigned short       u16 ;
typedef  unsigned int         u32 ;

#define MAXLINE   1024

#define  ESC      27

#define  LOOP_FOREVER   true

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

//  function prototypes
int syslog(const char *fmt, ...);
char *get_system_message(void);
char *get_system_message(DWORD errcode);
int  strcmpiwc(const TCHAR *onestr, const TCHAR *twostr);

// ULLONG_MAX = 18,446,744,073,709,551,615
#define  MAX_ULL_COMMA_LEN  26
char *convert_to_commas(ULONGLONG uli, char *outstr);

// void save_sfn_base_path(char *sfn_base_path);
// char *sfn_convert_filename(char *lfn_filename);

