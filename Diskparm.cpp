//*****************************************************************************
//  Copyright (c) 1995-2023  Daniel D Miller
//  DISKPARM.CPP - Display partition information about disk drive.     
//                                                                     
//  Written by:   Daniel D. Miller                                     
//*****************************************************************************
//  Note on DISKPARM build:  
//  The function WNetGetUniversalName() brings several problems with it.  
//  First, it reportedly is not valid on Win95 drives.  
//  Second, it requires MPR.LIB to be linked into the application
//*****************************************************************************

// #include <stdlib.h>             //  ultoa()
#include <windows.h>

#include "common.h"
#include "ndir32.h"

char dpath[4] = "d:\\";

static char fsnbfr[PATH_MAX];

//  mediatype.cpp
extern char *get_cd_device_desc(char drv);

// static char UNCname[PATH_MAX] ;
struct UNC
{
   char *uptr;
   char ustr[PATH_MAX];
};

static char fsn_bfr[32] ;      //  buffer for name of lfn file system

static char diskavail[MAX_ULL_COMMA_LEN+1]; 
static char disktotal[MAX_ULL_COMMA_LEN+1]; 

//*****************************************************************
const unsigned int DEFAULT_CLUSTER_SIZE = 4096 ;

static unsigned get_cluster_size(TCHAR dltr)
{
   TCHAR dirpath[4] = "c:\\" ;
   unsigned cl_bytes = DEFAULT_CLUSTER_SIZE ;

   //  12/17/13 
   //  Experimenting with older function to get sector/cluster data...
   //  Okay, this gives good data even on large volumes...

   //  EARLY NOTES: this still needs to be developed.
   //  If NTFS,  use lookup table.
   //  If FAT16, use GetDiskFreeSpace()
   //  If FAT32, try to figure some way to get to INT functions
   //  
   //  12/17/13 Actually, *all* of these formats can use GetDiskFreeSpace()
   //  for bytes/sector and sectors/cluster.  
   //  Only the total-size fields are conditional, 
   //  because GetDiskFreeSpace() is u32, not u64.
// c:\:  8,  512, 112671991, 152672853
// d:\:  1, 2048, 0, 191731
// g:\: 32,  512, 922615, 945313
// l:\:  8,  512, 204827101, 488279551
// n:\:  8,  512, 204827101, 488279551
// p:\:  8,  512, 204827101, 488279551
// t:\:  8,  512, 204827101, 488279551
// u:\:  8,  512, 204827101, 488279551
// v:\:  8,  512, 204827101, 488279551
   dirpath[0] = dltr ;
   DWORD secperclus, bytespersec, numfreeclus, numtotalclus ;
   if (GetDiskFreeSpaceA(dirpath, &secperclus, &bytespersec, &numfreeclus, &numtotalclus) != 0) {
      // sprintf(msgstr, "%s: %u, %u, %u, %u\n", 
      //    dpath, (uint) secperclus, (uint) bytespersec, (uint) numfreeclus, (uint) numtotalclus) ;
      // OutputDebugString(msgstr) ; 
      cl_bytes = secperclus * bytespersec ;
   } 
   return cl_bytes ;
}   

//**************************************************************************
//  this function determines the following information:
//    - What file system is in use??
//    - Total partition size in bytes
//    - Free partition space in bytes
//    - Cluster size for active partition
//    - Volume label for requested partition
//  
//  NOTE: This function originally returned a boolean value which was
//        defined as: is_FAT32_present
//        I no longer recall what the purpose of this information was,
//        but it is no longer referenced now.
//**************************************************************************
static u64 freec1 = 0, frees1 = 0, totals1 = 0 ;
bool get_disk_info(TCHAR *dstr)
{
   DWORD vsernbr, mclen, fsflags ;
   // UINT dtype ;
   TCHAR *dirptr ;
   bool gvi_valid = true ;

   dpath[0] = *dstr ;
   if (*(dstr+1) == ':')
      dirptr = dpath ;
   else
      dirptr = dstr ;

   // sprintf(tempstr, "trying %s\n", dirptr) ;
   // nputs(n.colordefalt, tempstr) ;
   // getchar() ;
   //  NOTE:  GetVolumeInformation() requires a filename ending
   //    with a backslash.  No wildcards are supported.
   if (!GetVolumeInformationA( dirptr,                 // LPCSTR lpRootPathName,
                              (LPSTR) volume_name,    // LPSTR lpVolumeNameBuffer,
                              PATH_MAX,               // DWORD nVolumeNameSize,
                              &vsernbr,               // LPDWORD lpVolumeSerialNumber,
                              &mclen,                 // LPDWORD lpMaximumComponentLength,
                              &fsflags,               // LPDWORD lpFileSystemFlags,
                              (LPSTR) fsn_bfr,        // LPSTR lpFileSystemNameBuffer,
                              PATH_MAX                // DWORD nFileSystemNameSize
                            )) {
      // syslog("cannot read volume info from %s:\n", dirptr) ;
      volume_name[0] = 0 ; //  try to keep going...
      gvi_valid = false ;
   }

#ifdef UNICODE
   if (_tcslen(volume_name) == 0) {
      _tcscpy(volume_name, (const wchar_t*) "undefined") ;
   }
#else
   if (_tcslen(volume_name) == 0) {
      _tcscpy(volume_name, "undefined") ;
   }
#endif

   // dtype = GetDriveType(dpath) ;

   clbytes = get_cluster_size(dpath[0]) ;   //  assume dpath[] is valid
   //  that worked okay... now, get the disk summary
   // unsigned __int64 freec1, frees1, totals1 ;
   // LPCSTR dptr = dirptr ;
   if (!GetDiskFreeSpaceExA(dstr, 
      (ULARGE_INTEGER *) &freec1, 
      (ULARGE_INTEGER *) &totals1, 
      (ULARGE_INTEGER *) &frees1)) {
      // syslog("can't get free disk space [%s]\n", dstr) ;
      
      //  emergency handling when this function fails
      freec1 = 0 ;
      totals1 = 0 ;
      frees1 = 0 ;
      diskbytes = 0 ;
      diskfree = 0 ;
   } 
   //  if GetDiskFreeSpaceEx succeeds, proceed normally
   else {
      diskbytes = totals1 ;
      //  first try to get free disk space from NT info,
      //  otherwise use the data from GetDiskFreeSpaceEx().
      //  On WinNT systems, however, the latter will include
      //  so-called "free space" in the MFT (Master File Table),
      //  which is *never* available for normal use.
      //  
      //  Later note: however, accessing the MFT space via the ntfsinfo technique,
      //  originally presented in source code by sysinternals.com,
      //  requires Admin authorization to access.
      //  Since MFT space is inconsequential (about 1GB) on modern drives,
      //  it isn't worth requiring Admin clearance.
      // diskfree = get_nt_free_space(dpath[0]) ;
      // if (diskfree == 0) {
         diskfree = frees1 ;
      // }
   }
   return gvi_valid;
}

