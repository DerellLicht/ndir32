//*****************************************************************
//  Copyright (c) 1998-2006 Daniel D. Miller                       
//  file_fmts.cpp - file-format parsers for 
//  multimedia (/mm) display.
//*****************************************************************

// #undef __STRICT_ANSI__
#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>  //  PATH_MAX
#endif
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ndir32.h"
#include "file_fmts.h"

//*********************************************************************
static unsigned short get2bytes(unsigned char *p)
{
   ul2uc_t uconv ;
   
   uconv.uc[1] = *p++ ;
   uconv.uc[0] = *p ;
   return uconv.us[0] ;
}

//*********************************************************************
static unsigned get4bytes(unsigned char *p)
{
   ul2uc_t uconv ;
   
   uconv.uc[3] = *p++ ;
   uconv.uc[2] = *p++ ;
   uconv.uc[1] = *p++ ;
   uconv.uc[0] = *p ;
   return uconv.ul ;
}

//************************************************************************
static unsigned char dbuffer[256] ;
static char fpath[260] ;

static int read_into_dbuffer(char *fname)
{
   int hdl, rbytes ;

   sprintf(fpath, "%s\\%s", base_path, fname) ;
   hdl = open(fpath, O_BINARY | O_RDONLY) ;
   if (hdl < 0) 
      return errno;
   
   //  read header data
   rbytes = read(hdl, dbuffer, sizeof(dbuffer)) ;
   if (rbytes < 0) {
      close(hdl) ;
      return errno;
   }
   close(hdl) ;
   return 0;
}

//***************************************************************************
//  buffers/functions for compiling size subtotals
//***************************************************************************

//**********************************************************************
//lint -esym(751, icon_entry_t)  variable not referenced
typedef struct icon_entry_s {
   u8 Width ;        //  Cursor Width (16, 32 or 64)
   u8 Height ;       //  Cursor Height (16, 32 or 64 , most commonly = Width)
   u8 ColorCount ;   //  Number of Colors (2,16, 0=256)
   u8 Reserved   ;   //  =0
   u16 Planes    ;   //  =1
   u16 BitCount  ;   //  bits per pixel (1, 4, 8)
   u32 SizeInBytes ; //  Size of (InfoHeader + ANDbitmap + XORbitmap)
   u32 FileOffset ;  //  FilePos, where InfoHeader starts
} icon_entry_t, *icon_entry_p ;

//**********************************************************************
static int get_ico_cur_info(char *fname, char *mlstr, u8 decider)
{
   int result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable GIF") ;
   } else {
      u16 *uptr = (u16 *) dbuffer ;
      if (*uptr != 0) {
         sprintf(tempstr, "offset 0 bad: 0x%04X", *uptr) ;
         sprintf(mlstr, "%-28s", tempstr) ;
         return 0;
      }
      uptr++ ;
      if (*uptr != decider) {
         sprintf(tempstr, "offset 2 bad: 0x%04X", *uptr) ;
         sprintf(mlstr, "%-28s", tempstr) ;
         return 0;
      }
      uptr++ ;
      u16 NumIcons = *uptr++ ;
      if (NumIcons == 0) {
         sprintf(tempstr, "No icons in file") ;
         sprintf(mlstr, "%-28s", tempstr) ;
         return 0;
      }
      u16 idx ;
      u32 MaxWidth = 0 ;
      // printf("[%s] number of icons in file = %u\n", fname, NumIcons) ;
      icon_entry_p iptr ;
      iptr = (icon_entry_p) (char *) uptr ;
      for (idx=0; idx<NumIcons; idx++) {
         // u32 colors = (u32) iptr->ColorCount ;
         // if (colors == 0)
         //    colors = 256 ;
         // printf("  %u: %u x %u, %u colors\n", idx, iptr->Width, iptr->Height, colors) ;
         if (MaxWidth < iptr->Width) 
             MaxWidth = iptr->Width ;
         iptr++ ;
      }
      iptr = (icon_entry_p) (char *) uptr ;
      for (idx=0; idx<NumIcons; idx++) {
         if (MaxWidth == iptr->Width) {
            u32 colors = (u32) iptr->ColorCount ;
            if (colors == 0)
               colors = 256 ;
            // printf("  %u: %u x %u, %u colors\n", idx, iptr->Width, iptr->Height, colors) ;
            sprintf(tempstr, "%4u x %4u x %u colors", iptr->Width, iptr->Height, colors) ;
            sprintf(mlstr, "%-28s", tempstr) ;
         }
         iptr++ ;
      }
   }
   return 0 ;
}

