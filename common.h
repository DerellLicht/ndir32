//************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                  
//  common.h - common functions, used in various applications
//                                                            
//  Written by:  Daniel D. Miller                             
//************************************************************

int syslog(const char *fmt, ...);
char *get_system_message(void);
char *get_system_message(DWORD errcode);
int  strcmpiwc(const char *onestr, const char *twostr);

// ULLONG_MAX = 18,446,744,073,709,551,615
#define  MAX_ULL_COMMA_LEN  26
char *convert_to_commas(ULONGLONG uli, char *outstr);

void save_sfn_base_path(char *sfn_base_path);
char *sfn_convert_filename(char *lfn_filename);

