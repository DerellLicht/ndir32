//  read/parse a midi file

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char  u8 ;

typedef struct midi_header_s {
   char           ID[4];
   unsigned long  length; //  length of data within chunk
} midi_header_t, *midi_header_p ;

char fname[1024] ;

typedef struct mthd_chunk_s {
   /* Here's the 8 byte header that all chunks must have */
   char           ID[4];  /* This will be 'M','T','h','d' */
   unsigned long  length; /* This will be 6 */

   /* Here are the 6 bytes */
   unsigned short Format;
   unsigned short NumTracks;
   unsigned short Division;
} mthd_chunk_t, *mthd_chunk_p ;

//*****************************************************************************
void usage(void)
{
   puts("Usage: rmidi midi_filename") ;
}

//*****************************************************************************
unsigned short swap2(unsigned short uvalue2)
{
   u8 utemp ;
   ul2uc_t uconv ;
   uconv.us[0] = uvalue2 ;
   utemp = uconv.uc[0] ;
   uconv.uc[0] = uconv.uc[1] ;
   uconv.uc[1] = utemp ;
   return uconv.us[0] ;
}

//*****************************************************************************
unsigned swap4(unsigned uvalue4)
{
   u8 utemp ;
   ul2uc_t uconv ;
   uconv.ul = uvalue4 ;
   utemp = uconv.uc[0] ;
   uconv.uc[0] = uconv.uc[3] ;
   uconv.uc[3] = utemp ;
   utemp = uconv.uc[1] ;
   uconv.uc[1] = uconv.uc[2] ;
   uconv.uc[2] = utemp ;
   return uconv.ul ;
}

//*****************************************************************************
void parse_midi_event(char *bfr, unsigned track_len)
{
   u8 event = (u8) *bfr++ ;
   // u8 evlen = (u8) *bfr++ ;
   bfr++ ;  //  skip evlen for now
   switch (event) {
   case 0x21:  //  Midi port:  FF 21 01 pp
      printf("Midi port %u, track_len=%u\n", (unsigned) (u8) *bfr, track_len) ;
      break;

   case 0x58:  //  Time Signature: FF 58 04 nn dd cc bb   
      {
      int nn = *bfr++ ;
      int dd = *bfr++ ;
      int cc = *bfr++ ;
      int bb = *bfr++ ;
      unsigned denom = 1 << (unsigned) dd ;
      printf("Time signature: n=%d, d=%d, c=%d, b=%d, tlen=%u\n", nn, dd, cc, bb, track_len) ;
      printf("   %u:%u time\n", nn, denom) ;
      printf("   %u MIDI clocks in a metronome click\n", cc) ;
      printf("   %u notated 32nd notes in a MIDI quarter note (24 MIDI clocks)\n", bb) ;

      }         
      break;

   default:
      printf("event 0x%02X not yet supported (%u len)\n", (unsigned) (u8) *bfr, track_len) ;
      break;
   }
}

//*****************************************************************************
int main(int argc, char **argv)
{
   int j ;
   char *p ;
   fname[0] = 0 ;
   //  parse command line
   for (j=1; j<argc; j++) {
      p = argv[j] ;
      if (*p == '-') {
         p++ ;
         switch (*p) {
         default:
            usage() ;
            return 1 ;
         }
      } else {
         strncpy(fname, p, sizeof(fname)) ;
      }
   }

   //  validate inputs
   if (fname[0] == 0) {
      usage() ;
      return 1;
   }
   printf("reading %s\n", fname) ;

   int hdl = open(fname, O_BINARY | O_RDONLY) ;
   if (hdl < 0) {
      perror(fname) ;
      return 1;
   }
   u8 bfr[1024] ;
   unsigned chlen ;
   char hdr[5] ;
   off_t idx = 0 ;
   while (1) {
      int rdbytes = read(hdl, bfr, sizeof(bfr)) ;
      if (rdbytes <= 0) 
         break;
      if (idx == 0) {
         mthd_chunk_p thdptr = (mthd_chunk_p) bfr ;
         memcpy(hdr, thdptr->ID, 4) ;
         hdr[4] = 0 ;
         chlen = swap4(thdptr->length) ;
         unsigned f = swap2(thdptr->Format) ;
         unsigned n = swap2(thdptr->NumTracks) ;
         int d = (int) swap2(thdptr->Division) ;

         printf("%s: %u bytes, format=%u, tracks=%u, division=%d\n", hdr, chlen, f, n, d) ;
      } else {
         midi_header_p mptr = (midi_header_p) bfr ;
         memcpy(hdr, mptr->ID, 4) ;
         hdr[4] = 0 ;
         if (strncmp(hdr, "MTrk", 4) != 0) {
            printf("unknown midi track %s\n", hdr) ;
            chlen = 0 ; //  probably meaningless
         } else {
            chlen = swap4(mptr->length) ;
            if (bfr[8] != 0x00  ||  bfr[9] != 0xFF) {
               printf("%s: 0x%02X 0x%02X: unknown event, %u bytes\n", 
                  hdr, (unsigned) bfr[8], (unsigned) bfr[9], chlen) ;
            }
            else {
               parse_midi_event((char *) &bfr[10], chlen) ;
            }
         }
      }

// 00000000  4d 54 68 64 00 00 00 06  00 01 00 0a 00 78 4d 54  |MThd.........xMT|
// 00000010  72 6b 00 00 00 19 00 ff  58 04 04 02 18 08 00 ff  |rk......X.......|
      //  goto next chunk
      idx += chlen + 8 ;
      // printf("seek offset 0x%X\n", (unsigned) idx) ;
      lseek(hdl, idx, SEEK_SET) ;
   }
   close(hdl) ;
   
   return 0;
}

