//***************************************************************************
//  CONIO32.HPP: Template for 32-bit console programs                        
//                                                                           
//  Written by:   Daniel D. Miller                                           
//                                                                           
//  Last Update:  02/05/03 08:09                                             
//                                                                           
//  compile with:    cl /c /W3 /O2 /G4 conio32.cpp                              
//    NOTE: This program requires Microsoft Visual C++ 4.0 or greater.       
//                                                                           
//***************************************************************************

extern HANDLE hStdOut, hStdIn ;

//**********************************************************
void console_init(char *title);
void hide_cursor(void);
void perr(PCHAR szFileName, int line, PCHAR szApiName, DWORD dwError) ;

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
unsigned drandom(unsigned rmax);
BOOL control_handler(DWORD dwCtrlType);
char *get_system_message(void);
char *get_system_message(DWORD errcode);

#define PERR(bSuccess, api) {if (!(bSuccess)) perr(__FILE__, __LINE__, api, GetLastError());}

