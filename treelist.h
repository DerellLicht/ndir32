#define  USE_VECTOR

//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
#ifdef  USE_VECTOR
   std::vector<dirs> brothers {};
   std::vector<dirs> sons {};
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
   uint mb_len {};
};

#ifdef  USE_VECTOR
extern std::vector<dirs> dlist ;
#else
extern dirs *top ;
#endif

extern unsigned level;

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);