int get_ico_info(char *fname, char *mlstr)
{
   return get_ico_cur_info(fname, mlstr, 1) ;
}

int get_cur_info(char *fname, char *mlstr)
{
   return get_ico_cur_info(fname, mlstr, 2) ;
}

//************************************************************************
//  A JFIF-standard file will start with the four bytes (hex) FF D8 FF E0,
//  followed by two variable bytes (often hex 00 10), followed by 'JFIF'
//************************************************************************
// SOF0: Start Of Frame 0:
// ~~~~~~~~~~~~~~~~~~~~~~~
// 
//   - $ff, $c0 (SOF0)
//   - length (high byte, low byte), 8+components*3
//   - data precision (1 byte) in bits/sample, usually 8 (12 and 16 not
//    supported by most software)
//   - image height (2 bytes, Hi-Lo), must be >0 if DNL not supported
//   - image width (2 bytes, Hi-Lo), must be >0 if DNL not supported
//   - number of components (1 byte), usually 1 = grey scaled, 3 = color YCbCr
//    or YIQ, 4 = color CMYK)
//   - for each component: 3 bytes
//     - component id (1 = Y, 2 = Cb, 3 = Cr, 4 = I, 5 = Q)
//     - sampling factors (bit 0-3 vert., 4-7 hor.)
//     - quantization table number
// 
//  Remarks:
//   - JFIF uses either 1 component (Y, greyscaled) or 3 components (YCbCr,
//    sometimes called YUV, colour).
//************************************************************************

static char const * const jpeg_fmt[3] = {
   "Jpeg", "Jfif", "Exif" 
} ;

#define  SOI_FLAG    ((unsigned short) 0xFFD8)
#define  SOF0_FLAG   ((unsigned short) 0xFFC0)
#define  SOF2_FLAG   ((unsigned short) 0xFFC2)
#define  APP0_FLAG   ((unsigned short) 0xFFE0)
#define  EXIF_FLAG   ((unsigned short) 0xFFE1)

int get_jpeg_info(char *fname, char *mlstr)
{
   int hdl, rbytes ;
   // unsigned char *hd ;
   // union ul2uc uconv ;
   // unsigned char utemp ;
   unsigned format = 0, rows, cols ;
   // unsigned short *usptr ;
   unsigned foffset = 0 ;  //  offset into file
   unsigned seglen ;
   unsigned short uflag ;
   int inbuffer ;
   // int result = 0 ;

   sprintf(fpath, "%s\\%s", base_path, fname) ;

   hdl = open(fpath, O_BINARY | O_RDONLY) ;
   if (hdl < 0) 
      goto jpeg_unreadable;

   rows = cols = 0 ;
   inbuffer = 1 ;
   while (inbuffer >= 0) {
      // printf("reading %5u bytes at offset %u\n", 100, foffset) ;
      rbytes = read(hdl, dbuffer, 100) ;
      //  check for errors,
      if (rbytes < 0) {
         // result = errno ;
         break;
      } 
      //  see if we're done reading file
      else if (rbytes == 0) {
         // result = 0 ;
         break;
      }

      uflag = get2bytes(dbuffer) ;  
      switch (uflag) {
      case 0:  //  probably end of file
         inbuffer = -1 ; //  break out of parse loop, to read file
         // result = 0 ;
         break;
         
      case SOF0_FLAG:
      case SOF2_FLAG:
         seglen = get2bytes(&dbuffer[2]) ;
         foffset += seglen + 2 ;
         lseek(hdl, foffset, SEEK_SET) ;  // seek new file position

         //  extract the data that I want
         cols = get2bytes(&dbuffer[5]) ;
         rows = get2bytes(&dbuffer[7]) ;
         break;
         
      case SOI_FLAG:
         // puts("found JPEG START_OF_IMAGE flag") ;
         foffset += 2 ;
         lseek(hdl, foffset, SEEK_SET) ;  // seek new file position
         break;

      case APP0_FLAG:
         format = 1 ;
         seglen = get2bytes(&dbuffer[2]) ;
         foffset += seglen + 2 ;
         lseek(hdl, foffset, SEEK_SET) ;  // seek new file position
         break;

      case EXIF_FLAG:
         format = 2 ;
         seglen = get2bytes(&dbuffer[2]) ;
         foffset += seglen + 2 ;
         lseek(hdl, foffset, SEEK_SET) ;  // seek new file position
         break;

      default:
         seglen = get2bytes(&dbuffer[2]) ;
         // printf("found 0x%04X flag, segment length = %5u bytes\n", uflag, seglen) ;
         foffset += seglen + 2 ;
         lseek(hdl, foffset, SEEK_SET) ;  // seek new file position
         break;
      }  //  switch uflag 

   }  //  outer loop to read more buffer data
   close(hdl) ;

   //  build the return string
   if (rows == 0  &&  cols == 0) {
      sprintf(mlstr, "%4s - no SOF0 segment      ", jpeg_fmt[format]) ;
   } else {
      sprintf(mlstr, "%4u x %4u  %4s           ", 
         rows, cols, jpeg_fmt[format]) ;
   }
   return 0 ;

jpeg_unreadable:
   sprintf(mlstr, "%-28s", "unreadable jpg ") ;
   return 0 ;
}

