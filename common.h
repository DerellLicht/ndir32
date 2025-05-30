//************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                  
//  common.h - common functions, used in various applications
//                                                            
//  Written by:  Daniel D. Miller                             
//************************************************************

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
bool isUpperAscii(WCHAR *outstr, uint slen);
int hex_dump(u8 *bfr, int bytes);
int syslog(const TCHAR *fmt, ...);
TCHAR *get_system_message(void);
TCHAR *get_system_message(DWORD errcode);
int  strcmpiwc(const TCHAR *onestr, const TCHAR *twostr);

// ULLONG_MAX = 18,446,744,073,709,551,615
#define  MAX_ULL_COMMA_LEN  26
// char *convert_to_commas(ULONGLONG uli, char *outstr);
TCHAR *convert_to_commas(ULONGLONG uli, TCHAR *outstr);
