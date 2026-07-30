//**********************************************************
//  directory structure for directory_tree routines
//**********************************************************
struct dirs
{
   std::vector<dirs> brothers {};
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
   uint maxlen {};
   uint submaxlen {};
};

extern dirs dlist ;

extern unsigned level;

//  treelist.cpp
void tree_listing (unsigned total_filespec_count);

//  tdisplay.cpp
void draw_dir_tree (void);