//*********************************************************
typedef struct {
   long           chunkSize;
   short          wFormatTag;
   unsigned short wChannels;
   unsigned long  dwSamplesPerSec;
   unsigned long  dwAvgBytesPerSec;
   unsigned short wBlockAlign;
   unsigned short wBitsPerSample;
   /* Note: there may be additional fields here, depending upon wFormatTag. */
} FormatChunk;
                
int get_wave_info(char *fname, char *mlstr)
{
   int result, rbytes ;
   FormatChunk *fcptr ;
   struct stat st ;
   off_t fsize ;
   char *hd ;
   unsigned srate ;
   double ptime ;

   sprintf(fpath, "%s\\%s", base_path, fname) ;

   //  get file size
   result = stat(fpath, &st) ;
   if (result < 0) {
      sprintf(mlstr, "%-28s", "cannot stat Wave file") ;
      return 0;
   }
   fsize = st.st_size ;

   result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable Wave") ;
   } 
      //  first, search for the "fmt" string
   else if (strncmp((char *)  dbuffer,    "RIFF", 4) != 0  ||
            strncmp((char *) &dbuffer[8], "WAVE", 4) != 0) {
      sprintf(mlstr, "%-28s", "unknown wave format") ;
   }
   else {
      hd = (char *) &dbuffer[12] ;
      rbytes = 0 ;
      while (LOOP_FOREVER) {
         if (strncmp(hd, "fmt ", 4) == 0) {
            hd += 4 ;
            break;
         }
         hd++ ;
         //  make sure we stop before search string overruns buffer
         if (++rbytes == (sizeof(dbuffer) - 4)) {
            // return EILSEQ;
            sprintf(mlstr, "%-28s", "no fmt in Wave file") ;
            return 0;
         }
      }
      fcptr = (FormatChunk *) hd ;
      srate = fcptr->dwSamplesPerSec ;
      ptime = (double) fsize / (double) fcptr->dwAvgBytesPerSec ;
      total_ptime += ptime ;
      unsigned uplay_secs = (unsigned) ptime ;
      unsigned uplay_mins = uplay_secs / 60 ;
      uplay_secs = uplay_secs % 60 ;
      
      if (ptime < 60.0) {
         sprintf(mlstr, "%5u hz, %6.2f seconds    ", srate, ptime) ;
      } else {
         // ptime /= 60.0 ;
         sprintf(mlstr, "%5u hz, %3u:%02u minutes    ", srate, uplay_mins, uplay_secs) ;
      }
   }
   return 0;
}

