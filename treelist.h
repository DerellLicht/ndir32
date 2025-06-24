
//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   dirs *brothers {nullptr};
   dirs *sons {nullptr};
   // TCHAR *name {nullptr};
   std::wstring   name {};
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

extern dirs *top ;

extern unsigned level;

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);


