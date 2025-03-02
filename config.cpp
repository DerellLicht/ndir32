//*****************************************************************
//  config.cpp - NDIR config file handling                       
//                                                                 
//  Written by:   Daniel D. Miller  (the Derelict)                 
//                                                                 
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>
#endif
#include <ctype.h>

#include "common.h"
#include "ndir32.h"

#define  MAX_EXT        200
struct attrib_list {
   uchar  attr ;
   TCHAR  ext[MAX_EXT_SIZE+1] ;
} ;

static attrib_list attr_table[MAX_EXT] ;
static unsigned attrib_count = 0 ;

//#########################################################################
//    INI-file handlers
//#########################################################################

//*********************************************************************
//  default file colors
static attrib_list const attr_default_list[] = {
{ 0x02, _T(".1ST") }, { 0x0E, _T(".ARC") }, { 0x03, _T(".ASM") }, { 0x03, _T(".BAS") }, 
{ 0x04, _T(".BAT") }, { 0x03, _T(".C")   }, { 0x0C, _T(".COM") }, { 0x03, _T(".CPP") },
{ 0x02, _T(".DOC") }, { 0x0C, _T(".EXE") }, { 0x0D, _T(".H")   }, { 0x0D, _T(".HEX") }, 
{ 0x03, _T(".HPP") }, { 0x0D, _T(".INC") }, { 0x0D, _T(".LIB") }, { 0x0A, _T(".LST") },
{ 0x05, _T(".MAP") }, { 0x02, _T(".ME")  }, { 0x02, _T(".NOW") }, { 0x0D, _T(".OBJ") }, 
{ 0x03, _T(".PAS") }, { 0x02, _T(".TXT") }, {    0, _T("") }                 
} ;

//  default directory colors
static uchar const dir_default_list[MAX_DIR_ENTRY] = {
3, 4, 5, 6, 7, 8, 9, 0x0A,
3, 4, 5, 6, 7, 8, 9, 0x0A,
3, 4, 5, 6 } ;

//***************************************************************
void getcolor(ffdata *fnew)
{
   unsigned j;
   attrib_list *aptr;

   for (j = 0; j < attrib_count; j++) {
      aptr = &attr_table[j];
      if (strcmpiwc (fnew->ext, aptr->ext) != 0) {
         fnew->color = aptr->attr;
         return;
      }
   }
   fnew->color = n.colordefalt; //  if not found, assign default color
}  //lint !e429  Custodial pointer 'fnew' has not been freed or returned

//*********************************************************************
static void parse_color_entry(TCHAR *iniptr)
{
   TCHAR *eqptr ;
   TCHAR *hdptr ;
   TCHAR *tlptr ;
   uchar atr ;
   attrib_list *aptr ;

   //  check for multiple-color-entry forms...
   //0x32:.com,.bat,.btm,.sys
   //14:.arc,.tgz,.tar,.gz,.z,.zip,.bz2,.rar,.7z,.iso,.zcp
   hdptr = _tcschr(iniptr, _T(':')) ;
   if (hdptr != 0) {
      *hdptr++ = 0 ; //  terminate attribute, point to first extension
      atr = (uchar) _tcstoul(iniptr, 0, 0) ;
      if (atr == 0)
         return ;
      
      while (LOOP_FOREVER) {
         //  make sure we don't overrun our table
         if (attrib_count >= MAX_EXT)
            return ;
         // if (*hdptr != '.')
         //    return ;
         tlptr = _tcschr(hdptr, _T(',')) ;
         //  see if we're at end of line
         if (tlptr != 0) {
            *tlptr++ = 0 ; //  NULL-term extension
         }
         //  check for too-long extensions in INI file
         //  If extension in INI file is too long, just discard it
         uint extlen = (*hdptr == _T('.')) ? MAX_EXT_SIZE : MAX_EXT_SIZE-1 ;
         if (_tcslen(hdptr) <= extlen) {
            aptr = &attr_table[attrib_count++] ;
            if (*hdptr == _T('.')) {
               _tcscpy(aptr->ext, hdptr) ;
            }
            else {
               _stprintf(aptr->ext, _T(".%s"), hdptr) ;
            }
            aptr->attr = atr ;
         }
         if (tlptr == 0) {
            break;
         }
         hdptr = tlptr ;
      }
      
   }
   //  handle single-color entries
   //.FAQ=2
   else {
      //  make sure we don't overrun our table
      if (attrib_count >= MAX_EXT)
         return ;

      eqptr = _tcschr(iniptr, '=') ;
      if (eqptr == 0)
         return ;
      *eqptr++ = 0 ; //  NULL-terminate lvalue, point to rvalue

      //.ARC=14
      aptr = &attr_table[attrib_count++] ;
      _tcsncpy(aptr->ext, iniptr, MAX_EXT_SIZE) ;
      aptr->attr = (uchar) _tcstoul(eqptr, 0, 0) ;
   }
}

