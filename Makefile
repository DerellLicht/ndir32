USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES

# notes on 64-bit toolchains
# d:\tdm64\bin      is gcc v5.1.0;  NDIR64 built with this is 106KB
# c:\tdm-gcc-64\bin is gcc v10.3.0; NDIR64 built with this is 178KB
ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
#TOOLS=c:\tdm-gcc-64\bin
else
#TOOLS=c:\mingw\bin
TOOLS=c:\tdm32\bin
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

#  clang-tidy options
CHFLAGS = -header-filter=.*
CHTAIL = -- 
CHTAIL += -Ider_libs
ifeq ($(USE_64BIT),YES)
CHTAIL += -DUSE_64BIT
endif
ifeq ($(USE_UNICODE),YES)
CHTAIL += -DUNICODE -D_UNICODE
endif

LIBS=-lmpr -lshlwapi -lole32 -luuid

LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
IFLAGS += -Ider_libs
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

CPPSRC=Ndir32.cpp cmd_line.cpp config.cpp conio32.cpp Diskparm.cpp err_exit.cpp Filelist.cpp Fileread.cpp \
	Ndisplay.cpp nio.cpp nsort.cpp treelist.cpp tdisplay.cpp mediatype.cpp read_link.cpp \
	der_libs\common_funcs.cpp \
	der_libs\qualify.cpp 

OBJS = $(CPPSRC:.cpp=.o)

#*************************************************************************
%.o: %.cpp
	$(TOOLS)\g++ $(CFLAGS) -c $< -o $@

ifeq ($(USE_64BIT),NO)
BIN = ndir32.exe
else
BIN = ndir64.exe
endif

all: $(BIN)

clean:
	rm -f *.o ndir*.exe *~ *.zip

check:
	cmd /C "d:\clang\bin\clang-tidy.exe $(CHFLAGS) $(CPPSRC) $(CHTAIL)"

dist:
	rm -f ndir.zip
	zip ndir.zip $(BIN) readme.txt revisions.txt LICENSE

wc:
	wc -l $(CPPSRC)

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC) $(CXXSRC)

$(BIN): $(OBJS)
	$(TOOLS)\g++ $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 

# DO NOT DELETE

Ndir32.o: der_libs/common.h ndir32.h conio32.h der_libs/qualify.h
cmd_line.o: der_libs/common.h ndir32.h conio32.h
config.o: der_libs/common.h ndir32.h
conio32.o: der_libs/common.h ndir32.h conio32.h
Diskparm.o: der_libs/common.h ndir32.h
err_exit.o: der_libs/common.h ndir32.h conio32.h
Filelist.o: der_libs/common.h ndir32.h conio32.h
Fileread.o: der_libs/common.h ndir32.h
Ndisplay.o: der_libs/common.h ndir32.h conio32.h
nio.o: der_libs/common.h ndir32.h conio32.h
nsort.o: der_libs/common.h ndir32.h
treelist.o: der_libs/common.h ndir32.h conio32.h treelist.h
tdisplay.o: der_libs/common.h ndir32.h conio32.h treelist.h
mediatype.o: scsi_defs.h
read_link.o: der_libs/common.h ndir32.h
der_libs\common_funcs.o: der_libs/common.h
der_libs\qualify.o: der_libs/qualify.h
