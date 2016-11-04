SHELL=cmd.exe
USE_DEBUG = NO
LIBS=-lmpr -lshlwapi

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -s -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
#***************************************************************
#  gcc options
#  Without c99:   (but %llu doesn't work)
#    65024 ->     28160   43.31%    win32/pe     ndir32.exe
#  With c99:   %llu works
#    85504 ->     39424   46.11%    win32/pe     ndir32.exe
#CFLAGS += -std=c99
#***************************************************************
#  After upgrading from g++ 4.3.3 to g++ 4.4.1,
#  I can now get longlong to printf correctly.
#  %llu doesn't work, but %I64u does, and file size is MUCH smaller
#  once I can remove -std=c++98 
#    80896 ->     33280   41.14%    win32/pe     ndir32.exe  
#  g++ options
#  Without c++98: (but %llu doesn't work)
#    76288 ->     33280   43.62%    win32/pe     ndir32.exe
#  With c++98: %llu works
#    96768 ->     44544   46.03%    win32/pe     ndir32.exe
#CFLAGS += -std=c++98
#LFLAGS += -std=c++98
#  Interesting... if I undefine this here, it appears to circumvent -std=c++98,
#  and thus printf("%llu") remains broken.
#  However, if I include it in each of the files which call functions
#  from string.h, then everything works fine... 
#CFLAGS += -U__STRICT_ANSI__
#***************************************************************

CPPSRC=cmd_line.cpp conio32.cpp Diskparm.cpp err_exit.cpp Filelist.cpp Fileread.cpp \
	mft_list.cpp ndata.cpp Ndir32.cpp Ndisplay.cpp Qualify.cpp \
	nsort.cpp treelist.cpp mp3.parser.cpp file_fmts.cpp mediatype.cpp

OBJS = $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

#*************************************************************************
%.o: %.cpp
	g++ $(CFLAGS) $<

all: ndir32.exe

clean:
	rm -f *.o ndir32.exe *~ *.zip

source:
	rm -f *.zip
	zip -D ndir32src.zip *.cpp *.h *.cpp readme.txt revisions.txt makefile
   
dist:
	rm -f ndir.zip
	zip ndir.zip ndir32.exe readme.txt revisions.txt   

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CPPSRC)"

depend: 
	makedepend $(CSRC) $(CPPSRC)

ndir32.exe: $(OBJS)
	g++ $(OBJS) $(LFLAGS) -o ndir32.exe $(LIBS) 
	upx -9 ndir32.exe

# DO NOT DELETE

cmd_line.o: ndir32.h conio32.hpp
conio32.o: conio32.hpp
Diskparm.o: ndir32.h conio32.hpp
err_exit.o: ndir32.h conio32.hpp
Filelist.o: conio32.hpp ndir32.h
Fileread.o: ndir32.h
ndata.o: ndir32.h
Ndir32.o: ndir32.h conio32.hpp
Ndisplay.o: ndir32.h conio32.hpp file_fmts.hpp
nsort.o: ndir32.h
treelist.o: ndir32.h conio32.hpp
mp3.parser.o: ndir32.h
file_fmts.o: ndir32.h file_fmts.hpp
mediatype.o: scsi_defs.h
