//*************************************************************************
//                                                                 
//  NDIR32.CPP - The Ultimate directory program (32-bit).          
//                                                                 
//  Written by:   Daniel D. Miller  (the derelict)                 
//                                                                 
//  compile with  makefile                                         
//                                                                 
//*************************************************************************
//  DAN:  Please update revisions.txt with each change.            
//*************************************************************************

#include <windows.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>     //  getenv(), exit()
#include <ctype.h>

#include "ndir32.h"
#include "conio32.hpp"

#define  VER_NUMBER "2.50"

//lint -esym(843, Version, ShortVersion) could be declared as const
char *Version = " NDIR.EXE, Version " VER_NUMBER " " ;
char *ShortVersion = " NDIR " VER_NUMBER " " ;

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' (line 134, file Ndir32.cpp) could be made static
//lint -e714  Symbol '_CRT_glob' (line 134, file Ndir32.cpp) not referenced
int _CRT_glob = 0 ;

//  cmd_line.cpp
extern void parse_command_string(char *cmdstr) ;

/***********************  qualify() constants  ************************/
#define  QUAL_WILDCARDS    0x01
#define  QUAL_NO_PATH      0x02
#define  QUAL_IS_FILE      0x04
#define  QUAL_INV_DRIVE    0x80

//  from CMD_LINE.CPP
extern void parse_command_args(int start, int argc, char** argv);

//  from QUALIFY.CPP
extern unsigned qualify(char *argptr, int lfn_off);

// extern void output_html_header(char *title);
// extern void output_html_footer(void);

//***************  function prototypes  ***************
static void process_filespecs(void);
static void sort_target_paths(void);
static void read_config_file(void);

//*****************************************************************************
// ULLONG_MAX = 18,446,744,073,709,551,615
//*****************************************************************************
//lint -esym(714, convert_to_commas)
//lint -esym(759, convert_to_commas)
//lint -esym(765, convert_to_commas)
char *convert_to_commas(ULONGLONG uli, char *outstr)
{  //lint !e1066
   int slen, inIdx, j ;
   char *strptr ;
   char temp_ull_str[MAX_ULL_COMMA_LEN+1] ;
   static char local_ull_str[MAX_ULL_COMMA_LEN+1] ;
   if (outstr == NULL)
       outstr = local_ull_str ;

   sprintf(temp_ull_str, "%"PRIu64"", uli);
   // sprintf(temp_ull_str, "%llu", uli);
   // sprintf(temp_ull_str, "%I64u", uli);
   // _ui64toa(uli, temp_ull_str, 10) ;
   slen = strlen(temp_ull_str) ;
   inIdx = --slen ;//  convert byte-count to string index 

   //  put NULL at end of output string
   strptr = outstr + MAX_ULL_COMMA_LEN ;
   *strptr-- = 0 ;   //  make sure there's a NULL-terminator

   for (j=0; j<slen; j++) {
      *strptr-- = temp_ull_str[inIdx--] ;
      if ((j+1) % 3 == 0)
         *strptr-- = ',' ;
   }
   *strptr = temp_ull_str[inIdx] ;

   //  copy string from tail-aligned to head-aligned
   strcpy(outstr, strptr) ;
   return outstr ;
}

//***********************************************************
//  DEBUG function: insert filespecs in display list
//***********************************************************
void insert_target_filespec(char *fstr)
   {
   // target[tcount] = new char[PATH_MAX] ;
   target[tcount] = (char *) malloc(PATH_MAX) ;
   if (target[tcount] == NULL)
      error_exit(OUT_OF_MEMORY, NULL) ;
   strcpy(target[tcount], fstr) ;

   unsigned result = qualify(target[tcount], n.lfn_off) ;
   if (result == QUAL_INV_DRIVE)
      error_exit(INV_DRIVE, target[tcount]) ;

   tcount++ ;
   }

//*****************************************************************
static char ininame[PATH_MAX] ;

