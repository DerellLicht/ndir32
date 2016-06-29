#include <stdio.h>

typedef unsigned char  u8 ;
typedef unsigned int   u32 ;

//**************************************************************
typedef struct id3size_in_s {
   unsigned p0 : 7 ;
   unsigned z0 : 1 ;
   unsigned p1 : 7 ;
   unsigned z1 : 1 ;
   unsigned p2 : 7 ;
   unsigned z2 : 1 ;
   unsigned p3 : 7 ;
   unsigned z3 : 1 ;
} id3size_in_t ;

typedef union id3conv_in_s {
   id3size_in_t d ;
   u32 raw ;
} id3conv_in_t ;

typedef struct id3size_out_s {
   unsigned p0 : 7 ;
   unsigned p1 : 7 ;
   unsigned p2 : 7 ;
   unsigned p3 : 7 ;
   unsigned z3 : 4 ;
} id3size_out_t ;

typedef union id3conv_out_s {
   id3size_out_t d ;
   u32 raw ;
} id3conv_out_t ;

unsigned get_id3_size(u8 *uptr)
{
   id3conv_in_t din ;
   id3conv_out_t dout ;

   din.raw = 0 ;
   din.d.p3 = (u8) *uptr++ ;
   din.d.p2 = (u8) *uptr++ ;
   din.d.p1 = (u8) *uptr++ ;
   din.d.p0 = (u8) *uptr++ ;

   dout.d.p3 = din.d.p3 ;
   dout.d.p2 = din.d.p2 ;
   dout.d.p1 = din.d.p1 ;
   dout.d.p0 = din.d.p0 ;
   return dout.raw ;
}

//**************************************************************
int main(int argc, char **argv)
{
   // u8 id3data[4] = { 0x00, 0x00, 0x02, 0x01 } ; //  257
   u8 id3data[4] = { 0x00, 0x01, 0x02, 0x1A } ;

   u32 id3size = get_id3_size(&id3data[0]) ;
   printf("id3size=%u [0x%X] bytes\n", id3size, id3size) ;
   return 0;
}