//*********************************************************************
static void parse_dir_color_entry(TCHAR *iniptr)
{
   static int dcIdx = 0 ;
   iniptr++ ;  //  skip colon flag
   if (dcIdx < MAX_DIR_ENTRY) {
      dtree_colors[dcIdx++] = (uchar) _tcstoul(iniptr, 0, 0) ;
   }
}

//*********************************************************************
static int write_default_ini_file(TCHAR *ini_str)
{
   FILE *ofile ;
   int j ;
   
   ofile = _tfopen(ini_str, _T("wt")) ;
   if (ofile == 0) 
      return errno ;
   
   _ftprintf(ofile, _T("; Default configuration file for NDIR32.EXE\n")) ;
   _ftprintf(ofile, _T("; This file was generated automatically by NDIR, \n")) ;
   _ftprintf(ofile, _T("; but it will not be over-written.  \n")) ;
   _ftprintf(ofile, _T("; Edit this file as you desire to re-configure NDIR.\n")) ;
   _ftprintf(ofile, _T("; \n")) ;
   _ftprintf(ofile, _T("; Lines beginning with semicolons, and blank lines, are ignored\n")) ;
   _ftprintf(ofile, _T("\n")) ;
   
   //  generate state flags
   _ftprintf(ofile, _T("ucase=%u     ; use upper-case for all names\n"), n.ucase) ;
   _ftprintf(ofile, _T("sort=%u      ; 0=ext, 1=name, 2=size, 3=date/time, 4=none\n"), n.sort) ;
   _ftprintf(ofile, _T("reverse=%u   ; reverse normal sort order\n"), n.reverse) ;
   _ftprintf(ofile, _T("clear=%u     ; clear screen before display\n"), n.clear) ;
   _ftprintf(ofile, _T("pause=%u     ; pause on full screen\n"), n.pause) ;
   _ftprintf(ofile, _T("format=%u    ; output columns, 0=1, 1=2, 2=4, 3=6, 5=3\n"), n.format) ;
   _ftprintf(ofile, _T("minimize=%u  ; minimize header/footer\n"), n.minimize) ;
   _ftprintf(ofile, _T("show_all=%u  ; show S/H/R files\n"), n.show_all) ;
   _ftprintf(ofile, _T("dir_first=%u ; list directories only\n"), n.dir_first) ;
   _ftprintf(ofile, _T("exec_only=%u ; show executables only (exe,com,bat,btm)\n"), n.exec_only) ;
   _ftprintf(ofile, _T("long_attr=%u ; show executables only (exe,com,bat,btm)\n"), n.long_attr) ;
   _ftprintf(ofile, _T("horz=%u      ; list files horizontally\n"), n.horz) ;
   _ftprintf(ofile, _T("low_ascii=%u\n"), n.low_ascii) ;
   _ftprintf(ofile, _T("color=%u\n"), n.color) ;
   _ftprintf(ofile, _T("showSHRfiles=%u  ; use diff attrib for S/H/R files\n"), n.showSHRfiles) ;
   // _ftprintf(ofile, "ega_keep=%u  ; switch to 50-line mode\n", n.ega_keep) ;
   _ftprintf(ofile, _T("fdate_option=%u  ; 0=LastWriteTime (default), 1=LastAccessTime, 2=CreationTime\n"), n.fdate_option) ;

   //  generate default colors
   _ftprintf(ofile, _T("\n")) ;
   _ftprintf(ofile, _T("global display colors.\n")) ;
   _ftprintf(ofile, _T("All colors can be decimal or hex (preceded by 0x)\n")) ;
   _ftprintf(ofile, _T("colorlogo=%u\n"), n.colorlogo) ;
   _ftprintf(ofile, _T("colornhead=%u\n"), n.colornhead) ;
   _ftprintf(ofile, _T("colorxhead=%u\n"), n.colorxhead) ;
   _ftprintf(ofile, _T("colorframe=%u\n"), n.colorframe) ;
   _ftprintf(ofile, _T("colorattr=%u\n"), n.colorattr) ;
   _ftprintf(ofile, _T("colorsize=%u\n"), n.colorsize) ;
   _ftprintf(ofile, _T("colordate=%u\n"), n.colordate) ;
   _ftprintf(ofile, _T("colortime=%u\n"), n.colortime) ;
   _ftprintf(ofile, _T("colordir=%u\n"), n.colordir) ;
   _ftprintf(ofile, _T("colorSHR=%u\n"), n.colorSHR) ;
   _ftprintf(ofile, _T("colordefalt=%u\n"), n.colordefalt) ;

   //  generate colors for desired extentions
   // attrib_list attr_default_list[] = {
   _ftprintf(ofile, _T("\n")) ;
   _ftprintf(ofile, _T("; assign colors to file extensions.\n")) ;
   _ftprintf(ofile, _T("; You can add your extensions here, limited to 200 extentions\n")) ;
   _ftprintf(ofile, _T("; Wildcards are supported!!\n")) ;
   _ftprintf(ofile, _T("; two formats are supported:\n")) ;
   _ftprintf(ofile, _T("; ext=color   and\n")) ;
   _ftprintf(ofile, _T("; color:ext,ext,ext,ext\n")) ;
   _ftprintf(ofile, _T("; Note that period is required for each extension\n")) ;
   for (j=0; attr_default_list[j].ext[0] != 0; j++) {
      _ftprintf(ofile, _T("%s=%u\n"), attr_default_list[j].ext, 
                                attr_default_list[j].attr ) ;
   }

   //  generate directory-tree colors
   _ftprintf(ofile, _T("\n")) ;
   _ftprintf(ofile, _T("; assigned colors for levels in directory tree.\n")) ;
   _ftprintf(ofile, _T("; Each line defines an additional level, limit=20 colors\n")) ;
   for (j=0; j<MAX_DIR_ENTRY; j++) {
      _ftprintf(ofile, _T(":%u\n"), dir_default_list[j] ) ;
   }

   fclose(ofile) ;
   return 0;
}

