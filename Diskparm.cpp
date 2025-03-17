//*****************************************************************************
//  Copyright (c) 1995-2025  Daniel D Miller
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
#include <stdio.h>
#include <tchar.h>

#include "common.h"
#include "ndir32.h"

TCHAR dpath[4] = _T("d:\\");

static TCHAR fsnbfr[PATH_MAX];

//  mediatype.cpp
extern TCHAR *get_cd_device_desc(TCHAR drv);

// static char UNCname[PATH_MAX] ;
struct UNC
{
   TCHAR *uptr;
   TCHAR ustr[PATH_MAX];
};

static TCHAR fsn_bfr[32] ;      //  buffer for name of lfn file system

static TCHAR diskavail[MAX_ULL_COMMA_LEN+1]; 
static TCHAR disktotal[MAX_ULL_COMMA_LEN+1]; 

//*****************************************************************
const unsigned int DEFAULT_CLUSTER_SIZE = 4096 ;

static unsigned get_cluster_size(TCHAR dltr)
{
   TCHAR dirpath[4] = _T("c:\\") ;
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
   if (GetDiskFreeSpace(dirpath, &secperclus, &bytespersec, &numfreeclus, &numtotalclus) != 0) {
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
   if (!GetVolumeInformation( dirptr,                 // LPCSTR lpRootPathName,
                              (TCHAR *)volume_name,    // LPSTR lpVolumeNameBuffer,
                              PATH_MAX,               // DWORD nVolumeNameSize,
                              &vsernbr,               // LPDWORD lpVolumeSerialNumber,
                              &mclen,                 // LPDWORD lpMaximumComponentLength,
                              &fsflags,               // LPDWORD lpFileSystemFlags,
                              (TCHAR *)fsn_bfr,        // LPSTR lpFileSystemNameBuffer,
                              PATH_MAX                // DWORD nFileSystemNameSize
                            )) {
      // syslog("cannot read volume info from %s:\n", dirptr) ;
      volume_name[0] = 0 ; //  try to keep going...
      gvi_valid = false ;
   }

   if (_tcslen(volume_name) == 0) {
      _tcscpy(volume_name, _T("undefined")) ;
   }
   // dtype = GetDriveType(dpath) ;

   clbytes = get_cluster_size(dpath[0]) ;   //  assume dpath[] is valid
   //  that worked okay... now, get the disk summary
   // unsigned __int64 freec1, frees1, totals1 ;
   // LPCSTR dptr = dirptr ;
   if (!GetDiskFreeSpaceEx(dstr, 
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

static TCHAR *get_drive_type_string(UINT gdt, TCHAR dchar)
{
   // int cd_type ;
   switch (gdt) {
   case DRIVE_NO_ROOT_DIR: return _T("NoRootDir") ;
   //  is there any chance of getting info on this drive type??
   //  These are typically USB hubs...
   case DRIVE_REMOVABLE  : return _T("removable") ;
   case DRIVE_FIXED      : return _T("fixed") ;
   case DRIVE_REMOTE     : return _T("remote") ;
   case DRIVE_CDROM      : 
      return get_cd_device_desc(dchar) ;

   case DRIVE_RAMDISK    : return _T("ramdisk") ;
   case DRIVE_UNKNOWN    : 
   default:
      return _T("unknown") ;
   }
}

//**********************************************************
void display_drive_summary (void)
{
   unsigned dtype;
   unsigned long bufsize;
   //                                    012345678901234567
   static TCHAR const spaces18[20] = _T("                  ");

   //  draw header
   nput_line (n.colorframe, '*');
   nputs (n.colornhead, _T("                               Disk Drive Summary                              \n"));
   nput_line (n.colorframe, '=');

   if (n.drive_summary == DSUMMARY_FREE) {
      nputs (n.colornhead, _T("   file sys      total space          free space     [Cluster Size] UNC path \n"));
   } else {
      nputs (n.colornhead, _T("   file sys      total space          used space     [Cluster Size] UNC path \n"));
   }
   nputs (n.colornhead, _T("   ========  ==================  ==================  ========================\n"));

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
      dtype = GetDriveType(dpath);
      if (!get_disk_info(dpath)) {
         TCHAR *tp = get_drive_type_string(dtype, dchar);
         _stprintf(tempstr, _T("%c: %-9s %18s  %18s           no media present\n"), 
            dchar, tp, spaces18, spaces18) ;
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

         _stprintf(tempstr, _T("%c: %-9s %18s  %18s  "), dchar, fsnbfr, disktotal, diskavail);
// syslog(_T("%c: %-9s %18s  %18s  "), dchar, fsnbfr, disktotal, diskavail);
         nputs (n.colordir, tempstr);

         // if (WNetGetUniversalName(dpath, REMOTE_NAME_INFO_LEVEL, 
         if (WNetGetUniversalName(dpath, UNIVERSAL_NAME_INFO_LEVEL, &UNCpaths, &bufsize) != NO_ERROR) {
            nputs (n.colordir, _T("Network Drive\n"));
            // return 1;
         }
         else {
            _stprintf(tempstr, _T("%s\n"), UNCpaths.uptr);
// syslog(_T("%s\n"), UNCpaths.uptr);
            nputs (n.colordir, tempstr);
         }
      }
      else {
      // else if (dtype == DRIVE_FIXED) {
         _stprintf(tempstr, _T("%c: %-9s %18s  %18s  [%6u] %s\n"), 
            dchar, fsn_bfr, disktotal, diskavail, (unsigned) clbytes, volume_name);
         nputs (n.colordefalt, tempstr);
// syslog(_T("%s\n"), tempstr) ;

         // unsigned cluster_size = get_cluster_size(dpath[0]);
         // get_disk_info(dpath) ;  //  why are you calling this again??
         // lfree  += frees1.QuadPart;
         // ltotal += totals1.QuadPart;
         lfree  += frees1 ;
         ltotal += totals1 ;
      }
   }

   //  display drive summary
   convert_to_commas(ltotal, disktotal);
   convert_to_commas(lfree, diskavail);

   nput_line (n.colorframe, '*');
   _stprintf(tempstr, _T("             %18s  %18s"), disktotal, diskavail);
// syslog(_T("             %18s  %18s"), disktotal, diskavail);
   nputs (n.colorxhead, tempstr);
   _stprintf(tempstr, _T("  Total Physical space\n\r"));
// syslog(_T("  Total Physical space\n\r"));
   nputs (n.colornhead, tempstr);
}
