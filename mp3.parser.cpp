//***************************************************************************************
//  Copyright (c) 1998-2020 Daniel D. Miller                       
//  mp3.parser.cpp - mp3 file parser
//***************************************************************************************
// From The File Extension Source. 
// The standards document for the MP3_ENC bitstream (ISO/IEC 11172-3:1993, 
// section 2.4.2.3, p. 20) states that MPEG-1 audio headers begins 
// with "1111 1111 1111" (syncword) followed by "1" (for this standard), followed 
// by "01" for layer III, followed by "1" if the stream has no redundancy, and 
// then provides bits that indicate stream's bitrate. The bitstream "1111 1111 
// 1111 1011" has "FFFB" as its hexadecimal equivalent.
// 
// When ID3 metadata is placed in a MP3_FF file, established conventions call 
// for the use of the ID3 marker which consists of the string "3DI". For ID3v1, 
// the marker is positioned 10 bytes preceding the metadata. For ID3v2 data, the 
// marker is positioned in the first 3 bytes of the file when the metadata is 
// pre-pended and 10 bytes from the end of a file when it is post-pended.
//***************************************************************************************
// from: http://www.dv.co.yu/mpgscript/mpeghdr.htm
// Note: as of June 2020, the preceding site non-existent.
// replacement site is: http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
//  
// How to calculate frame length 
// 
// First, let's distinguish two terms frame size and frame length. 
// Frame size is the number of samples contained in a frame. 
// It is constant and always 384 samples for Layer I and 1152 samples 
// for Layer II and Layer III. 
// Frame length is length of a frame when compressed.  It is calculated 
// in slots.  One slot is 4 bytes long for Layer I, and one byte long 
// for Layer II and Layer III.  When you are reading MPEG file you must 
// calculate this to be able to find each consecutive frame. 
// Remember, frame length may change from frame to frame due to padding 
// or bitrate switching. 
// 
// Read the BitRate, SampleRate and Padding of the frame header. 
// 
// For Layer I files use this formula: 
// 
//     FrameLengthInBytes = (12 * BitRate / SampleRate + Padding) * 4 
// 
// For Layer II & III files use this formula: 
// 
//     FrameLengthInBytes = 144 * BitRate / SampleRate + Padding 
// 
// Example:
// Layer III, BitRate=128000, SampleRate=441000, Padding=0
//       ==>  FrameSize=417 bytes 
//***************************************************************************************
//  Another excellent analytical tool is Konrad Windszus's MpegAudioInfo :
//  http://www.codeproject.com/audio/MPEGAudioInfo.asp
//***************************************************************************************
//  To compile this module as a stand-alone utility:
//  g++ -Wall -s -O3 -Wno-write-strings -DDO_CONSOLE mp3.parser.cpp -o mp3parse.exe
//***************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>  //  read()
#include <windows.h>

#include "ndir32.h"

typedef unsigned char  u8 ;
typedef unsigned int   u32 ;

//  this buffer needs to be large enough to completely read one frame.
//  An lseek() will be used to point to the next frame.
#define  RD_BFR_SZ   (4 * 1024)
static u8 rd_bfr[RD_BFR_SZ] ;

//  11-bit mask for testing frame_sync field
#define REF_FRAME_SYNC  0x7FF

//   b17.mp3
// 0000 0000 ff e3 20 c4 00 0e d8 76  50 01 49 10 01 04 c5 7a 
// 0000 0010 80 50 08 06 05 04 88 d1  a3 46 8d 1a 34 73 84 21 

//   ba exit.mp3
// 0000 0000 ff fb 50 c4 00 00 00 00  00 00 00 00 00 00 00 00 
// 0000 0010 00 00 00 00 00 49 6e 66  6f 00 00 00 0f 00 00 01 

