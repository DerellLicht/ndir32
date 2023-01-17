//***************************************************************************
//  CONIO32.HPP: Template for 32-bit console programs                        
//                                                                           
//  Written by:   Daniel D. Miller                                           
//                                                                           
//  Last Update:  01/17/23
//***************************************************************************

#define  MAX_CHAR_COLS     1024

//**********************************************************
void console_init(char *title);
void hide_cursor(void);

void restore_console_attribs(void);
bool is_redirected(void);
CHAR get_char(void);
WORD get_scode(void);
unsigned get_window_rows(void);
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
void dputc(const CHAR c);
void dprintc(unsigned row, unsigned col, const char outchr);
void dprintc(unsigned row, unsigned col, unsigned attr, const char outchr);
void dputnchar(CHAR chr, CHAR attr, int count);
void dputs(const char *outstr);
void dprints(unsigned row, unsigned col, const char* outstr);
BOOL control_handler(DWORD dwCtrlType);
