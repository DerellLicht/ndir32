SHELL=cmd.exe
USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = NO

ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
else
TOOLS=c:\mingw\bin
endif

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -s -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
ifeq ($(USE_64BIT),YES)
CFLAGS += -DUSE_64BIT
endif

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LFLAGS += -dUNICODE -d_UNICODE 
endif

LIBS=-lmpr -lshlwapi -lole32 -luuid

#***************************************************************
#  After upgrading from g++ 4.3.3 to g++ 4.4.1,
#  I can now get longlong to printf correctly.
#  %llu doesn't work, but %I64u does, and file size is MUCH smaller
#  once I can remove -std=c++98 (Well, not on 64-bit build...)
#    80896 ->     33280   41.14%    win32/pe     ndir32.exe  
#  g++ options
#  Without c++98: (but %llu doesn't work)
#    76288 ->     33280   43.62%    win32/pe     ndir32.exe
#  With c++98: %llu works (no, it doesn't)
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
	common.cpp Ndir32.cpp Ndisplay.cpp Qualify.cpp \
	nsort.cpp treelist.cpp tdisplay.cpp mediatype.cpp read_link.cpp
#    mp3.parser.cpp file_fmts.cpp 

OBJS = $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

#*************************************************************************
%.o: %.cpp
	$(TOOLS)\g++ $(CFLAGS) $<

ifeq ($(USE_64BIT),NO)
BIN = ndir32.exe
else
BIN = ndir64.exe
endif

all: $(BIN)

clean:
	rm -f *.o ndir*.exe *~ *.zip

source src:
	rm -f *.zip
	zip -D ndir32src.zip *.cpp *.h *.cpp readme.txt revisions.txt makefile
   
dist:
	rm -f ndir.zip
	zip ndir.zip $(BIN) readme.txt revisions.txt   

wc:
	wc -l *.cpp

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CPPSRC)"

depend: 
	makedepend $(CSRC) $(CPPSRC)

$(BIN): $(OBJS)
	$(TOOLS)\g++ $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 
ifeq ($(USE_DEBUG),NO)
ifeq ($(USE_64BIT),NO)
	upx -9 $(BIN)
else
	upx64 -9 $(BIN)
endif
endif

# DO NOT DELETE

cmd_line.o: common.h ndir32.h conio32.h
conio32.o: common.h ndir32.h conio32.h
Diskparm.o: common.h ndir32.h conio32.h
err_exit.o: common.h ndir32.h conio32.h
Filelist.o: common.h ndir32.h conio32.h
Fileread.o: common.h ndir32.h
common.o: common.h ndir32.h
Ndir32.o: common.h ndir32.h conio32.h qualify.h
Ndisplay.o: common.h ndir32.h conio32.h
Qualify.o: qualify.h
nsort.o: common.h ndir32.h
treelist.o: common.h ndir32.h conio32.h treelist.h
tdisplay.o: common.h ndir32.h conio32.h treelist.h
mediatype.o: scsi_defs.h
read_link.o: common.h ndir32.h
