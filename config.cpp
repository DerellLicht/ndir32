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
#include <tchar.h>

#include "common.h"
#include "ndir32.h"

attrib_list attr_table[MAX_EXT] ;
unsigned attrib_count = 0 ;

//#########################################################################
//    INI-file handlers
//#########################################################################

//*********************************************************************
//  default file colors
attrib_list const attr_default_list[] = {
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

//*********************************************************************
int write_default_ini_file(TCHAR *ini_str)
{
   FILE *ofile ;
   int j ;
   
   ofile = _tfopen(ini_str, _T("wt")) ;
   if (ofile == 0) 
      return errno ;
   
   fprintf(ofile, "; Default configuration file for NDIR32.EXE\n") ;
   fprintf(ofile, "; This file was generated automatically by NDIR, \n") ;
   fprintf(ofile, "; but it will not be over-written.  \n") ;
   fprintf(ofile, "; Edit this file as you desire to re-configure NDIR.\n") ;
   fprintf(ofile, "; \n") ;
   fprintf(ofile, "; Lines beginning with semicolons, and blank lines, are ignored\n") ;
   fprintf(ofile, "\n") ;
   
   //  generate state flags
   fprintf(ofile, "ucase=%u     ; use upper-case for all names\n", n.ucase) ;
   fprintf(ofile, "sort=%u      ; 0=ext, 1=name, 2=size, 3=date/time, 4=none\n", n.sort) ;
   fprintf(ofile, "reverse=%u   ; reverse normal sort order\n", n.reverse) ;
   fprintf(ofile, "clear=%u     ; clear screen before display\n", n.clear) ;
   fprintf(ofile, "pause=%u     ; pause on full screen\n", n.pause) ;
   fprintf(ofile, "format=%u    ; output columns, 0=1, 1=2, 2=4, 3=6, 5=3\n", n.format) ;
   fprintf(ofile, "minimize=%u  ; minimize header/footer\n", n.minimize) ;
   fprintf(ofile, "show_all=%u  ; show S/H/R files\n", n.show_all) ;
   fprintf(ofile, "dir_first=%u ; list directories only\n", n.dir_first) ;
   fprintf(ofile, "exec_only=%u ; show executables only (exe,com,bat,btm)\n", n.exec_only) ;
   fprintf(ofile, "long_attr=%u ; show executables only (exe,com,bat,btm)\n", n.long_attr) ;
   fprintf(ofile, "horz=%u      ; list files horizontally\n", n.horz) ;
   fprintf(ofile, "low_ascii=%u\n", n.low_ascii) ;
   fprintf(ofile, "color=%u\n", n.color) ;
   fprintf(ofile, "showSHRfiles=%u  ; use diff attrib for S/H/R files\n", n.showSHRfiles) ;
   // fprintf(ofile, "ega_keep=%u  ; switch to 50-line mode\n", n.ega_keep) ;
   fprintf(ofile, "fdate_option=%u  ; 0=LastWriteTime (default), 1=LastAccessTime, 2=CreationTime\n", n.fdate_option) ;

   //  generate default colors
   fprintf(ofile, "\n") ;
   fprintf(ofile, "global display colors.\n") ;
   fprintf(ofile, "All colors can be decimal or hex (preceded by 0x)\n") ;
   fprintf(ofile, "colorlogo=%u\n", n.colorlogo) ;
   fprintf(ofile, "colornhead=%u\n", n.colornhead) ;
   fprintf(ofile, "colorxhead=%u\n", n.colorxhead) ;
   fprintf(ofile, "colorframe=%u\n", n.colorframe) ;
   fprintf(ofile, "colorattr=%u\n", n.colorattr) ;
   fprintf(ofile, "colorsize=%u\n", n.colorsize) ;
   fprintf(ofile, "colordate=%u\n", n.colordate) ;
   fprintf(ofile, "colortime=%u\n", n.colortime) ;
   fprintf(ofile, "colordir=%u\n", n.colordir) ;
   fprintf(ofile, "colorSHR=%u\n", n.colorSHR) ;
   fprintf(ofile, "colordefalt=%u\n", n.colordefalt) ;

   //  generate colors for desired extentions
   // attrib_list attr_default_list[] = {
   fprintf(ofile, "\n") ;
   fprintf(ofile, "; assign colors to file extensions.\n") ;
   fprintf(ofile, "; You can add your extensions here, limited to 200 extentions\n") ;
   fprintf(ofile, "; Wildcards are supported!!\n") ;
   fprintf(ofile, "; two formats are supported:\n") ;
   fprintf(ofile, "; ext=color   and\n") ;
   fprintf(ofile, "; color:ext,ext,ext,ext\n") ;
   fprintf(ofile, "; Note that period is required for each extension\n") ;
   for (j=0; attr_default_list[j].ext[0] != 0; j++) {
      fprintf(ofile, "%s=%u\n", attr_default_list[j].ext, 
                                attr_default_list[j].attr ) ;
   }

   //  generate directory-tree colors
   fprintf(ofile, "\n") ;
   fprintf(ofile, "; assigned colors for levels in directory tree.\n") ;
   fprintf(ofile, "; Each line defines an additional level, limit=20 colors\n") ;
   for (j=0; j<MAX_DIR_ENTRY; j++) {
      fprintf(ofile, ":%u\n", dir_default_list[j] ) ;
   }

   fclose(ofile) ;
   return 0;
}

//*********************************************************************
struct ini_entry {
   char  *lvalue ;
   uchar *rvalue ;
};
static ini_entry const ndir_ini[] = {
{ "ucase",        &n.ucase },
{ "sort",         &n.sort },
{ "reverse",      &n.reverse },
{ "clear",        &n.clear },
{ "pause",        &n.pause },
{ "format",       &n.format },
{ "minimize",     &n.minimize },
{ "show_all",     &n.show_all },
{ "dir_first",    &n.dir_first },
{ "exec_only",    &n.exec_only },
{ "horz",         &n.horz },
{ "low_ascii",    &n.low_ascii },
{ "color",        &n.color },
{ "showSHRfiles", &n.showSHRfiles },
// { "ega_keep",     &n.ega_keep },
// { "lfn_off",      &n.lfn_off },
{ "fdate_option", &n.fdate_option },
{ "long_attr",    &n.long_attr },
{ "colorlogo",    &n.colorlogo },
{ "colornhead",   &n.colornhead },
{ "colorxhead",   &n.colorxhead },
{ "colorframe",   &n.colorframe },
{ "colorattr",    &n.colorattr },
{ "colorsize",    &n.colorsize },
{ "colordate",    &n.colordate },
{ "colortime",    &n.colortime },
{ "colordir",     &n.colordir },
{ "colorSHR",     &n.colorSHR },
{ "colordefalt",  &n.colordefalt },
{ "sizeopt",      &n.size_display },
{ 0, 0} } ;

void parse_ini_line(char *iniptr)
{
   char *eqptr ;
   int j ;

   eqptr = _tcschr(iniptr, '=') ;
   if (eqptr == 0)
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