//**********************************************************
// static char *cd_names[4] = {
// "CD_ROM", "CD_RW", "DVD_ROM", "DVD_RW"   
// } ;

static char *get_drive_type_string(UINT gdt, char dchar)
{
   // int cd_type ;
   switch (gdt) {
   case DRIVE_NO_ROOT_DIR: return "NoRootDir" ;
   //  is there any chance of getting info on this drive type??
   //  These are typically USB hubs...
   case DRIVE_REMOVABLE  : return "removable" ;
   case DRIVE_FIXED      : return "fixed" ;
   case DRIVE_REMOTE     : return "remote" ;
   case DRIVE_CDROM      : 
      return get_cd_device_desc(dchar) ;

   case DRIVE_RAMDISK    : return "ramdisk" ;
   case DRIVE_UNKNOWN    : 
   default:
      return "unknown" ;
   }
}

//**********************************************************
void display_drive_summary (void)
{
   unsigned dtype;
   unsigned long bufsize;

   //  draw header
   nput_line (n.colorframe, '*');
   nputs (n.colornhead,
      "                               Disk Drive Summary                              \n\r");
   nput_line (n.colorframe, '=');

   if (n.drive_summary == DSUMMARY_FREE) {
      nputs (n.colornhead, "   file sys      total space          free space     [Cluster Size] UNC path \n");
   } else {
      nputs (n.colornhead, "   file sys      total space          used space     [Cluster Size] UNC path \n");
   }
   nputs (n.colornhead, "   ========  ==================  ==================  ========================\n");

   ULONGLONG lfree = 0;
   ULONGLONG ltotal = 0;

   DWORD gld_return = GetLogicalDrives() ;
   DWORD mask ;
   char dchar = 'a' ;
   for (mask=1; mask != 0; mask <<= 1, dchar++) {
      if ((gld_return & mask) == 0)
         continue;

      // dpath[0] = (char) dltr + 'a';
      dpath[0] = dchar ;
      dtype = GetDriveTypeA (dpath);
      if (!get_disk_info(dpath)) {
         wsprintfA(tempstr, "%c: %-9s %18s  %18s           no media present\n", 
            dchar, get_drive_type_string(dtype, dchar), " ", " ") ;
         nputs (n.colordefalt, tempstr);
         continue;
      }

      //  convert free space to used space 
      if (n.drive_summary == DSUMMARY_USED) {
         frees1 = totals1 - frees1 ;
      }
      convert_to_commas(totals1, disktotal);
      convert_to_commas(frees1, diskavail);

      //  if network, pull the UNC name, otherwise show drive type
      if (dtype == DRIVE_REMOTE) {
         UNC UNCpaths;
         UNCpaths.uptr = UNCpaths.ustr;
         bufsize = PATH_MAX;

         wsprintfA(tempstr, "%c: %-9s %18s  %18s  ", dchar, fsnbfr, disktotal, diskavail);
         nputs (n.colordir, tempstr);

         // if (WNetGetUniversalName(dpath, REMOTE_NAME_INFO_LEVEL, 
         if (WNetGetUniversalNameA(dpath, UNIVERSAL_NAME_INFO_LEVEL, &UNCpaths, &bufsize) != NO_ERROR) {
            nputs (n.colordir, "Network Drive\n");
            // return 1;
         }
         else {
            wsprintfA(tempstr, "%s\n", UNCpaths.uptr);
            nputs (n.colordir, tempstr);
         }
      }
      else {
      // else if (dtype == DRIVE_FIXED) {
         wsprintfA(tempstr, "%c: %-9s %18s  %18s  ", dchar, fsn_bfr, disktotal, diskavail);
         nputs (n.colordefalt, tempstr);

         // unsigned cluster_size = get_cluster_size(dpath[0]);
         // get_disk_info(dpath) ;  //  why are you calling this again??
         // lfree  += frees1.QuadPart;
         // ltotal += totals1.QuadPart;
         lfree  += frees1 ;
         ltotal += totals1 ;
         wsprintfA(tempstr, "[%6u] %s\n", (unsigned) clbytes, volume_name);
         nputs (n.colordefalt, tempstr);
      }
   }

   //  display drive summary
   convert_to_commas(ltotal, disktotal);
   convert_to_commas(lfree, diskavail);

   nput_line (n.colorframe, '*');
   wsprintfA(tempstr, "             %18s  %18s", disktotal, diskavail);
   nputs (n.colorxhead, tempstr);
   wsprintfA(tempstr, "  Total Physical space\n\r");
   nputs (n.colornhead, tempstr);
}
