#define  USE_VECTOR
// #undef  USE_VECTOR

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
#ifdef  USE_VECTOR
   std::vector<dirs> brothers {};
#else   
   dirs *brothers {nullptr};
   dirs *sons {nullptr};
#endif   
   std::wstring name {};
   uchar attrib {};
   ULONGLONG dirsize {};
   ULONGLONG dirsecsize {};
   ULONGLONG subdirsize {};
   ULONGLONG subdirsecsize {};
   uint files {};
   uint directs {};
   uint subfiles {};
   uint subdirects {};
};

#ifdef  USE_VECTOR
extern dirs dlist ;
#else
extern dirs *top ;
#endif

extern unsigned level;

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);

//  tdisplay.cpp
void draw_dir_tree (void);