// 00000:  FF F3 40 C4 
typedef struct mp3_frame_header_s {
   unsigned emphasis : 2 ;                                                 // 0,1
   unsigned original_media : 1 ;                                           // 2
   unsigned copyrighted : 1 ;                                              // 3
   unsigned mode_ext : 2 ; //  Joint stereo only                           // 4,5
   unsigned channel_mode : 2 ;                                             // 6,7
   
   unsigned private_info : 1 ;                                             // 
   unsigned padding : 1 ;                                                  // 
   unsigned sample_freq : 2 ;                                              // 
   unsigned bitrate_index : 4 ;                                            //
    
   unsigned crc_protection : 1 ;                                           // 
   unsigned layer_desc : 2 ;                                               // 
   //  DDM note: Per ISO-IEC-11172-3, these first two fields are different,
   //  as shown in these comments.  Coments from MP3 Tech site:
   //  Note: MPEG Version 2.5 was added lately to the MPEG 2 standard. 
   //  It is an extension used for very low bitrate files, 
   //  allowing the use of lower sampling frequencies.  
   //  If your decoder does not support this extension, it is recommended 
   //  to use 12 bits for synchronization instead of 11 bits. 
   // unsigned id : 1 ;
   // unsigned syncword : 12 ;
   unsigned mpeg_audio_version_id : 2 ;
   unsigned frame_sync : 11 ;
} mp3_frame_header_t ;

typedef union mp3_parser_u {
   unsigned raw ;
   mp3_frame_header_t h ;
} mp3_parser_t ;

//***************************************************************************************
typedef struct mp3_frame_s {
   struct mp3_frame_s *next ;
   unsigned offset ;
   // unsigned status ;
   unsigned mpeg_version ; //  1=1, 2=2, 3=2.5
   unsigned mpeg_layer ;
   unsigned bitrate ;
   unsigned sample_rate ;
   unsigned prot_bit ;
   unsigned padding_bit ;
   unsigned csum_bit ;
   unsigned private_bit ;
   unsigned channel_mode ;
   unsigned mode_ext ;
   unsigned copyright ;
   unsigned original_media ;
   unsigned emphasis ;
   unsigned frame_length_in_bytes ;
   double play_time ;
   u32 raw ;
} mp3_frame_t, *mp3_frame_p ;

static mp3_frame_p frame_list = 0 ;
static mp3_frame_p frame_tail = 0 ;

//***************************************************************************************
// char *audio_ver[4] = { "MPEG 2.5", "reserved", "MPEG2", "MPEG1" } ;
// char *mpeg_layer[4] = { "reserved", "III", "II", "I" } ;

// int audio_ver_idx[4] = { 2, 0, 2, 1 } ;
static int audio_ver_idx2[4] = { 3, 0, 2, 1 } ; //  3 is actually 2.5
static int mpeg_layer_idx[4] = { 0, 3, 2, 1 } ;

