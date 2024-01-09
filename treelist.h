
//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   dirs *brothers;
   dirs *sons;
   char *name;
   uchar attrib;
   ULONGLONG dirsize;
   ULONGLONG dirsecsize;
   ULONGLONG subdirsize;
   ULONGLONG subdirsecsize;
   unsigned files;
   unsigned directs;
   unsigned subfiles;
   unsigned subdirects;
   bool     is_multi_byte ;
};

extern dirs *top ;

extern unsigned level;