//*********************************************************
// WebP file header:
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      'R'      |      'I'      |      'F'      |      'F'      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           File Size                           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      'W'      |      'E'      |      'B'      |      'P'      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 
// Extended WebP file header:
// 
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                   WebP file header (12 bytes)                 |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      ChunkHeader('VP8X')                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Rsv|I|L|E|X|A|R|                   Reserved                    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |          Canvas Width Minus One               |             ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// ...  Canvas Height Minus One    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// VP8X format:  1800x1200 = 0x0708, 0x04b0, off by 1
// https://www.ietf.org/id/draft-zern-webp-07.html#name-riff-file-format
// 0000 0000 52 49 46 46 30 bb 05 00  57 45 42 50 56 50 38 58  RIFF0+.WEBPVP8X
// 0000 0010 0a 00 00 00 20 00 00 00  07 07 00 af 04 00 49 43  .... ....».IC

// VP8 format:   576x768 = x0240, 0x0300, off by 0
// 0000 0000 52 49 46 46 4c b9 01 00  57 45 42 50 56 50 38 20  RIFFL¦.WEBPVP8
// 0000 0010 40 b9 01 00 10 dd 04 9d  01 2a 40 02 00 03 3e 51  @¦.¦¥*@.>Q
//                                         |wwwww|lllll|

// Future note:
// VP8L format:
// The beginning of the header has the RIFF container. This consists of the following 21 bytes:
// 
//    String "RIFF"
//    A little-endian 32 bit value of the block length, the whole size of the block 
//    controlled by the RIFF header. Normally this equals the payload size 
//    (file size minus 8 bytes: 4 bytes for the 'RIFF' identifier and 4 bytes for 
//    storing the value itself).
//    String "WEBP" (RIFF container name).
//    String "VP8L" (chunk tag for lossless encoded image data).
//    A little-endian 32-bit value of the number of bytes in the lossless stream.
//    One byte signature 0x2f.
// 
// The first 28 bits of the bitstream specify the width and height of the image. 
// Width and height are decoded as 14-bit integers as follows:
// 
// int image_width = ReadBits(14) + 1;
// int image_height = ReadBits(14) + 1;

int get_webp_info(char *fname, char *mlstr)
{
   sprintf(fpath, "%s\\%s", base_path, fname) ;
   int result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable WebP") ;
   } 
      //  first, search for the "fmt" string
   else if (strncmp((char *)  dbuffer,    "RIFF", 4) != 0  ||
            strncmp((char *) &dbuffer[8], "WEBP", 4) != 0) {
      sprintf(mlstr, "%-28s", "unknown webp format") ;
   }
   else {
      ul2uc_t uconv ;
      char *hd ;
      unsigned width, height ;
      switch (dbuffer[15]) {
      case 'X':
         hd = (char *) &dbuffer[24] ;
         uconv.ul = 0;
         uconv.uc[0] = *hd++ ;
         uconv.uc[1] = *hd++ ;
         uconv.uc[2] = *hd++ ;
         width = uconv.ul + 1;
         uconv.ul = 0;
         uconv.uc[0] = *hd++ ;
         uconv.uc[1] = *hd++ ;
         height = uconv.ul + 1;
         sprintf(mlstr, "%4u x %4u  VP8X format    ", width, height) ;
         break ;
         
      case ' ':
         if (dbuffer[23] == 0x9D  &&
             dbuffer[24] == 0x01  &&
             dbuffer[25] == 0x2A) {
            hd = (char *) &dbuffer[26] ;
            uconv.ul = 0;
            uconv.uc[0] = *hd++ ;
            uconv.uc[1] = *hd++ ;
            width = uconv.ul ;
            uconv.ul = 0;
            uconv.uc[0] = *hd++ ;
            uconv.uc[1] = *hd++ ;
            height = uconv.ul ;
            sprintf(mlstr, "%4u x %4u  VP8 format     ", width, height) ;
         } else {
            sprintf(mlstr, "VP8X bad sync code    ") ;
         }
         break ;
      
      case 'L':
         sprintf(mlstr, "VP8L format - need support   ") ;
         break ;

      default:
         sprintf(mlstr, "VP8%c format - need support   ", dbuffer[15]) ;
         break ;      
      }
   }
   return 0;
}