int main(int argc, char **argv)
   {
   console_init(Version) ;

   //***********************************************************
   //  Check for NDIR environment variable
   //***********************************************************

   //  get program filename
   int startIdx = 1 ;
   char exename[PATH_MAX] ;

   // for (int j = 1; j < argc; j++) {
   //    printf("0: %s\n", argv[j]) ;
   // }

   //  interesting lessons from WinNT 4.0:
   //  If the OS is WinNT 4.0, and;
   //  If the executable file is located in the current directory,
   //  THEN:
   //    argv[0] does NOT contain the fully-qualified
   //    path of the EXE, it *only* contains the EXE name.
   //    In all other situations, argv[0] is fully qualified!!
   //  
   //  P.S.  While we're here, derive default INI filename also
   // printf("argv0=%s\n", argv[0]) ;
   char* strptr = strrchr(argv[0], '\\') ;
   //  no path present
   if (strptr == 0) {
      SearchPath(NULL, argv[0], ".exe", PATH_MAX, ininame, NULL) ;
      strptr = strrchr(ininame, '\\') ;
      if (strptr != 0) 
         strcpy(strptr, "\\ndir.ini") ;

      strcpy(exename, argv[0]) ;
      // ininame[0] = 0 ;  //  ONLY support current location
   }
   else {
      //  pick up INI filename
      strcpy(ininame, argv[0]) ;
      strptr = strrchr(ininame, '\\') ;
      if (strptr == 0)
         return 1;
      strcpy(strptr, "\\ndir.ini") ;
      
      //  now process exe name for getenv()
      strptr++ ;  //lint !e613:  skip backslash
      strcpy(exename, strptr) ;  //lint !e613
      strptr = strchr(exename, '.') ;
      if (strptr != 0) 
         *strptr = 0 ;  //  strip the extension
   }

   char* options = getenv(exename) ; 
   if (options != 0) {
      argv[0] = options ;
      startIdx = 0 ;
   }
// printf("ininame=%s\n", ininame) ;
// getchar() ;

   // for (int j = startIdx; j < argc; j++) {
   //    printf("1: %s\n", argv[j]) ;
   // }
   //***********************************************************
   //  first read default settings
   //***********************************************************
   read_config_file() ;

   //***********************************************************
   //  override defaults with command line and environment vars
   //***********************************************************
   parse_command_args(startIdx, argc, argv) ;
   verify_flags() ;  //  this may add extensions if -x is given

   //***********************************************************
   //  Execute the requested command
   //***********************************************************
   // output_html_header("ndir32");
   display_logo() ;

   if (n.help)
      info(helptxt) ;
   else if (n.info)
      info(idtxt) ;
   else if (n.drive_summary)
      display_drive_summary() ;
   else {
      //  If no filespec was given, insert current path with *.*
      if (tcount==0)
         insert_target_filespec(".") ;

      sort_target_paths() ;      //  LFN: okay
      process_filespecs() ;
   }

   // output_html_footer();
   error_exit(DATA_OKAY, NULL) ;
   return 0 ;
   }

/**********************************************************************/
/**                     File listing routines                        **/        
/**********************************************************************/
static char fi_name[PATH_MAX], fi_ext[PATH_MAX] ;
static char fj_name[PATH_MAX], fj_ext[PATH_MAX] ;