static unsigned brtable[5][16] = {
{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
{ 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 },
{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }} ;

static unsigned samprate_table[3][4] = {
{ 44100, 48000, 32000, 0 },   
{ 22050, 24000, 16000, 0 },   
{ 11025, 12000,  8000, 0}} ;

//***************************************************************************************
static unsigned get_id3_size(u8 *uptr)
{
   u8 p3 = (u8) *uptr++ ;
   u8 p2 = (u8) *uptr++ ;
   u8 p1 = (u8) *uptr++ ;
   u8 p0 = (u8) *uptr++ ;
   // printf("size bytes=%02X:%02X:%02X:%02X\n", p3, p2, p1, p0) ;

   u32 isize = p3 ;
   isize = (isize << 7) + p2 ;
   isize = (isize << 7) + p1 ;
   isize = (isize << 7) + p0 ;
   return isize ;
}

//***************************************************************************************
//  mp3 header is actually stored big-endian, so we need to swap
//  the bytes around to work properly in a little-endian program.
//***************************************************************************************
static u32 get_mp3_header(u8 *rbfr)
{
   ul2uc_t uconv ;
   uconv.uc[3] = *rbfr++ ;
   uconv.uc[2] = *rbfr++ ;
   uconv.uc[1] = *rbfr++ ;
   uconv.uc[0] = *rbfr ;
   return uconv.ul ;
}

//***************************************************************************************
static int parse_mp3_frame(u8 *rbfr, unsigned offset)
{
   mp3_parser_t mp ;
   mp.raw = get_mp3_header(rbfr) ;
#ifdef DO_CONSOLE
   static unsigned fcount = 0 ;
   printf("parse_frame: rbuf: %02X %02X %02X %02X; raw: 0x%08X\n", 
      (u8) *rbfr, (u8) *(rbfr+1), (u8) *(rbfr+2), (u8) *(rbfr+3), mp.raw);
#endif
   if (mp.h.frame_sync != REF_FRAME_SYNC) {
#ifdef DO_CONSOLE
      printf("invalid frame sync A: %04X / %08X\n", mp.h.frame_sync, mp.raw);
#endif
      return 0;
   }

   mp3_frame_p mtemp = new mp3_frame_t ;
   if (mtemp == 0)   //lint !e774
      return -ENOMEM;
   memset((char *) mtemp, 0, sizeof(mp3_frame_t)) ;
   mtemp->offset = offset ;

   //  mp.h.datum contain the relevint info!!
   // printf("raw header: 0x%08X\n", mp.raw) ;
   // printf("mpeg version %s, layer %s\n", 
   //    audio_ver[mp.h.mpeg_audio_version_id],
   //    mpeg_layer[mp.h.layer_desc]) ;
   // int avidx = audio_ver_idx[mp.h.mpeg_audio_version_id] ;
   mtemp->mpeg_version = audio_ver_idx2[mp.h.mpeg_audio_version_id] ;
   mtemp->mpeg_layer = mpeg_layer_idx[mp.h.layer_desc] ;
   // if (mp.h.crc_protection)
   //    csum_count++ ;
   int bridx = mp.h.bitrate_index ;
   if (bridx == 0  ||  bridx == 15  ||  mtemp->mpeg_version == 0  ||  mtemp->mpeg_layer == 0) {
#ifdef DO_CONSOLE
      printf("bad header (offset 0x%08X) [%08X]: avidx=%d, mlidx=%d, bridx=%d\n", 
         offset, mp.raw, 
         mp.h.mpeg_audio_version_id, mtemp->mpeg_layer, bridx) ;
#endif
      delete mtemp ;
      // return -EINVAL;
      return 0;
   }
   mtemp->mpeg_version-- ;
   mtemp->mpeg_layer-- ;   // convert to base 0, as index
   int brtbl_idx ;
   if (mtemp->mpeg_version == 0) {
      brtbl_idx = mtemp->mpeg_layer ; //  0, 1, 2
   } else { //  avidx == 1
      brtbl_idx = (mtemp->mpeg_layer == 0) ? 3 : 4 ;
   }
   mtemp->bitrate      = brtable[brtbl_idx][bridx] ;
   mtemp->sample_rate  = samprate_table[mtemp->mpeg_version][mp.h.sample_freq] ;
   mtemp->padding_bit  = (unsigned) mp.h.padding ;        //lint !e571 Suspicious cast
   mtemp->csum_bit     = (unsigned) mp.h.crc_protection ; //lint !e571 Suspicious cast
   mtemp->channel_mode = (unsigned) mp.h.channel_mode ;
   mtemp->mode_ext     = (unsigned) mp.h.mode_ext     ;
   mtemp->raw          = mp.raw ;
#ifdef DO_CONSOLE
   printf("mpV=%u, mpL=%u, btidx=%u, bridx=%u, cm: %u, me: %u\n",
      // mtemp->mpeg_version, mtemp->mpeg_layer, brtbl_idx, bridx, mtemp->channel_mode, mtemp->mode_ext);
      mtemp->mpeg_version, mtemp->mpeg_layer, brtbl_idx, bridx, mp.h.channel_mode, mp.h.mode_ext);
#endif

   //  Frame length in bytes
   if (mtemp->sample_rate != 0  &&  mtemp->bitrate != 0) {
      // For Layer I files use this formula: 
      //     FrameLengthInBytes = (12 * BitRate / SampleRate + Padding) * 4 
      //  add tests to avoid divide-by-zero situations
      if (mtemp->mpeg_layer == 0) {
         mtemp->frame_length_in_bytes =
            ((12 * (mtemp->bitrate * 1000) / mtemp->sample_rate) + mtemp->padding_bit) * 4 ;
      }
      // For Layer II & III files use this formula: 
      //     FrameLengthInBytes = 144 * BitRate / SampleRate + Padding 
      else {
         u8 *next_frame ;
         u8 b1, b2 ;
         {
            mtemp->frame_length_in_bytes =
               (144 * (mtemp->bitrate * 1000) / mtemp->sample_rate) + mtemp->padding_bit;
               
            //  Issues in computing offset to next frame
            //  
            //  For various reasons, most of which I do not understand, a small number of 
            //  mp3 files do *not* calculate a correct byte length via this formula; 
            //  these are especially seen on small audio files.
            //  
            //  1. length is off by 1 or 2 bytes
            //  First issue is that computations may be short by one byte;
            //  this is the issue that is addressed by the hacked code below.
            //  NDIR hacks this be searching ahead by a few bytes, to find the
            //  11-bits-of-1s mask
            //  This was first seem in MPEG V2.5, but have been seen in V2 as well.
            //  
            //  2. length is multiple of actual offset to next frame
            //  Second issue is that for some files, this computation gives
            //  a length which is 2 or 3 times the actual bytes to next header.
            u32 advance = 0 ;
            u32 retries ;
#define  MAX_RETRIES    5
            for (retries=0; retries < MAX_RETRIES; retries++) {
               next_frame = rbfr + mtemp->frame_length_in_bytes + advance;
               b1 = *next_frame++ ;
               b2 = *next_frame ;
               //  This worked for V2.5 files, but not with V2.0,
               //  so cbrew.mp3 (for example) still failed
               if (mtemp->mpeg_version == 2) 
               {
                  if (b1 != 0xFF  ||  (b2 & 0xF0) != 0xE0  ||  b2 == 0xFF) {
                     advance++ ;
                  }
                  else {
                     break;
                  }
               }
               // if (mtemp->mpeg_version == 1) 
               else {
                  if (b1 != 0xFF  ||  (b2 & 0xF3) != 0xF3  ||  b2 == 0xFF) {
                        advance++ ;
                  }
                  else {
                     break;
                  }
               }
            }
            //  if we did *not* find the desired pattern by advancing,
            //  leave frame length at original value
            if (retries < MAX_RETRIES) {
               mtemp->frame_length_in_bytes += advance ;
            }
#ifdef DO_CONSOLE
            printf("bitrate: %u, sample_rate: %u, pad: %u, bytes: %u, retries: %u\n", 
               mtemp->bitrate, mtemp->sample_rate, mtemp->padding_bit,
               mtemp->frame_length_in_bytes, retries);
               
            next_frame = rbfr + mtemp->frame_length_in_bytes ;
            printf("parse_frame: next [%u]: [%02X] %02X %02X %02X %02X\n", retries, b1,
               (u8) *next_frame, (u8) *(next_frame+1), (u8) *(next_frame+2), 
               (u8) *(next_frame+3));
#endif
         }         
      }
#ifdef DO_CONSOLE
      printf("%u: bitrate [table %d] = %u Kbps, sample rate=%u Hz, frame len=%u, pad=%u\n", 
         fcount++, brtbl_idx, 
         mtemp->bitrate, mtemp->sample_rate, mtemp->frame_length_in_bytes,
         mtemp->padding_bit) ;
#endif

      mtemp->play_time = (double) (mtemp->frame_length_in_bytes * 8.0) / 
                         (double) (mtemp->bitrate * 1000.0) ;
   }

   //  add frame to list
   if (frame_list == 0)
      frame_list = mtemp ;
   else
      frame_tail->next = mtemp ;
   frame_tail = mtemp ;

   return mtemp->frame_length_in_bytes;
}  //  offset

//***************************************************************************************
static bool is_mp3_header_valid(u32 mheader)
{
   mp3_parser_t mp ;
   mp.raw = mheader ;
   if (mp.h.frame_sync != REF_FRAME_SYNC) {
#ifdef DO_CONSOLE
      printf("invalid frame sync B: %04X\n", mp.h.frame_sync);
#endif
      return 0;
   }
   if (mp.h.bitrate_index == 0  ||  mp.h.bitrate_index == 15) {
#ifdef DO_CONSOLE
      printf("invalid bitrate_index: %u\n", mp.h.bitrate_index);
#endif
      return 0;
   }
   return 1;
}

//***************************************************************************************
//  This function is used to find the FIRST frame in the file;
//  subsequent frames are found by calculating frame length.
//***************************************************************************************
static int find_mp3_signature(u8 *rbfr, unsigned rlen, int id3_offset)
{
   mp3_parser_t mp ;
   unsigned mp3_offset ;
   //  if ID3 offset is negative, then no such tag is present,
   //  so we *should* find the first mp3 header at offset 0.
   //  if this is not true, then scan specifically for 0xFFFB
   if (id3_offset < 0) {
      mp.raw = get_mp3_header(rbfr) ;
      if (is_mp3_header_valid(mp.raw))
         return 0;
      
      for (mp3_offset = 0; mp3_offset < rlen; mp3_offset++) {
         if (*(rbfr+mp3_offset) == 0xFF) {
            mp.raw = get_mp3_header(rbfr+mp3_offset) ;

            //  QUOTE (jonboy @ Aug 14 2006, 18:13) *
            //  Well, it turns out that the particular mp3 file I was testing uses "FFF3" 
            //  instead of "FFFB" to mark the headers.
            //  Does this mean no checksum? Is this pretty standard?
            // 
            //  Yeah. It means no checksum and it's actually MPEG2 
            //  (either 24000, 22050, or 16000 KHz)
            // if ((mp.raw & 0xFFFF0000) == 0xFFFB0000) {
            if ((mp.raw & 0xFFE00000) == 0xFFE00000) {
               // printf("offset %u, data=0x%08X\n", mp3_offset, mp.raw) ;
               return mp3_offset;
            }
         }
      }
   } else {
      u8 *tl = rbfr + id3_offset;
      for (mp3_offset = id3_offset; mp3_offset < rlen; mp3_offset++) {
         if (*(tl) == 0xFF  &&  (*(tl+1) & 0xE0) == 0xE0) {
            return mp3_offset;
         }
         tl++ ;
      }
   }
   return -1;
}

//***************************************************************************************
#ifndef DO_CONSOLE
static void clear_existing_list(void)
{
   mp3_frame_p mkill ;
   mp3_frame_p mtemp = frame_list; 
   frame_list = 0 ;
   while (mtemp != 0) {
      mkill = mtemp ;
      mtemp = mtemp->next ;
      delete mkill ;
   }
   
}
#endif

//***************************************************************************************
//  note that this function is *not* optimized for performance.
//  It does a new lseek() to locate each frame, and mp3 files tend to have
//  MANY frames.  It would be more efficient to buffer large chunks of
//  data and work thru the buffers, but that would make for more complex
//  code, partially because of allowing for frames (especially headers)
//  overlapping end of buffer.
//***************************************************************************************
static int read_mp3_file(char *fname)
{
   //  open file and start reading
//       int open(const char *pathname, int flags);
   int hdl = open(fname, O_BINARY | O_RDONLY) ;
   if (hdl < 0) {
      perror(fname) ;
      return -(int)errno;
   }
   // unsigned total_bytes = 0 ;
   int result = 0 ;
   int mp3_offset ;
   // unsigned prev_seek = 0 ;
   unsigned seek_byte = 0 ;
   unsigned first_pass = 1 ;
   // unsigned passct = 0 ;
   while (1) {
#ifdef DO_CONSOLE
      puts("");
#endif
      // printf("pass %u: seek_byte=%X, prev_seek=%X, diff=%u\n", 
      //    passct++, seek_byte, prev_seek, seek_byte-prev_seek) ;
      // ssize_t read(int fd, void *buf, size_t count);
      int rdbytes = read(hdl, rd_bfr, RD_BFR_SZ) ;
      if (rdbytes <= 0) {
         // printf("rdbytes=%d\n", rdbytes) ;
         //  rdbytes == 0  indicates EOF
         if (rdbytes < 0) {
            printf("offset %u: %s\n", seek_byte, strerror(errno)) ;
            result = -(int)errno;
         }
         break;
      }
      if (first_pass) {
         int id3size = -1 ;
         //  if there's an id3 header present, we need to 
         //  skip past it before looking for mp3 headers.
         if (rd_bfr[0] == 'I'  &&  rd_bfr[1] == 'D'  &&  rd_bfr[2] == '3') {
            id3size = get_id3_size(&rd_bfr[6]) ;
#ifdef DO_CONSOLE
            printf("id3 tag size=%u/0x%X bytes\n", id3size, id3size) ;
#endif
         }
         //  scan current buffer for mp3 data signature
         mp3_offset = find_mp3_signature(rd_bfr, rdbytes, id3size) ;
         if (mp3_offset < 0) {
            printf("%s: no first frame found...\n", fname) ;
            // return -EINVAL;
            result = -(int)EINVAL;
            break;
         }
#ifdef DO_CONSOLE
         printf("first frame at offset 0x%X\n", mp3_offset) ;
#endif

      } 
      //  otherwise, read should already be at correct offset
      else {
         mp3_offset = 0 ;
      }
      //  if you find an invalid frame, you probably found end-of-data.
      //  some files have ID data at end of file vs beginning.
      result = parse_mp3_frame(rd_bfr+mp3_offset, seek_byte) ;
      if (result <= 0) {
         if (result < 0) 
            printf("parse_frame: %s\n", strerror(-result)) ;
         break;
      }
      // prev_seek = seek_byte ;
      if (first_pass) {
         seek_byte += (unsigned) mp3_offset ;
      }
      seek_byte += (unsigned) result ;
      lseek(hdl, seek_byte, SEEK_SET) ;
      first_pass = 0 ;
   }
   close(hdl) ;
   return result ;
}

#ifdef DO_CONSOLE
//***************************************************************************************
void usage(void)
{
   puts("Usage: mymp3 mp3_filename") ;
}

//***************************************************************************************
int main(int argc, char **argv)
{
   char fname[1024] ;
   //  parse command line
   int j ;
   char *p ;
   fname[0] = 0 ;
   for (j=1; j<argc; j++) {
      p = argv[j] ;
      strncpy(fname, p, sizeof(fname)) ;
   }

   //  validate arguments
   if (fname[0] == 0) {
      usage() ;
      return 1;
   }

   printf("input: %s\n", fname) ;
   int result = read_mp3_file(fname) ;
   
   //***************************************************************
   //  now, see how many frames we found,
   //  and summarize the file information
   //***************************************************************
   if (result >= 0) {
      mp3_frame_p mtemp = frame_list; 
      unsigned bitrate  = mtemp->bitrate ;
      unsigned samprate = mtemp->sample_rate ;
      unsigned vbr = 0 ;   //  if bitrate changes, set this to TRUE

      unsigned frame_count = 0 ;
      double play_secs = 0.0 ;
      for (mtemp = frame_list; mtemp != 0; mtemp = mtemp->next) {
         frame_count++ ;
         play_secs += mtemp->play_time ;
         if (mtemp->bitrate != bitrate) 
            vbr = 1 ;
      }
      char *verid_str[3] = { "1", "2", "2.5" } ;
      char *layer_str[3] = { "I", "II", "III" } ;
      printf("found %u frames\n", frame_count) ;
      mtemp = frame_list; 
   // unsigned mpeg_version ; //  1=1, 2=2, 3=2.5
   // unsigned mpeg_layer ;
      printf("mpegV%s, layer %s, chnl_mode: %u, mode_ext: %u\n", 
         verid_str[mtemp->mpeg_version],
         layer_str[mtemp->mpeg_layer],
         mtemp->channel_mode,
         mtemp->mode_ext);
      if (vbr) {
         printf("bitrate=variable\n") ;
      } else {
         printf("bitrate=%uKbps\n", bitrate) ;
      }
      printf("sample rate=%u Hz\n", samprate) ;

      unsigned uplay_secs = (unsigned) play_secs ;
      unsigned uplay_mins = uplay_secs / 60 ;
      uplay_secs = uplay_secs % 60 ;
      printf("total play time=%u:%02u; raw first header: 0x%08X, fname: %s\n", 
         uplay_mins, uplay_secs, frame_list->raw, fname) ;
   }
   
   return 0;
}
#else
//*********************************************************
int get_mp3_info(char *fname, char *mlstr)
{
   int result ;
   char fpath[260] ;

   clear_existing_list() ;
   //    431,340 44100 hz,  2.45 seconds     Bell1.wav
   sprintf(fpath, "%s\\%s", base_path, fname) ;

   result = read_mp3_file(fpath);
   if (result < 0) {
      sprintf(mlstr, "%-28s", "cannot parse file") ;
      return 0;
   }

   mp3_frame_p mtemp = frame_list; 
   unsigned bitrate  = mtemp->bitrate ;
   // unsigned samprate = mtemp->sample_rate ;
   unsigned vbr = 0 ;   //  if bitrate changes, set this to TRUE

   // unsigned frame_count = 0 ;
   double play_secs = 0.0 ;
   for (mtemp = frame_list; mtemp != 0; mtemp = mtemp->next) {
      // frame_count++ ;
      play_secs += mtemp->play_time ;
      if (mtemp->bitrate != bitrate) 
         vbr = 1 ;
   }
   // char *layer_str[3] = { "I", "II", "III" } ;
   // char *verid_str[3] = { "1", "2", "2.5" } ;
   // printf("found %u frames\n", frame_count) ;
   // mtemp = frame_list; 
   // printf("mpegV%s, layer %s\n", 
   //    verid_str[mtemp->mpeg_version],
   //    layer_str[mtemp->mpeg_layer]) ;
   // if (vbr) {
   //    printf("bitrate=variable\n") ;
   // } else {
   //    printf("bitrate=%uKbps\n", bitrate) ;
   // }
   // printf("sample rate=%u Hz\n", samprate) ;

   total_ptime += play_secs ;
   unsigned uplay_secs = (unsigned) play_secs ;
   unsigned uplay_mins = uplay_secs / 60 ;
   uplay_secs = uplay_secs % 60 ;
   // sprintf(mlstr, "%5u hz, %5.2f seconds     ", srate, ptime) ;
   if (vbr) {
      sprintf(mlstr, "var Kbps, %3u:%02u minutes    ", uplay_mins, uplay_secs) ;
   } else {
      sprintf(mlstr, "%3u Kbps, %3u:%02u minutes    ", bitrate, uplay_mins, uplay_secs) ;
   }
   return 0;
}

#endif