//************************************************************************
//  sf_1946.sid: 15176 x 27811 x 256  (3B48 x 6CA3 X 100)
// 00000000  6d 73 69 64 01 00 01 00  00 00 01 00 00 00 06 00  |msid............|
// 00000010  00 3b 48 00 00 6c a3 00  00 00 69 00 03 5c d4 00  |.;H..l....i..\..|
// 00000020  05 07 06 00 06 b3 8a 00  08 3f 72 00 0d 88 1e 00  |.........?r.....|
// 00000030  12 cd 29 00 17 a1 7e 00  25 d0 e5 00 34 1c 40 00  |..)...~.%...4.@.|
// 00000040  40 f2 30 00 64 17 0a 00  87 30 2a 00 a4 81 7e 00  |@.0.d....0*...~.|
// 00000050  d8 d5 87 01 11 f1 d4 01  32 7b 0a 01 32 7b 12 01  |........2{..2{..|
// 00000060  35 c6 72 ff d0 01 35 c6  7a ff ac 00 53 00 00 00  |5.r...5.z...S...|
// 00000070  01 00 00 00 85 00 00 00  00 00 00 00 00 00 00 00  |................|
// 00000080  ed 00 00 01 b2 ff aa 01  00 00 00 00 00 00 00 00  |................|
// 00000090  00 00 00 00 00 00 00 67  40 b9 51 65 00 00 00 80  |.......g@.Qe....|
// 000000a0  00 00 00 80 00 00 00 c4  00 00 88 e7 00 00 fd df  |................|
//************************************************************************
//  foothill.sid: 11520 x 11264 x truecolor  (2D00 x 2C00 x ???)
// 00000000  6d 73 69 64 01 00 01 00  00 00 03 00 00 00 09 00  |msid............|
// 00000010  00 2d 00 00 00 2c 00 00  00 00 8d 00 00 0d e0 00  |.-...,..........|
// 00000020  00 1a cc 00 00 28 3e 00  00 35 29 00 00 67 6e 00  |.....(>..5)..gn.|
// 00000030  00 9a 4a 00 00 cd 2d 00  01 94 6a 00 02 5c d6 00  |..J...-...j..\..|
// 00000040  03 25 68 00 06 29 c0 00  09 2a 86 00 0c 36 bc 00  |.%h..)...*...6..|
// 00000050  10 7f 89 00 14 c7 ec 00  18 a7 63 00 23 01 8d 00  |..........c.#...|
// 00000060  2d 50 5b 00 36 34 06 00  49 91 da 00 5c 86 1a 00  |-P[.64..I...\...|
// 00000070  6a 6d 18 00 88 1c 5f 00  a5 8b 48 00 ba 76 16 00  |jm...._...H..v..|
// 00000080  ba 76 1e 00 ba 76 26 ff  d0 00 ba 76 2e ff ac 00  |.v...v&....v....|
// 00000090  1e 00 00 00 01 00 00 00  b1 00 00 05 0c 00 00 09  |................|
// 000000a0  81 00 00 00 00 00 00 00  00 00 00 00 16 00 00 00  |................|
// 000000b0  15 ff aa 01 01 00 00 00  00 00 00 00 00 00 00 00  |................|
// 000000c0  00 00 00 67 68 40 30 57  77 00 00 00 80 00 00 00  |...gh@0Ww.......|
// 000000d0  80 00 00 00 d5 ff a3 82  48 c5 d5 65 4c 02 27 59  |........H..eL.'Y|
// 000000e0  51 f4 c7 70 58 1d 27 3b  14 e1 0b 64 06 37 d2 c2  |Q..pX.';...d.7..|
// 000000f0  51 8b 90 a8 96 5b 60 27  3f 96 a2 8a f2 85 ee 32  |Q....[`'?......2|
//************************************************************************
typedef struct sid_info_s {
   char msid[4] ;
   u8 unknown4[6] ;
   u8 color_clue1 ;
   u8 unknown11[3] ;
   u8 color_clue2 ;
   u32 width ;
   u32 height ;
// } sid_info_t, *sid_info_p ;
} __attribute__ ((packed)) sid_info_t ;
typedef sid_info_t *sid_info_p ;

