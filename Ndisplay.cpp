//*****************************************************************
//  Copyright (c) 1998-2023 Daniel D. Miller                       
//  NDISPLAY.CPP - NDIR display handlers                           
//*****************************************************************

#undef __STRICT_ANSI__
#ifdef USE_64BIT
#define  _WIN32_WINNT    0x0600
#endif
#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>  //  PATH_MAX
#endif
#ifdef USE_64BIT
#include <fileapi.h>
#endif

#include "ndir32.h"
#include "conio32.h"
#include "file_fmts.h"

extern int get_mp3_info(char *fname, char *mlstr);
//lint -e626   argument inconsistent with format
//lint -e559   Size of argument no. 3 inconsistent with format

//  from NDIR.CPP
extern char *Version ;
extern char *ShortVersion;

static unsigned linecnt = 0 ;           //  non-color display line counter

//  this array is 64 elements, not 32 (== 2^5) because 
//  the volume_label bit (0x80) is not displayed.  
//  There are actually six bits in the field.
// static char attrstr[64][6] = {
//   "_____", "____r", "___h_", "___hr", /*  00,01,02,03  */
//   "__s__", "__s_r", "__sh_", "__shr", /*  04,05,06,07  */
//   "_____", "____r", "___h_", "___hr", /*  08,09,0A,0B  */
//   "__s__", "__s_r", "__sh_", "__shr", /*  0C,0D,0E,0F  */
//   "_d___", "_d__r", "_d_h_", "_d_hr", /*  10,11,12,13  */
//   "_ds__", "_ds_r", "_dsh_", "_dshr", /*  14,15,16,17  */
//   "_d___", "_d__r", "_d_h_", "_d_hr", /*  18,19,1A,1B  */
//   "__s__", "_ds_r", "_dsh_", "_dshr", /*  1C,1D,1E,1F  */
//   "a____", "a___r", "a__h_", "a__hr", /*  20,21,22,23  */
//   "a_s__", "a_s_r", "a_sh_", "a_shr", /*  24,25,26,27  */
//   "a____", "a___r", "a__h_", "a__hr", /*  28,29,2A,2B  */
//   "a_s__", "a_s_r", "a_sh_", "a_shr", /*  2C,2D,2E,2F  */
//   "ad___", "ad__r", "ad_h_", "ad_hr", /*  30,31,32,33  */
//   "ads__", "ads_r", "adsh_", "adshr", /*  34,35,36,37  */
//   "ad___", "ad__r", "ad_h_", "ad_hr", /*  38,39,3A,3B  */
//   "ads__", "ads_r", "adsh_", "adshr"  /*  3C,3D,3E,3F  */
// };
static uchar attrclr;