static void process_filespecs(void)
   {
   char* strptr ;
   unsigned i, j, k ;

   /***********************************************************************/
   /*************************  loop on filespecs  *************************/
   /***********************************************************************/

   //***********************************************************************
   //  when tree listing is selected, it is assumed that each specified
   //  argument is a separate path, and that no wildcards nor specific 
   //  filenames were provided.
   //  If such anomalies are presented, unpredictable results will occur.
   //***********************************************************************
   if (n.tree == 1  ||  n.tree == 4  ||  n.tree == 5) {
      tree_listing() ;
   }
   else if (tcount == 1  &&  !n.exec_only) {
      start = finish = 0 ;

      //  check for validity of long_filename functions
      dname[0] = (char) *target[start] ;

      // if (n.lfn_off == 1) 
      //    lfn_supported = 0 ;
      // else 
      //    IFF (is_lfn_available(dname, (char far *) fsn_bfr) != 0) 
      //    THENN 0  
      //    ELSSE 1 ;
      // lfn_supported = 1 ;
      lfn_supported = 1 - n.lfn_off ;

      //  in lfn format, convert /3 to /4
      if (lfn_supported != 0  &&  columns == 3) 
         columns = 4 ;

      //  Extract base path from first filespec,
      //  and strip off filename
      strcpy(base_path, target[start]) ;
      strptr = strrchr(base_path, '\\') ;
      if (strptr != 0) {
          strptr++ ;  //lint !e613  skip past backslash, to filename
         *strptr = 0 ;  //  strip off filename
      }
      base_len = strlen(base_path) ;

      //**************************************************
      get_disk_info(base_path) ;

      //**************************************************
      //  initialize file pointer and filecount,
      //  in case of multiple filespecs.
      //**************************************************
      if (ftop != NULL)
         {
         ffdata *ftemp = ftop ;
         ffdata *fkill ;
         while (ftemp != NULL)
            {
            fkill = ftemp ;
            ftemp = ftemp->next ;
            free(fkill) ;
            }
         ftop = NULL ;
         }
      filecount = 0 ;

      //**************************************************
      //  Call directory_tree or file_listing routines,
      //  as specified by flags.
      //**************************************************
      file_listing() ;
      }
   else 
      {
      int temp_columns = columns ;

      start = 0 ;
      while (1)
         {
         //  Extract base path from first filespec,
         //  and strip off filename
         j = start ;

         //  check for validity of long_filename functions
         dname[0] = *target[start] ;
         // if (n.lfn_off == 1) 
         //    lfn_supported = 0 ;
         // else 
         //    lfn_supported =  
         //       IFF (is_lfn_available(dname, (char far *) fsn_bfr) != 0) 
         //       THENN 0  
         //       ELSSE 1 ;
         lfn_supported = 1 ;

         //  in lfn format, convert /3 to /4
         if (lfn_supported != 0  &&  columns == 3) 
            columns = 4 ;

         //  Extract base path from first filespec,
         //  and strip off filename
         strcpy(base_path, target[start]) ;
         //lint -esym(613,strptr) 
         strptr = strrchr(base_path, '\\') ;
         if (strptr != 0) {
            strptr++ ;  // skip past backslash, to filename
            *strptr = 0 ;  //  strip off filename
         }
         base_len = strlen(base_path) ;

         //**************************************************
         get_disk_info(base_path) ;

         //  seek out all other filespecs with same path
         j++ ;
         while (1) {
            if (j >= tcount) {
               finish = j-1 ;
               break;
            }
            else {
               //  strip filename from next argument
               strcpy(tempstr, target[j]) ;
               strptr = strrchr(tempstr, '\\') ;   //lint !e613
               strptr++ ;
               *strptr = 0 ;

               //  now see if they are equal
               if (strcmp(base_path, tempstr) != 0) {
                  finish = j-1 ;
                  break;
               }
               else 
                  j++ ;
            }
         }

         //********************************************************
         //  DELETE DUPLICATE FILESPECS from target array.
         //  Delete record if no differences found.   
         //  Compare file and ext separately.         
         //  This routine uses selection sort, because the list
         //  usually only has a couple of items in it.
         //********************************************************
         for (i=start ; i< finish ; i++)
         for (j=i+1   ; j<=finish ; j++) {
            //  extract filename and extension file target string
            //  to compare for duplicate filespecs.
            strcpy(fi_name, &target[i][base_len]) ;
            strptr = strrchr(fi_name, '.') ; //lint !e613
            if (strptr != 0) {
               *strptr++ = 0 ;   //lint !e613
               strcpy(fi_ext, strptr) ;
            } else {
               fi_ext[0] = 0 ;
            }

            strcpy(fj_name, &target[j][base_len]) ;
            strptr = strrchr(fj_name, '.') ; //lint !e613
            if (strptr != 0) {
               *strptr++ = 0 ;   //lint !e613
               strcpy(fj_ext, strptr) ;
            } else {
               fj_ext[0] = 0 ;
            }

            //  Scan file name and extension for equality.
            //  If both filename and extension are equal, delete one.
            if (strcmpiwc(fi_name, fj_name)  &&  strcmpiwc(fi_ext, fj_ext)) {
               strptr = target[j] ;
               for (k=j+1; k<tcount; k++)
                   target[k] = target[k+1] ;
               tcount-- ;
               finish-- ;
               j-- ;
               free(strptr) ; // release allocated struct.
            }
         }
         // Ndir32.cpp  414  Info 850: for loop index variable 'j' 
         // whose type category is 'integral' 
         // is modified in body of the for loop that began at 'line 375'

         //**************************************************
         //  initialize file pointer and filecount,
         //  in case of multiple filespecs.
         //**************************************************
         if (ftop != NULL) //lint !e850
            {
            ffdata *ftemp = ftop ;
            ffdata *fkill ;
            while (ftemp != NULL)
               {
               fkill = ftemp ;
               ftemp = ftemp->next ;
               free(fkill) ;
               }
            ftop = NULL ;
            }
         filecount = 0 ;

         //**************************************************
         //  Call directory_tree or file_listing routines,
         //  as specified by flags.
         //**************************************************
         if (n.tree == 1)
            tree_listing() ;
         else
            file_listing() ;

         start = finish + 1 ;
         if (start >= tcount) 
            break;
         ncrlf() ;

         columns = temp_columns ;
         }  //  while not done

      }  //  if multiple filespecs are present
   } /*  end  process_filespecs() */

