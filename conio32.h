//***************************************************************************
//  Copyright (c) 1995-2023  Daniel D Miller
//  CONIO32.HPP: Template for 32-bit console programs                        
//                                                                           
//  Written by:   Daniel D. Miller                                           
//                                                                           
//  Last Update:  01/17/23
//***************************************************************************

#define  MAX_CHAR_COLS     1024

extern unsigned lines ;

//**********************************************************
void console_init(char *title);
void hide_cursor(void);

void restore_console_attribs(void);
bool is_redirected(void);
CHAR get_char(void);
WORD get_scode(void);
unsigned get_window_cols(void);
int _where_x(void);
int _where_y(void);
void dgotoxy(int x, int y);
void dclrscr(void);
void dclreol(void);
void dclreos(void);
void clear_visible_rows(int u, int l);
void dnewline(void);
void dreturn(void);
void set_text_attr(WORD tFGBG);
void set_text_attr(WORD tFG, WORD tBG);
void dputc(const TCHAR c);
void dprintc(unsigned row, unsigned col, const TCHAR outchr);
void dprintc(unsigned row, unsigned col, unsigned attr, const TCHAR outchr);
void dputnchar(TCHAR chr, TCHAR attr, int count);
void dputs(const TCHAR *outstr);
void dprints(unsigned row, unsigned col, const TCHAR * outstr);
BOOL control_handler(DWORD dwCtrlType);
//**********************************************************************************
//  This function will deal with printing multi-byte strings
//  in an ASCII (i.e., non-Unicode) program.
//  It passes both the byte-length of the string, for printing,
//  and the multi-byte length for updating screen position.
//**********************************************************************************
void dputsiw(const char *outstr, int wlen, int clen);