static char const monthstr[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//*********************************************************
//  this handles full-screen in NON-COLOR mode.
//*********************************************************
static void testpause (void)
{
   if (!n.pause)
      return;
   if (is_redirected ())
     return;

   if (++linecnt >= lines - 1) {
      nputs (n.colornhead, "Press any key to continue (or ESC to exit)");
      unsigned inkey = get_scode ();
      if (inkey == 27) {
         // if ((curlines != lines) && (!(n.ega_keep))) {
         //    set_lines (25);
         // }
         error_exit (DATA_OKAY, NULL);
      }

      if (n.color) {
         dreturn ();            // CR only!! 
         dclreol ();
      }
      else {
         printf ("\n");
      }
      linecnt = 1;
   }
}

/*****************************************************************/
static void ngotoxy (int x, int y)
{
   if (n.color) {
      dgotoxy (x, y);
   }
}

//**************************************************
void info (char *data[])
{
   unsigned j = 0;

   while (data[j] != NULL) {
      nputs (n.colordefalt, data[j++]);
      ncrlf ();
   }
}

//**************************************************
//  linecnt = number of lines written (non-color).
//**************************************************
void display_logo (void)
{
   if (n.batch) {
      return;
   }

   if (n.clear && !is_redirected ()) {
      dclrscr ();
   }

   linecnt = 0;

   if (n.minimize) {
      ngotoxy (0, _where_y ());
      nputs (n.colorlogo, ShortVersion);
   }
   else {
      ngotoxy ((80 - strlen (Version)) / 2, _where_y ());
      nputs (n.colorlogo, Version);
      ncrlf ();
   }
   // nputs(n.colordefalt, ShortVersion) ;
   set_text_attr (n.colordefalt);
}

//************************************************************************
//  lookup tables for special-extension display functions
//************************************************************************
typedef struct mm_lookup_s {
   char  ext[MAX_EXT_SIZE] ;
   int (*func)(char *fname, char *mlstr) ;
} mm_lookup_t ;

static mm_lookup_t const mm_lookup[] = {
{ "jpg", get_jpeg_info },
{ "gif", get_gif_info },
{ "wav", get_wave_info },
{ "mp3", get_mp3_info },
{ "bmp", get_bmp_info },
{ "avi", get_avi_info },
{ "png", get_png_info },
{ "ico", get_ico_info },
{ "cur", get_cur_info },
{ "sid", get_sid_info },
{ "webp", get_webp_info },
{ "", 0 }} ;

//************************************************************************
//  return final filename from symlink
//  sadly, this will only work with a 64-bit build
//************************************************************************
//  found in kernel32.dll
#ifdef USE_64BIT

#define  MAX_FILE_LENGTH   1024
char *GetLinkTarget(char const * const symlink_name) 
{
   static char final_file[MAX_FILE_LENGTH+1] = "";
   // Define smart pointer type for automatic HANDLE cleanup.
   // typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,
   //                         decltype( &::CloseHandle )> FileHandle;
   // Open file for querying only (no read/write access).
   HANDLE hdl = CreateFile( symlink_name, 0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                 // &::CloseHandle );
   if ( hdl == INVALID_HANDLE_VALUE ) {
      // h.release();
      // throw std::runtime_error( "CreateFileW() failed." );
      sprintf(final_file, "cannot create file");
   }
   else {

      const size_t requiredSize = GetFinalPathNameByHandle( hdl, NULL, 0,
                                                             FILE_NAME_NORMALIZED );
      if ( requiredSize == 0 ) {
         sprintf(final_file, "dest file size is 0");
      }
      else {
      // std::vector<wchar_t> buffer( requiredSize );
      GetFinalPathNameByHandle( hdl, final_file,
                                MAX_FILE_LENGTH,
                                FILE_NAME_NORMALIZED );
      }

   }
   return final_file;
}
#endif

//************************************************************************
extern unsigned multimedia_listing ;

void print1 (ffdata * fptr)
{
   char mlstr[30] ;
   int show_normal_info ;
   uchar SHRattr = fptr->attrib & 7;
   FILETIME lft;
   FileTimeToLocalFileTime (&(fptr->ft), &lft);
   // parse_time outdt;
   // // outdt.dtime[0] = fptr->ftime ;
   // // outdt.dtime[1] = fptr->fdate ;
   // FileTimeToDosDateTime (&lft, &(outdt.dtime[1]), &(outdt.dtime[0]));
   // int secs  = outdt.outdata.ft_tsec * 2;
   // int mins  = outdt.outdata.ft_min;
   // int hour  = outdt.outdata.ft_hour;
   // int day   = outdt.outdata.ft_day;
   // int month = outdt.outdata.ft_month;
   // long year = 1980L + (long) outdt.outdata.ft_year;
   SYSTEMTIME sdt ;
   FileTimeToSystemTime(&lft, &sdt) ;
   int secs  = sdt.wSecond ;
   int mins  = sdt.wMinute ;
   int hour  = sdt.wHour   ;
   int day   = sdt.wDay    ;
   int month = sdt.wMonth  ;
   long year = sdt.wYear   ;

   char attr[12];
   //  detect non-standard file attributes and display specially.
   // if ((fptr->attrib) > 0x3F) {
   //   sprintf (attr, " 0x%2X ", fptr->attrib);
   //   attrclr = 0x0C;
   // }
   // else {
   //   sprintf (attr, "%5s ", attrstr[fptr->attrib]);
   //   attrclr = n.colorattr;
   // }
   // char attrstr[] = "-adshr";
   if (n.long_attr) {
      sprintf(attr, "%08X ", fptr->attrib) ;
   } else {
      attr[0] = (fptr->attrib & FILE_ATTRIBUTE_ARCHIVE)        ? 'a' : '_' ;
      attr[1] = (fptr->attrib & FILE_ATTRIBUTE_DIRECTORY)      ? 'd' : '_' ;
      attr[2] = (fptr->attrib & FILE_ATTRIBUTE_SYSTEM)         ? 's' : '_' ;
      attr[3] = (fptr->attrib & FILE_ATTRIBUTE_HIDDEN)         ? 'h' : '_' ;
      attr[4] = (fptr->attrib & FILE_ATTRIBUTE_READONLY)       ? 'r' : '_' ;
      attr[5] = (fptr->attrib & FILE_ATTRIBUTE_REPARSE_POINT)  ? 'l' : '_' ;
      attr[6] = ' ';
      attr[7] = '\0';
   }
   attrclr = n.colorattr;

   mlstr[0] = 0 ;
   show_normal_info = 1 ;
   //  display directory entry
   if (fptr->dirflag) {
      sprintf (tempstr, "%14s ", "");
      nputs (n.colorsize, tempstr);
      nputs (attrclr, attr);
      if (lfn_supported) {
         //sprintf(tempstr, "%02d-%02d-%04lu ", month, day, year);
         sprintf (tempstr, "%3s %02d, %04lu ", monthstr[month - 1], day, year);
         nputs (n.colordate, tempstr);
         sprintf (tempstr, "%02d:%02d:%02d ", hour, mins, secs);
         nputs (n.colortime, tempstr);
         sprintf (tempstr, "[%s]", fptr->filename);
      }
      else {
         sprintf (tempstr, "%3s %02d, %04lu ", monthstr[month - 1], day,
            year);
         nputs (n.colordate, tempstr);
         sprintf (tempstr, "%02d:%02d:%02d ", hour, mins, secs);
         nputs (n.colortime, tempstr);
         sprintf (tempstr, "[%-8s%-4s]", fptr->name, fptr->ext);
         if (strlen (fptr->name) == 0) {
            sprintf (tempstr, "[NoShortName ]");
         }
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }

   //  display file entry
   else {
      char *p ;
      unsigned idx ;

      if (multimedia_listing) {
         p = strrchr(fptr->filename, '.') ;
         if (p != 0  &&  strlen(p) <= MAX_EXT_SIZE) {
            p++ ; //  skip past the period

            for (idx=0; mm_lookup[idx].ext[0] != 0; idx++) {
               if (strnicmp(p, mm_lookup[idx].ext, sizeof(mm_lookup[idx].ext)) == 0) {
                  //  call the special string generator function
                  (*mm_lookup[idx].func)(fptr->filename, mlstr) ; //lint !e522
                  show_normal_info = 0 ;
                  break;
               }
            }
         }
      }

      //  show file size
      switch (n.size_display) {
      case 2:
         // fsize.convert(fptr->fsize / 1000000) ;
         sprintf (tempstr, "%13s", convert_to_commas(fptr->fsize / 1000000, NULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M ");
         break;
      case 1:
         // fsize.convert(fptr->fsize / 1000) ;
         sprintf (tempstr, "%13s", convert_to_commas(fptr->fsize / 1000, NULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K ");
         break;
      default:
         // fsize.convert(fptr->fsize) ;
         sprintf (tempstr, "%14s ", convert_to_commas(fptr->fsize, NULL));
         nputs (n.colorsize, tempstr);
         break;
      }

      //  process multimedia display
      if (!show_normal_info) {
         nputs (n.colorattr, mlstr);
      } 
      //  display normal file listing
      else {
         nputs (attrclr, attr);
         sprintf (tempstr, "%3s %02d, %04lu ", monthstr[month - 1], day, year);
         nputs (n.colordate, tempstr);
         sprintf (tempstr, "%02d:%02d:%02d ", hour, mins, secs);
         nputs (n.colortime, tempstr);
      }

      //  format filename as required
      if (lfn_supported)
         sprintf (tempstr, "%s ", fptr->filename);
      else {
         sprintf (tempstr, "%-8s%-4s ", fptr->name, fptr->ext);
         if (strlen (fptr->name) == 0) {
            sprintf (tempstr, "NoShortName  ");
         }
      }

      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);
         
#ifdef USE_64BIT
      //  if this file is a symlink, try to display the actual file
      if ((fptr->attrib & FILE_ATTRIBUTE_REPARSE_POINT)) {
         ncrlf() ;
         nputs (n.colorsize, "               ");
         nputs (attrclr, "=====> ");
         char *lptr = GetLinkTarget(fptr->filename) ;
         sprintf (tempstr, "%s ", lptr);
         nputs (fptr->color, tempstr);
      }
#endif      
   }
}

//*********************************************************
void print2 (ffdata * fptr)
{
   uchar SHRattr = fptr->attrib & 7;
   FILETIME lft;
   FileTimeToLocalFileTime (&(fptr->ft), &lft);
   // parse_time outdt;
   // // outdt.dtime[0] = fptr->ftime ;
   // // outdt.dtime[1] = fptr->fdate ;
   // FileTimeToDosDateTime (&lft, &(outdt.dtime[1]), &(outdt.dtime[0]));
   // int mins = outdt.outdata.ft_min;
   // int hour = outdt.outdata.ft_hour;
   // int day = outdt.outdata.ft_day;
   // int month = outdt.outdata.ft_month;
   // long year = 1980L + (long) outdt.outdata.ft_year;
   SYSTEMTIME sdt ;
   FileTimeToSystemTime(&lft, &sdt) ;
   // int secs  = sdt.wSecond ;
   int mins  = sdt.wMinute ;
   int hour  = sdt.wHour   ;
   int day   = sdt.wDay    ;
   int month = sdt.wMonth  ;
   long year = sdt.wYear   ;

   if (fptr->dirflag)
      {
      sprintf(tempstr, "%-8s%-4s    [DIR]  ", fptr->name, fptr->ext) ;
      if (strlen(fptr->name) == 0) {
         sprintf(tempstr, "NoShortName     [DIR]  ") ;
      }
      //  display filename in appropriate color...
      if (SHRattr != 0  &&  n.showSHRfiles ) 
         nputs(n.colorSHR | SHRattr, tempstr) ;
      else 
         nputs(n.colordir, tempstr) ;
      }
   else
      {
      sprintf(tempstr, "%-8s%-4s  ", fptr->name, fptr->ext) ;
      if (strlen(fptr->name) == 0) {
         sprintf(tempstr, "NoShortName   ") ;
      }
      //  display filename in appropriate color...
      if (SHRattr != 0  &&  n.showSHRfiles ) 
         nputs(n.colorSHR | SHRattr, tempstr) ;
      else 
         nputs(fptr->color, tempstr) ;

      if (fptr->fsize > 99999999L)  
         {
         sprintf(tempstr, "%6u", (uint) (fptr->fsize/1000LLU)) ;  //lint !e626
         nputs(n.colorsize, tempstr) ;
         nputs(n.colorsize ^ 0x08, "K  ") ;
         }
      else  
         {
         sprintf(tempstr, "%7u  ", (uint) (fptr->fsize)) ;  //lint !e626
         nputs(n.colorsize, tempstr) ;
         }
      }

   sprintf(tempstr, "%02d-%3s-%02d ", day, monthstr[month-1], (int) (year % 100));
   nputs(n.colordate, tempstr) ;
   sprintf(tempstr, "%02d:%02d", hour, mins);
   nputs(n.colortime, tempstr) ;
}

//*********************************************************
void print3 (ffdata * fptr)
{
   parse_time outdt;
   uchar SHRattr = fptr->attrib & 7;
   // outdt.dtime[0] = fptr->ftime ;
   // outdt.dtime[1] = fptr->fdate ;
   //  how to convert FILETIME to my format??
   outdt.dtime[0] = '?';
   outdt.dtime[1] = '?';
   int month = outdt.outdata.ft_month;
   long year = 1980L + (long) outdt.outdata.ft_year;
   ULONGLONG fsize = fptr->fsize;

   if (fptr->dirflag) {
      sprintf (tempstr, "%-8s%-4s  [DIR] ", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName   [DIR] ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      sprintf (tempstr, "%-8s%-4s ", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName  ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);

      if (fsize > 99999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M ");
      }
      else if (fsize > 999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K ");
      }
      else {
         sprintf (tempstr, "%6u ", (uint) (fsize));
         nputs (n.colorsize, tempstr);
      }
   }

   sprintf (tempstr, "%02d-%02d", month, (int) (year % 100));
   nputs (n.colordate, tempstr);

   //  put the name last
   if (fptr->dirflag) {
      sprintf (tempstr, "%-8s%-4s  [DIR] ", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName   [DIR] ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      sprintf (tempstr, "%-8s%-4s ", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName  ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);

      if (fsize > 99999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M ");
      }
      else if (fsize > 999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K ");
      }
      else {
         sprintf (tempstr, "%6u ", (uint) (fsize));
         nputs (n.colorsize, tempstr);
      }
   }

}

/*****************************************************************/
void print4 (ffdata * fptr)
{
   ULONGLONG fsize = fptr->fsize;
   uchar SHRattr = fptr->attrib & 7;

   if (fptr->dirflag) {
      sprintf (tempstr, "%-8s%-4s  [DIR]", fptr->name, fptr->ext);
      //  display filename in appropriate color...
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName   [DIR]");
      }
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      sprintf (tempstr, "%-8s%-4s ", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName  ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);

      if (fsize > 99999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M");
      }
      else if (fsize > 999999L) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K");
      }
      else {
         sprintf (tempstr, "%6u", (uint) (fsize));
         nputs (n.colorsize, tempstr);
      }
   }
}

/*****************************************************************/
void print6 (ffdata * fptr)
{
   uchar SHRattr = fptr->attrib & 7;
   if (fptr->dirflag) {
      sprintf (tempstr, "%-8s%-4s", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      sprintf (tempstr, "%-8s%-4s", fptr->name, fptr->ext);
      if (strlen (fptr->name) == 0) {
         sprintf (tempstr, "NoShortName ");
      }
      //  display filename in appropriate color...
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);
   }
}

//*********************************************************
//  from FILELIST.CPP
//  global vars for file size calculations
//*********************************************************
extern char tempfmtstr[10] ; //  for forming strings of the form %-nns

//*********************************************************
void lfn_print2 (ffdata * fptr)
{
   uchar SHRattr = fptr->attrib & 7;

   FILETIME lft;
   FileTimeToLocalFileTime (&(fptr->ft), &lft);
   // parse_time outdt;
   // FileTimeToDosDateTime (&lft, &(outdt.dtime[1]), &(outdt.dtime[0]));
   // int mins = outdt.outdata.ft_min;
   // int hour = outdt.outdata.ft_hour;
   // int day = outdt.outdata.ft_day;
   // int month = outdt.outdata.ft_month;
   // long year = 1980L + (long) outdt.outdata.ft_year;
   SYSTEMTIME sdt ;
   FileTimeToSystemTime(&lft, &sdt) ;
   // int secs  = sdt.wSecond ;
   int mins  = sdt.wMinute ;
   int hour  = sdt.wHour   ;
   int day   = sdt.wDay    ;
   int month = sdt.wMonth  ;
   long year = sdt.wYear   ;

   ULONGLONG fsize = fptr->fsize;

   if (fptr->dirflag) {
      nputs (n.colordir, " [DIR] ");
      sprintf (tempstr, "%02d-%3s-%02d ", day, monthstr[month - 1],
         (int) (year % 100));
      nputs (n.colordate, tempstr);
      sprintf (tempstr, "%02d:%02d ", hour, mins);
      nputs (n.colortime, tempstr);

      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      //  print file size
      if (fsize > 99999999L  ||  n.size_display == 2) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000000ULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M ");
      }
      else if (fsize > 999999L  ||  n.size_display == 1) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000ULL));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K ");
      }
      else {
         sprintf (tempstr, "%6u ", (uint) fsize);
         nputs (n.colorsize, tempstr);
      }

      sprintf (tempstr, "%02d-%3s-%02d ", day, monthstr[month - 1],
         (int) (year % 100));
      nputs (n.colordate, tempstr);
      sprintf (tempstr, "%02d:%02d ", hour, mins);
      nputs (n.colortime, tempstr);

      //  generate filename
      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);
   }
}

/*****************************************************************/
void lfn_print4 (ffdata * fptr)
{
   ULONGLONG fsize = fptr->fsize;
   uchar SHRattr = fptr->attrib & 7;

   if (fptr->dirflag) {
      nputs (n.colordir, " [DIR] ");
      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      //  print file size
      if (fsize > 99999999L  ||  n.size_display == 2) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "M ");
      }
      else if (fsize > 999999L  ||  n.size_display == 1) {
         sprintf (tempstr, "%5u", (uint) (fsize / 1000L));
         nputs (n.colorsize, tempstr);
         nputs (n.colorsize ^ 0x08, "K ");
      }
      else {
         sprintf (tempstr, "%6u ", (uint) fsize);
         nputs (n.colorsize, tempstr);
      }

      //  generate filename
      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);
   }
}