//**************************************************************
//  string compare routine, case-insensitive, 
//  wildcards are handled in the DOS fashion.
//**************************************************************
int strcmpiwc(const char *onestr, const char *twostr)
   {
   char onechar, twochar ;
   int k = 0 ;

   while (1) 
      {
      onechar = *(onestr+k) ;
      twochar = *(twostr+k) ;

      //  if both are at end of string and no differences
      //  have been found, the strings are equal.
      if (onechar == 0  &&  twochar == 0) 
         return 1;

      //  if one string is at end and the other is not,
      //  there is NOT a match.
      if (onechar == 0  ||  twochar == 0)
         return 0;

      //  at this point, neither char is NULL

      //  if either char is a 'match all' wildcard, the strings are equal
      if (onechar == '*'  ||  twochar == '*') 
         return 1 ;

      if (onechar == '?'  ||  twochar == '?') ; //  match continues


      else if (tolower(onechar) != tolower(twochar)) 
         return 0;

      k++ ;
      }
   }

//**************************************************
//  Sort filespecs alphabetically,
//  to group those in one directory.
//**************************************************
static void sort_target_paths(void)
   {
   char* strptr ;
   unsigned i, j ;

   for (j=0   ; j<tcount-1 ; j++)
   for (i=j+1 ; i<tcount ; i++)
   if (strcmp(target[j],target[i]) >  0)
      {
      strptr    = target[i] ;
      target[i] = target[j] ;
      target[j] = strptr ;
      }
   }

//***********************************************************
//  This replaces the CXL function of the same name
//***********************************************************
void set_lines(int crt_lines)
   {
   // COORD dwSize = { 80, crt_lines } ;
   // SetConsoleScreenBufferSize(hStdOut, dwSize) ;
   
   //  The preceding method changes the actual buffer size,
   //  not the window size, which may not be what is wanted
   //  under WinNT.  This method changes the actual window
   //  size, but positions the new window at the *top* of
   //  the screen buffer, which may give unexpected results
   //  if used with "don't clear screen" in a large window.
   //  Neither method is exactly correct in all cases,
   //  but will probably suffice most times...
   SMALL_RECT newwin = { 0, 0, 79, (SHORT) (crt_lines-1) } ;
   SetConsoleWindowInfo(hStdOut, TRUE, &newwin) ;
   }

