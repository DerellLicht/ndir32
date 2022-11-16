//************************************************************************
//  This file uses the legacy SCSI interface to determine internal
//  drive details of optical drives.
//************************************************************************

#include <stddef.h>     //  offsetof
#include <windows.h>
#include <stdio.h>
#include <initguid.h>   // Guid definition
// #include <devguid.h>    // Device guids
// #include <setupapi.h>   // for SetupDiXxx functions.
#ifdef USE_64BIT
#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <ntdddisk.h>
#include <ntddscsi.h>
#else
#include <ddk/cfgmgr32.h>   // for SetupDiXxx functions.
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>
#endif

#include "scsi_defs.h"

typedef unsigned int  uint ;

//lint -e40  Undeclared identifier, which it obviously is not
//lint -e78  Symbol typedef'ed at line 113, file scsi_defs.h used in expression (so what??)

//*************************************************************************
typedef struct ProfileInfo_s {
   char *description ;
   uint profile_code ;
} ProfileInfo_t ;

ProfileInfo_t const ProfileInfo[] = {
{ "CDROM",     0x08 },  // CD-ROM Read only Compact Disc capable
{ "CDR",       0x09 },  // CD-R Write once Compact Disc capable
{ "CDRW",      0x0A },  // CD-RW Re-writable Compact Disc capable
{ "DVDROM",    0x10 },  // DVD-ROM Read only DVD
{ "DVDR",      0x11 },  // DVD-R Sequential Recording Write once DVD using Sequential recording
{ "DVDRAM",    0x12 },  // DVD-RAM Re-writable DVD
{ "DVD-RWOvr", 0x13 },  // DVD-RW Restricted Overwrite Re-recordable DVD using Restricted Overwrite
{ "DVD-RWSeq", 0x14 },  // Sequential recording Re-recordable DVD using Sequential recording
{ "DVD-R_DL",  0x15 },  // DVD-R Dual Layer Sequential Recording Dual Layer DVD-R using Sequential recording
{ "DVD-R_DL",  0x16 },  // DVD-R Dual Layer Jump Recording Dual Layer DVD-R using Layer Jump recording
{ "DVD+RW",    0x1A },  // DVD+RW DVD+ReWritable
{ "DVD+R",     0x1B },  // DVD+R DVD+Recordable
{ "DVD+RW_DL", 0x2A },  // Dual Layer DVD+Rewritable Dual Layer
{ "DVD+R_DL",  0x2B },  // DVD+R Dual Layer DVD+Recordable Dual Layer
{ "BluRayRom", 0x40 },  // BD-ROM Blu-ray Disc ROM
{ "BluRayBDR", 0x41 },  // BD-R SRM Blu-ray Disc Recordable – Sequential Recording
{ "BluRayRRM", 0x42 },  // Blu-ray Disc Recordable – Random Recording Mode
{ "BluRayRE",  0x43 },  // BD-RE Blu-ray Disc Rewritable
{ "HDDVDROM",  0x50 },  // HD DVD-ROM Read-only HD DVD
{ "HDDVDR",    0x51 },  // HD DVD-R Write-once HD DVD
{ "HDDVDRAM",  0x52 },  // HD DVD-RAM Rewritable HD DVD
{ "Unknown",   0 } } ;

//****************************************************************************
static unsigned get_cd_cfg(BOOL Status, PSCSI_PASS_THROUGH_WITH_BUFFERS Psptwb )
{
   if (!Status ) 
     return 0 ;

   if (Psptwb->Spt.ScsiStatus) 
     return 0 ;

   // printf("cfg buffer: 6=0x%02X, 7=0x%02X\n", Psptwb->DataBuf[6], Psptwb->DataBuf[7]) ;
   // unsigned profile = (Psptwb->DataBuf[6] << 4) | Psptwb->DataBuf[7] ;
   unsigned profile = Psptwb->DataBuf[13] ;
   return profile;
   // Dim currentProfile As Integer = ((scsiCommand.DataBuffer(6) << 4) Or scsiCommand.DataBuffer(7))
}