/*****************************************************************/
void lfn_print6 (ffdata * fptr)
{
   uchar SHRattr = fptr->attrib & 7;
   if (fptr->dirflag) {
      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (n.colordir, tempstr);
   }
   else {
      sprintf (tempstr, tempfmtstr, fptr->filename);
      if (SHRattr != 0 && n.showSHRfiles)
         nputs (n.colorSHR | SHRattr, tempstr);
      else
         nputs (fptr->color, tempstr);
   }
}

/******************************************************************/
void ncrlf (void)
{
   if (n.color) {
      dnewline ();
   }
   else {
      printf ("\n");
   }
   testpause ();
}

/******************************************************************/
void nputc (uchar attr, const uchar outchr)
{
   // unsigned hattr ;
   if (n.color) {
      set_text_attr (attr);
      dputc (outchr);
   }
   else
      printf ("%c", outchr);
}

/******************************************************************/
void nputs (uchar attr, const char *outstr)
{
   if (n.color) {
      set_text_attr (attr);
      dputs (outstr);
   }
   else
      printf ("%s", outstr);
}

/******************************************************************/
void nput_char (uchar attr, char chr, int count)
{
   if (n.color) {
      dputnchar (chr, attr, count);
   }
   else {
      for (int j = 0; j < count; j++)
         putchar (chr);
   }
}

/******************************************************************/
void nput_line (uchar attr, char chr)
{
   int j, wincols = get_window_cols() - 1 ;
   if (n.color) {
      dputnchar (chr, attr, wincols);
   }
   else {
      for (j = 0; j < wincols; j++)
         putchar (chr);
   }
   ncrlf ();
}

//lint +e559   Size of argument no. 3 inconsistent with format
//lint +e626   argument inconsistent with format