//#########################################################################
//    INI-file handlers
//#########################################################################

//*********************************************************************
//  default file colors
static attrib_list attr_default_list[] = {
{ 0x02, ".1ST" }, { 0x0E, ".ARC" }, { 0x03, ".ASM" }, { 0x03, ".BAS" }, 
{ 0x04, ".BAT" }, { 0x03, ".C"   }, { 0x0C, ".COM" }, { 0x03, ".CPP" },
{ 0x02, ".DOC" }, { 0x0C, ".EXE" }, { 0x0D, ".H"   }, { 0x0D, ".HEX" }, 
{ 0x03, ".HPP" }, { 0x0D, ".INC" }, { 0x0D, ".LIB" }, { 0x0A, ".LST" },
{ 0x05, ".MAP" }, { 0x02, ".ME"  }, { 0x02, ".NOW" }, { 0x0D, ".OBJ" }, 
{ 0x03, ".PAS" }, { 0x02, ".TXT" }, {    0, "" }                 
} ;

//  default directory colors
static uchar const dir_default_list[MAX_DIR_ENTRY] = {
3, 4, 5, 6, 7, 8, 9, 0x0A,
3, 4, 5, 6, 7, 8, 9, 0x0A,
3, 4, 5, 6 } ;

static int write_default_ini_file(char *ini_str)
{
   FILE *ofile ;
   int j ;
   
   ofile = fopen(ini_str, "wt") ;
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
   fprintf(ofile, "ega_keep=%u  ; switch to 50-line mode\n", n.ega_keep) ;
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
   fprintf(ofile, "; Each line defines an additional level, limit=20\n") ;
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
{ "ega_keep",     &n.ega_keep },
{ "lfn_off",      &n.lfn_off },
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

static void parse_ini_line(char *iniptr)
{
   char *eqptr ;
   int j ;

   eqptr = strchr(iniptr, '=') ;
   if (eqptr == 0)
      return ;
   *eqptr++ = 0 ; //  NULL-terminate lvalue, point to rvalue
   for (j=0; ndir_ini[j].lvalue != 0; j++) {
      //  if we find the lvalue (pointed to by iniptr) in the 
      //  lookup table, assigned the rvalue (pointed to by eqptr)
      //  to the variable stored
      if (strcmpiwc(iniptr, ndir_ini[j].lvalue)) {
         *(ndir_ini[j].rvalue) = (uchar) strtoul(eqptr, 0, 0) ;
         // printf("found %s=%u\n", ndir_ini[j].lvalue, (int) *(ndir_ini[j].rvalue)) ;
         break;
      }
   }
}

//*********************************************************************
static void parse_color_entry(char *iniptr)
{
   char *eqptr ;
   char *hdptr ;
   char *tlptr ;
   uchar atr ;
   attrib_list *aptr ;

   //  check for multiple-color-entry forms...
   //0x32:.com,.bat,.btm,.sys
   //14:.arc,.tgz,.tar,.gz,.z,.zip,.bz2,.rar,.7z,.iso,.zcp
   hdptr = strchr(iniptr, ':') ;
   if (hdptr != 0) {
      *hdptr++ = 0 ; //  terminate attribute, point to first extension
      atr = (uchar) strtoul(iniptr, 0, 0) ;
      if (atr == 0)
         return ;
      
      while (LOOP_FOREVER) {
         //  make sure we don't overrun our table
         if (attrib_count >= MAX_EXT)
            return ;
         // if (*hdptr != '.')
         //    return ;
         tlptr = strchr(hdptr, ',') ;
         //  see if we're at end of line
         if (tlptr != 0) {
            *tlptr++ = 0 ; //  NULL-term extension
         }
         //  check for too-long extensions in INI file
         //  If extension in INI file is too long, just discard it
         uint extlen = (*hdptr == '.') ? MAX_EXT_SIZE : MAX_EXT_SIZE-1 ;
         if (strlen(hdptr) <= extlen) {
            aptr = &attr_table[attrib_count++] ;
            if (*hdptr == '.') {
               strcpy(aptr->ext, hdptr) ;
            }
            else {
               sprintf(aptr->ext, ".%s", hdptr) ;
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

      eqptr = strchr(iniptr, '=') ;
      if (eqptr == 0)
         return ;
      *eqptr++ = 0 ; //  NULL-terminate lvalue, point to rvalue

      //.ARC=14
      aptr = &attr_table[attrib_count++] ;
      strncpy(aptr->ext, iniptr, MAX_EXT_SIZE) ;
      aptr->attr = (uchar) strtoul(eqptr, 0, 0) ;
   }
}

//*********************************************************************
static void parse_dir_color_entry(char *iniptr)
{
   static int dcIdx = 0 ;
   iniptr++ ;  //  skip colon flag
   if (dcIdx < MAX_DIR_ENTRY) {
      dtree_colors[dcIdx++] = (uchar) strtoul(iniptr, 0, 0) ;
   }
}

//*********************************************************************
static int read_ini_file(char const * ini_str)
{
   FILE *ofile ;
   int slen ;
   char *strptr ;
   static char line[PATH_MAX] ;

// printf("reading %s\n", ini_str) ;
   ofile = fopen(ini_str, "rt") ;
   if (ofile == 0) 
      return errno ;

   while (fgets(line, sizeof(line), ofile) != 0) {
      //  strip off newline char
      slen = strlen(line) ;
      strptr = &line[slen-1] ;
      if (*strptr == '\n') {
         *strptr-- = 0 ;   //  strip off newline
         // slen-- ;
      }

      //  next, find and strip off comments
      strptr = strchr(line, ';') ;
      if (strptr != 0)
         *strptr-- = 0 ;

      //  skip blank lines
      slen = strlen(line) ;
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
static char const local_ini_name[] = ".\\ndir.ini" ;
static char ini_path[PATH_MAX] ;

static void read_config_file(void)
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
      strcpy(ini_path, local_ini_name) ;
   } 
   //  If global INI filename IS present, try to load it
   else {
      result = read_ini_file(ininame) ;
      if (result == 0) {
         return ;
      }
      strcpy(ini_path, ininame) ;
   }

   //  If we couldn't open any existing INI files,
   //  generate default file in appropriate location.
   result = write_default_ini_file(ini_path) ;
   if (result != 0) {
      // perror(ini_path) ;
      sprintf (tempstr, "path [%s]\n", ini_path);
      nputs (0xA, tempstr);
      sprintf (tempstr, "FindFirst: %s\n", get_system_message ());
      nputs (0xA, tempstr);
   }
   //  try to read again, after writing defaults
   read_ini_file(ini_path) ;
}

//********************************************************************
//  On Windows platform, try to redefine printf/fprintf
//  so we can output code to a debug window.
//  Also, shadow syslog() within OutputDebugStringA()
//  Note: printf() remapping was unreliable,
//  but syslog worked great.
//********************************************************************
//lint -esym(714, syslog)
//lint -esym(759, syslog)
//lint -esym(765, syslog)
int syslog(const char *fmt, ...)
{
   char consoleBuffer[3000] ;
   va_list al; //lint !e522

//lint -esym(526, __builtin_va_start)
//lint -esym(628, __builtin_va_start)
   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   // if (common_logging_enabled)
   //    fprintf(cmlogfd, "%s", consoleBuffer) ;
   OutputDebugStringA(consoleBuffer) ;
   va_end(al);
   return 1;
}