u32 swap32(u32 invalue)
{
   ul2uc_t uconv ;
   uconv.ul = invalue ;
   u8 utemp ;
   utemp = uconv.uc[0] ;
   uconv.uc[0] = uconv.uc[3] ;
   uconv.uc[3] = utemp ;
   utemp = uconv.uc[1] ;
   uconv.uc[1] = uconv.uc[2] ;
   uconv.uc[2] = utemp ;
   return uconv.ul;
}

int get_sid_info(char *fname, char *mlstr)
{
   sid_info_p sid_info ;
   unsigned rows, cols, bpp ;

   int result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable SID") ;
   } else if (strnicmp((char *) dbuffer, "msid", 4) != 0) {
      sprintf(mlstr, "%-28s", "unknown SID format") ;
   } else {
      sid_info = (sid_info_p) &dbuffer[0] ;
      cols = swap32(sid_info->width) ;
      rows = swap32(sid_info->height) ;
      bpp  = sid_info->color_clue1 * 8 ;

      sprintf(tempstr, "%4u x %4u x %u bpp", cols, rows, bpp) ;
      sprintf(mlstr, "%-28s", tempstr) ;
   }
   return 0 ;
}

//************************************************************************
//lint -esym(751, gif_info_t)  variable not referenced
typedef struct gif_info_s {
   unsigned width  : 16 ;
   unsigned height : 16 ;
   unsigned bpp : 3 ;   //  (bpp+1) = bits per pixel
   unsigned zro : 1 ;   //  always zero
   unsigned cr : 3 ; //  (cr + 1) = bits of color resolution
   unsigned M : 1 ;  //  1 if global color map follows descriptor
} gif_info_t, *gif_info_p ;

// 47 49 46 38 39 61 GIF89a
// 10 00  
// 10 00 
// 22 = 0 010 0 010
// b3 = 10 110 011

int get_gif_info(char *fname, char *mlstr)
{
   int result ;
   gif_info_p gif_info ;
   unsigned rows, cols, bpp ;

   result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable GIF") ;
   } else if (strnicmp((char *) dbuffer, "gif87a", 6) != 0   &&
              strnicmp((char *) dbuffer, "gif89a", 6) != 0) {
      sprintf(mlstr, "%-28s", "unknown GIF format") ;
   } else {
      gif_info = (gif_info_p) &dbuffer[6] ;  //  look past the label
      cols = gif_info->width ;
      rows = gif_info->height ;
      bpp  = gif_info->bpp + 1 ;

      sprintf(tempstr, "%4u x %4u x %u colors", cols, rows, (1U << bpp)) ;
      sprintf(mlstr, "%-28s", tempstr) ;
   }
   return 0 ;
}

//************************************************************************
int get_bmp_info(char *fname, char *mlstr)
{
   int result ;
   unsigned rows, cols, bpp ;
   // BITMAPFILEHEADER *bmfh ;
   BITMAPINFO *bmi ;

   result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable BMP") ;
   } else {
      //  see what we got
      // bmfh = (BITMAPFILEHEADER *) dbuffer ;
      bmi  = (BITMAPINFO *) &dbuffer[sizeof(BITMAPFILEHEADER)] ;

      cols = bmi->bmiHeader.biWidth ;
      rows = bmi->bmiHeader.biHeight ;
      bpp  = bmi->bmiHeader.biBitCount ;

      if (bpp > 16) {
         sprintf(mlstr, "%4u x %4u x %2u bpp        ", cols, rows, bpp) ;
      } else {
         sprintf(tempstr, "%4u x %4u x %u colors", cols, rows, (1U << bpp)) ;
         sprintf(mlstr, "%-28s", tempstr) ;
      }
   }
   return 0 ;
}

//************************************************************************
// The IHDR chunk must appear FIRST. It contains:
// 
//    Width:              4 bytes
//    Height:             4 bytes
//    Bit depth:          1 byte
//    Color type:         1 byte
//    Compression method: 1 byte
//    Filter method:      1 byte
//    Interlace method:   1 byte
//************************************************************************
// typedef struct png_info_s {
//    unsigned width ;
//    unsigned height ;
//    unsigned char  bitDepth ;
//    unsigned char  colorType ;
// } png_info_t, *png_info_p ;