//*********************************************************************
struct ini_entry {
   TCHAR  *lvalue ;
   uchar *rvalue ;
};
static ini_entry const ndir_ini[] = {
{ _T("ucase"),        &n.ucase },
{ _T("sort"),         &n.sort },
{ _T("reverse"),      &n.reverse },
{ _T("clear"),        &n.clear },
{ _T("pause"),        &n.pause },
{ _T("format"),       &n.format },
{ _T("minimize"),     &n.minimize },
{ _T("show_all"),     &n.show_all },
{ _T("dir_first"),    &n.dir_first },
{ _T("exec_only"),    &n.exec_only },
{ _T("horz"),         &n.horz },
{ _T("low_ascii"),    &n.low_ascii },
{ _T("color"),        &n.color },
{ _T("showSHRfiles"), &n.showSHRfiles },
// { _T("ega_keep"),     &n.ega_keep },
// { _T("lfn_off"),      &n.lfn_off },
{ _T("fdate_option"), &n.fdate_option },
{ _T("long_attr"),    &n.long_attr },
{ _T("colorlogo"),    &n.colorlogo },
{ _T("colornhead"),   &n.colornhead },
{ _T("colorxhead"),   &n.colorxhead },
{ _T("colorframe"),   &n.colorframe },
{ _T("colorattr"),    &n.colorattr },
{ _T("colorsize"),    &n.colorsize },
{ _T("colordate"),    &n.colordate },
{ _T("colortime"),    &n.colortime },
{ _T("colordir"),     &n.colordir },
{ _T("colorSHR"),     &n.colorSHR },
{ _T("colordefalt"),  &n.colordefalt },
{ _T("sizeopt"),      &n.size_display },
{ 0, 0} } ;

