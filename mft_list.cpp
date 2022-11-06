//====================================================================
//
// mft_list.cpp
// Derived from 'ntfsinfo.c',
// Copyright (C) 1997 Mark Russinovich, http://www.sysinternals.com
//
// Shows NTFS volume information.
// Note: This function requires linking mpr.lib
//
//  Later note: The code in this module is obsolete in NTFS world.
//  It is no longer included in the code base.
//====================================================================
#include <windows.h>
#include <ctype.h>

// extern int tolower(int c);

// const __int64 C1MByte = (1024 * 1024) ;

//
// File System Control command for getting NTFS information
//
#define FSCTL_GET_VOLUME_INFORMATION   0x90064

//
// return code type
//
#ifndef USE_64BIT
typedef UINT NTSTATUS;
#endif

//
// Error codes returned by NtFsControlFile (see NTSTATUS.H)
//
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#ifndef USE_64BIT
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#endif
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_ALREADY_COMMITTED         ((NTSTATUS)0xC0000021L)
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L)

//
// Io Status block (see NTDDK.H)
//
typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


//
// Apc Routine (see NTDDK.H)
//
typedef VOID (*PIO_APC_ROUTINE) (
            PVOID ApcContext,
            PIO_STATUS_BLOCK IoStatusBlock,
            ULONG Reserved
         );

//
// The undocumented NtFsControlFile
//
// This function is used to send File System Control (FSCTL)
// commands into file system drivers. Its definition is 
// in ntdll.dll (ntdll.lib), a file shipped with the NTDDK.
//
static NTSTATUS (__stdcall *NtFsControlFile)( 
               HANDLE FileHandle,
               HANDLE Event,              // optional
               PIO_APC_ROUTINE ApcRoutine,      // optional
               PVOID ApcContext,          // optional
               PIO_STATUS_BLOCK IoStatusBlock,  
               ULONG FsControlCode,
               PVOID InputBuffer,            // optional
               ULONG InputBufferLength,
               PVOID OutputBuffer,           // optional
               ULONG OutputBufferLength
         ); 


//
// NTFS volume information
//
#ifndef USE_64BIT
typedef struct {
   LARGE_INTEGER     SerialNumber;
   LARGE_INTEGER     NumberOfSectors;
   LARGE_INTEGER     TotalClusters;
   LARGE_INTEGER     FreeClusters;
   LARGE_INTEGER     Reserved;
   ULONG             BytesPerSector;
   ULONG             BytesPerCluster;
   ULONG             BytesPerMFTRecord;
   ULONG             ClustersPerMFTRecord;
   LARGE_INTEGER     MftValidDataLength;
   LARGE_INTEGER     MFTStart;
   LARGE_INTEGER     MFTMirrorStart;
   LARGE_INTEGER     MftZoneStart;
   LARGE_INTEGER     MftZoneEnd;
} NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;
#endif

//--------------------------------------------------------------------
//                      F U N C T I O N S
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// GetNTFSInfo - Open the volume and query its data.
//--------------------------------------------------------------------
static BOOLEAN GetNTFSInfo( int DriveId, PNTFS_VOLUME_DATA_BUFFER VolumeInfo ) 
{
   static char       volumeName[] = "\\\\.\\A:";
   HANDLE            volumeHandle;
   IO_STATUS_BLOCK      ioStatus;
   NTSTATUS       status;

   //
   // open the volume
   //
   volumeName[4] = DriveId + 'A'; 
   volumeHandle = CreateFile( volumeName, GENERIC_READ, 
               FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
               0, 0 );
   if( volumeHandle == INVALID_HANDLE_VALUE )   {

      // printf("\nError opening volume: ");
      // PrintWin32Error( GetLastError() );
      return FALSE;
   }

   // Query the volume information
   status = NtFsControlFile( volumeHandle, NULL, NULL, 0, &ioStatus,
                  FSCTL_GET_VOLUME_INFORMATION,
                  NULL, 0,
                  VolumeInfo, sizeof( NTFS_VOLUME_DATA_BUFFER ) );

   if( status != STATUS_SUCCESS ) {
      // printf("\nError obtaining NTFS information: ");
      // PrintNtError( status );
      CloseHandle( volumeHandle );
      return FALSE;
   }

   // Close the volume
   CloseHandle( volumeHandle );
   return TRUE;
}

