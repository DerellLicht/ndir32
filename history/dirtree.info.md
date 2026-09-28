I have a problem with the directory-tree display function in my `ndir32` color directory lister.

I'll try to provide a summary of the program, then the problem situation.
I'll then try to give a brief overview of how data is set up, and what I don't remember any more,
then I'll hand the relevant files to you, and ask you to figure out what I used to know...
and while you're at it, as usual, you can explain what I've been doing wrong, all along!!

## summary of ndir color directory lister, directory tree mode

`ndir32` has a number of display modes; one of them is a directory tree that shows
the folder tree below a specified folder node, and displays a variety of different info
depending upon specified command-line arguments.  For example, the file 
`ndir.tree.list.txt`, included in this post, shows a tree listing of the `ndir32` repo
folder tree, with the `.git` folders excluded.

## overview of console buffer handling for this program
- related files: conio32.cpp, conio32.h

`console_init()` [conio32.cpp line 125] is called from `main()`, and initializes the console environment 
for the program.  The primary function in this function is the call:
```c++
   bSuccess = GetConsoleScreenBufferInfo(hStdOut, &sinfo) ;
```

This populates the struct `static CONSOLE_SCREEN_BUFFER_INFO sinfo ;`
which is used elsewhere in this file for various console operations.
the following line, which is the output from the following `syslog()` call,
provides you with context for my current console:
```c++
   // dwSize: 135x2000, cursor: 0,50, max: 135x127, window: L0, T1, R134, B50   
   // syslog(L"dwSize: %ux%u, cursor: %u,%u, max: %ux%u, window: L%u, T%u, R%u, B%u\n",
   //    sinfo.dwSize.X, sinfo.dwSize.Y,
   //    sinfo.dwCursorPosition.X,
   //    sinfo.dwCursorPosition.Y,
   //    sinfo.dwMaximumWindowSize.X, sinfo.dwMaximumWindowSize.Y, 
   //    sinfo.srWindow.Left, sinfo.srWindow.Top, sinfo.srWindow.Right, sinfo.srWindow.Bottom);
```

## code flow for directory-tree listing
- related files: tdisplay.cpp, nio.cpp, con32.cpp

The core function that generates this display is `display_dir_tree()` [tdisplay.cpp Line 97] ; 
This is a recursive function which is called once for each folder, to handle all the folders
below that one... 

Each pass through the loop in `display_dir_tree()` prints one row of data, representing one
folder and related data... the interesting element in that loop is `ncrlf ();` [Line 287]

`ncrlf()` [nio.cpp Line 59] calls a variety of other functions in `nio.cpp` and `conio32.cpp` ;
So this function and its subsidiaries handle all of the newline, pause, exit, and other tasks 
involved in scrolling the pages.

## What is going wrong

If I generate a listing that has more than 2000 lines, I start getting corrupted data on screen.
This does *not* occur if I redirect to file; see the included file `target.output.txt` to see
almost 4600 lines of perfect output... but if I'm displaying onscreen, then just past line 2000, 
I start getting corruption.  

Check out the image `ndir.tree.fault.jpg`; the folder `JapanesePhoneticAnalysis` is at line 2000.

So it's pretty clear to me, that whatever I'm doing when I reach the end of that buffer, 
isn't correct... but I don't know what *is* correct... an `ndir` user named "Jason Hood" 
provided me with some guidance back in 2004, about dealing with buffer > screen, but 
I don't recall much of what we talked about, and of course all those emails are long gone...

##  So... please help??
Could you look over all of this and see if you have any insights on this issue??
I would be eternally grateful (once again!!)... 