static void parse_ini_line(TCHAR *iniptr)
{
   TCHAR *eqptr ;
   int j ;

   eqptr = _tcschr(iniptr, _T('=')) ;
   if (eqptr == NULL)
      return ;
   *eqptr++ = 0 ; //  NULL-terminate lvalue, point to rvalue
   for (j=0; ndir_ini[j].lvalue != 0; j++) {
      //  if we find the lvalue (pointed to by iniptr) in the 
      //  lookup table, assigned the rvalue (pointed to by eqptr)
      //  to the variable stored
      if (strcmpiwc(iniptr, ndir_ini[j].lvalue)) {
         *(ndir_ini[j].rvalue) = (uchar) _tcstoul(eqptr, 0, 0) ;
         // printf("found %s=%u\n", ndir_ini[j].lvalue, (int) *(ndir_ini[j].rvalue)) ;
         break;
      }
   }
}

//*********************************************************************
static int read_ini_file(TCHAR const * ini_str)
{
   FILE *ofile ;
   int slen ;
   TCHAR *strptr ;
   static TCHAR line[PATH_MAX] ;

// printf("reading %s\n", ini_str) ;
   ofile = _tfopen(ini_str, _T("rt")) ;
   if (ofile == 0) 
      return errno ;

   while (_fgetts(line, sizeof(line), ofile) != 0) {
      //  strip off newline char
      slen = _tcslen(line) ;
      strptr = &line[slen-1] ;
      if (*strptr == '\n') {
         *strptr-- = 0 ;   //  strip off newline
         // slen-- ;
      }

      //  next, find and strip off comments
      strptr = _tcschr(line, ';') ;
      if (strptr != 0)
         *strptr-- = 0 ;

      //  skip blank lines
      slen = _tcslen(line) ;
      if (slen == 0)
         continue;
      strptr = &line[slen-1] ;

      //  then strip off tailing spaces
      while (slen > 0  &&  *strptr == ' ') {
         *strptr-- = 0 ;
         slen-- ;
      }
      if (slen == 0)
         continue;

      //  now we should have a simple line in field=value format.
      //  See if we can parse it...

      //  see whether we're dealing with an extention-color entry,
      //  or a flags entry
      if (line[0] == '!') {
         parse_command_string(line) ;
      } else if (line[0] == '.') {
         parse_color_entry(line) ;
      } else if (line[0] >= '0'  &&  line[0] <= '9') {
         parse_color_entry(line) ;
      } else if (line[0] == ':') {
         parse_dir_color_entry(line) ;
      } else {
// printf("line=%s", line) ;
         parse_ini_line(line) ;
      }
   }
   
   fclose(ofile) ;
   return 0;
}

//*********************************************************************
static TCHAR const local_ini_name[] = _T(".\\ndir.ini") ;
static TCHAR ini_path[PATH_MAX] ;

void read_config_file(void)
{
   int result ;

   //  search for existing file.
   //  1. look in current directory
   //  2. if not found, search location of executable
   //  3. if not found, generate default file in location of executable
   // printf("seek local ini=%s\n", local_ini_name) ;
   result = read_ini_file(local_ini_name) ;
   if (result == 0) {
      return ;
   }

   //  If search for local file failed, try location of executable,
   //  if that isn't the local directory.

   //  If global INI filename isn't present, give up on search.
   //  This will usually mean that we are running under WinNT 4.0,
   //  and the executable is already in the current directory.
   //  Just write the file in the current directory.
   // printf("ininame=%s\n", ininame) ;
   // getchar() ;
   if (ininame[0] == 0) {
      _tcscpy(ini_path, local_ini_name) ;
   } 
   //  If global INI filename IS present, try to load it
   else {
      result = read_ini_file(ininame) ;
      if (result == 0) {
         return ;
      }
      _tcscpy(ini_path, ininame) ;
   }

   //  If we couldn't open any existing INI files,
   //  generate default file in appropriate location.
   result = write_default_ini_file(ini_path) ;
   if (result != 0) {
      // perror(ini_path) ;
      _stprintf (tempstr, _T("path [%s]\n"), ini_path);
      nputs (0xA, tempstr);
      _stprintf (tempstr, _T("FindFirst: %s\n"), get_system_message ());
      nputs (0xA, tempstr);
   }
   //  try to read again, after writing defaults
   read_ini_file(ini_path) ;
}