//****************************************************************************
//  Use SCSI command structure to read device-type code for optical drives
//****************************************************************************
static int GetDeviceConfig(HANDLE hCD)
{
   STORAGE_PROPERTY_QUERY query;
   SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
   HANDLE hDevice = hCD ;
   BOOL   status;
   char   outBuf[512];
   ULONG  length ;
   ULONG  returned = 0,
          returnedLength ;

   //  This first call with IOCTL_STORAGE_QUERY_PROPERTY
   //  will just tell us if this is a normal physical drive;
   //  if not, this call will fail
   query.PropertyId = StorageAdapterProperty;
   query.QueryType = PropertyStandardQuery;
   status = DeviceIoControl(hDevice,                
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &query,
                            sizeof( STORAGE_PROPERTY_QUERY ),
                            outBuf,                   
                            512,                      
                            &returnedLength,      
                            NULL                    
                            );
   if ( !status ) {
       // DebugPrint( 1, "IOCTL failed with error code%d.\n\n", GetLastError() );
       return -(int)GetLastError();
   }

   //**************************************************************************
   //  Now, prepare for the call to SCSI_GET_CONFIGURATION,
   //  which will privide thedevice code that we want...
   //**************************************************************************
   ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
   sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH);
   sptwb.Spt.PathId = 0;
   sptwb.Spt.TargetId = 1;
   sptwb.Spt.Lun = 0;
   // sptwb.Spt.CdbLength = CDB6GENERIC_LENGTH;
   sptwb.Spt.CdbLength = CDB10GENERIC_LENGTH;
   sptwb.Spt.SenseInfoLength = 24;
   sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
   sptwb.Spt.DataTransferLength = 192;
   sptwb.Spt.TimeOutValue = 10;
   sptwb.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf);
   sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,SenseBuf);
   // sptwb.Spt.Cdb[0] = SCSIOP_INQUIRY;
   // sptwb.Spt.Cdb[4] = 192;
   length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf) +
      sptwb.Spt.DataTransferLength;

   // If device supports SCSI-3, then we can get the CD drive capabilities, i.e. ability to 
   // read/write to CD-ROM/R/RW or/and read/write to DVD-ROM/R/RW.  
   // Use the previous spti structure, only modify the command to "mode sense"
   sptwb.Spt.Cdb[0] = 0x46;   // This is the GET CONFIGURATION command
   sptwb.Spt.Cdb[1] = 2;      // A bit to say get 1 page of information
   sptwb.Spt.Cdb[8] = 192;    //  buffer length
   status = DeviceIoControl(hDevice,
                            IOCTL_SCSI_PASS_THROUGH,
                            &sptwb,
                            sizeof(SCSI_PASS_THROUGH),
                            &sptwb,
                            length,
                            &returned,
                            FALSE);

   // PrintCapResults(status, returned, &sptwb); 
   // uint dbuflen = (uint) sptwb.DataBuf[3] ;
   // hex_dump((u8 *) sptwb.DataBuf, dbuflen+8) ;
   return get_cd_cfg(status, &sptwb); 
}

//*************************************************************************
char *get_dev_name(uint profile_code)
{
   uint idx ;
   for (idx=0; ProfileInfo[idx].profile_code != 0; idx++) {
      if (ProfileInfo[idx].profile_code == profile_code) {
         return ProfileInfo[idx].description ;
      }
   }
   return "Unknown" ;
}

//****************************************************************************
// #define  STAND_ALONE    1
#ifdef  STAND_ALONE
static char *cd_names[4] = {
"CD_ROM", "CD_RW", "DVD_ROM", "DVD_RW"   
} ;

int main (void)
{
   char szTemp[4096], szName[64];
   HANDLE hCD = CreateFile ("\\\\.\\D:",
      GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

   GetCDDeviceName (hCD, szTemp, szName);
   // printf("szTemp=[%s]\n", szTemp) ;
   printf("szName=[%s]\n", szName) ;
   //           1         2         3
   //         89012345678901234567890123456789
   //         vvvvvvvvpppppppppppppppprrrr
   // szName=[HL-DT-STDVDRAM GH41N    MN01]
   int cd_cfg = GetDeviceConfig(hCD) ;
   printf("cd_cfg returned %d [%s]\n", cd_cfg, get_dev_name(cd_cfg)) ;

   int cd_type = GetDeviceProperty(hCD) ;
   if (cd_type < 0) {
      printf("get_cd_type failed [%d]\n", cd_type) ;
   } else {
      printf("CD type is %s\n", cd_names[cd_type]) ;
   }

   CloseHandle (hCD);

   return 0;
}
#else
char *get_cd_device_desc(char drv)
{
   // char szTemp[4096], szName[64];
   char drive_path[10] = "\\\\.\\D:" ; //lint !e1778  not const safe
   drive_path[4] = drv ;

   HANDLE hCD = CreateFile (drive_path, 
      GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

   int cd_cfg = GetDeviceConfig(hCD) ;
   char *dev_name = get_dev_name(cd_cfg) ;

   // GetCDDeviceName (hCD, szTemp, szName);
   // printf("szTemp=[%s]\n", szTemp) ;
   // printf("szName=[%s]\n", szName) ;
   //           1         2         3
   //         89012345678901234567890123456789
   //         vvvvvvvvpppppppppppppppprrrr
   // szName=[HL-DT-STDVDRAM GH41N    MN01]
   // int cd_type = GetDeviceProperty(hCD) ;
   // if (cd_type < 0) {
   //    printf("get_cd_type failed [%d]\n", cd_type) ;
   // } else {
   //    printf("CD type is %s\n", cd_names[cd_type]) ;
   // }
   CloseHandle (hCD);
   return dev_name;
}
#endif