//--------------------------------------------------------------------
// LocateNDLLCalls - Loads function entry points in NTDLL
//--------------------------------------------------------------------
static BOOLEAN LocateNTDLLCalls()
{
   //lint -e611 -e720 
   if( !(NtFsControlFile = (
            NTSTATUS (__stdcall *)( 
               HANDLE FileHandle,
               HANDLE Event,              // optional
               PIO_APC_ROUTINE ApcRoutine,      // optional
               PVOID ApcContext,          // optional
               PIO_STATUS_BLOCK IoStatusBlock,  
               ULONG FsControlCode,
               PVOID InputBuffer,            // optional
               ULONG InputBufferLength,
               PVOID OutputBuffer,           // optional
               ULONG OutputBufferLength
         ) 
   ) GetProcAddress( GetModuleHandle("ntdll.dll"), "NtFsControlFile" )) ) {   //lint !e820
   // mft_list.cpp  160  Info 820: Boolean test of a parenthesized assignment
      return FALSE;
   }
   return TRUE;
   //lint +e611 +e720 
}

//--------------------------------------------------------------------
// main - Just open the volume and dump its information.
//--------------------------------------------------------------------
// unsigned get_nt_cluster_size(char dltr)
// {
//    NTFS_VOLUME_DATA_BUFFER  volumeInfo;
// 
//    if (!LocateNTDLLCalls() ) {
//       return 4096 ;  //  return 4K on fail
//   }
//    dltr = (int) tolower(dltr) - 'a' ;
// 
//    if (!GetNTFSInfo(dltr, &volumeInfo ))
//       return 4096 ;  //  return 4K on fail
//        
//    return volumeInfo.BytesPerCluster ;
// }

//--------------------------------------------------------------------
// open the volume and dump its information.
//  Well, one of the primary purposes of this function was to
//  determine how much of the total disk free space was actually
//  mft unused space.  Unfortunately, that information doesn't 
//  appear to actually be available on Win XP.
//  
//  Later note:  Also, on NTFS file system, MFT is not even defined!
//  
//  So, the upshot of all this, is that this function always returns 0,
//  therefore the entire file is not needed...
//--------------------------------------------------------------------
ULONGLONG get_nt_free_space(char dltr)
{
   NTFS_VOLUME_DATA_BUFFER  volumeInfo;
   ULONGLONG fb, mb, mub, mfb ;

   if (!LocateNTDLLCalls() ) {
      return 0 ;  //  return 0 on fail
   }
   dltr = (int) tolower(dltr) - 'a' ;

   if (!GetNTFSInfo(dltr, &volumeInfo ))
      return 0 ;  //  return 0 on fail

   // puts("drv  volume     bytes/   MFT size     bytes/   free space  MFT free  ") ;
   // puts("ltr  size (MB)  cluster  MBytes (%)   MFT rec  MBytes (%)  MBytes (%)") ;
   // puts("===  =========  =======  ==========   =======  ==========  ==========") ;

   fb = volumeInfo.FreeClusters.QuadPart * volumeInfo.BytesPerCluster ; // free bytes

   mb = volumeInfo.MftValidDataLength.QuadPart * volumeInfo.BytesPerCluster ; //  mft bytes

   mub = (volumeInfo.MftZoneEnd.QuadPart - volumeInfo.MftZoneStart.QuadPart) * volumeInfo.BytesPerCluster ;
   if (mub > mb)
      return 0;

   mfb = mb - mub ;

   //    printf("%c:   %9I64d    %5d   %4I64d (%2I64d%%)    %5d   %4I64d (%2I64d%%)  %4I64d (%2I64d%%)\n", 
   //       ('a' + dltr),     vs,     bc,    mb,     mp,          bm,  fb,     fp,        mfb,    mfp) ;
//lint -esym(550, fb, mfb)
   // return fb - mfb ;
   return 0;
}