int get_png_info(char *fname, char *mlstr)
{
   int result ;
   unsigned rows, cols ;
   unsigned char bitDepth, colorType ;
   unsigned char *p ;
   unsigned bcount = 0 ;

   result = read_into_dbuffer(fname) ;
   if (result != 0) {
      sprintf(mlstr, "%-28s", "unreadable PNG") ;
   } else {
      //  scan along for the IHDR string
      p = dbuffer ;
      while (LOOP_FOREVER) {
         if (strncmp((char *) p, "IHDR", 4) == 0) {
            p += 4 ;
            break;
         }
         p++ ;
         if (++bcount == sizeof(dbuffer)) {
            p = 0 ;
            break;
         }
      }
      if (p == 0) {
         sprintf(mlstr, "%-28s", "no IHDR in PNG") ;
         return 0;
      }
      
      //  see what we got
      cols = get4bytes(p) ;  p += 4 ;
      rows = get4bytes(p) ;  p += 4 ;
      bitDepth  = *p++ ;
      colorType = *p++ ;

      if (bitDepth == 8  &&  colorType == 2) {
         sprintf(mlstr, "%4u x %4u x 24 bpp        ", cols, rows) ;
      } else {
         sprintf(tempstr, "%4u x %4u, [%u, %u]",  
            cols, rows, colorType, bitDepth) ;
         sprintf(mlstr, "%-28s", tempstr) ;
      }
   }
   return 0 ;
}

//************************************************************************
typedef struct {
    DWORD  dwMicroSecPerFrame;
    DWORD  dwMaxBytesPerSec;
    DWORD  dwReserved1;
    DWORD  dwFlags;
    DWORD  dwTotalFrames;
    DWORD  dwInitialFrames;
    DWORD  dwStreams;
    DWORD  dwSuggestedBufferSize;
    DWORD  dwWidth;
    DWORD  dwHeight;
    DWORD  dwScale;
    DWORD  dwRate;
    DWORD  dwStart;
    DWORD  dwLength;
} MainAVIHeader;

int get_avi_info(char *fname, char *mlstr)
{
   int result ;
   unsigned rows, cols, bcount ;
   double run_time ;
   MainAVIHeader *avih ;
   char *hd ;

   result = read_into_dbuffer(fname) ;
   if (result != 0) 
      goto avi_unreadable;

   //  1. check for "RIFFnnnnAVI LIST"
   if (strncmp((char *) dbuffer, "RIFF", 4) != 0  ||
       strncmp((char *) &dbuffer[8], "AVI LIST", 8) != 0) 
      goto avi_unknown;
   
   //  2. search for "hdrlavih", which is followed by MainAVIHeader
   hd = (char *) &dbuffer[16] ;
   bcount = 0 ;
   rows = cols = 0 ;
   run_time = 0.0 ;
   while (LOOP_FOREVER) {
      if (strncmp(hd, "hdrlavih", 8) == 0) {
         avih = (MainAVIHeader *) (hd+12) ;
         cols = avih->dwWidth ;
         rows = avih->dwHeight ;
         run_time = (double) (avih->dwMicroSecPerFrame / 1000000.0)
                  * (double)  avih->dwTotalFrames ;
         break;
      }
      hd++ ;
      if (++bcount >= sizeof(dbuffer))
         goto avi_no_avih;
   }

   if (run_time < 60.0) {
      sprintf(tempstr, "%4u x %4u, %.2f secs", cols, rows, run_time) ;
   } else {
      run_time /= 60.0 ;
      sprintf(tempstr, "%4u x %4u, %.2f mins", cols, rows, run_time) ;
   }
   sprintf(mlstr, "%-28s", tempstr) ;
   return 0 ;

avi_unreadable:
   sprintf(mlstr, "%-28s", "unreadable avi ") ;
   return 0 ;
avi_unknown:
   sprintf(mlstr, "%-28s", "unknown avi format") ;
   return 0 ;
avi_no_avih:
   sprintf(mlstr, "%-28s", "no hdrlavih header found") ;
   return 0 ;
}

