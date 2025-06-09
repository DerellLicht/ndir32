USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES
USE_CLANG = NO

# notes on 64-bit toolchains
# d:\tdm64\bin      is gcc v5.1.0;  NDIR64 built with this is 106KB
# c:\tdm-gcc-64\bin is gcc v10.3.0; NDIR64 built with this is 178KB
# after converting malloc/free to new/delete
# d:\tdm64\bin      is gcc v5.1.0;  NDIR64 built with this is 214KB
# c:\tdm-gcc-64\bin is gcc v10.3.0; NDIR64 built with this is 257KB
ifeq ($(USE_64BIT),YES)
#  interesting... the modern tdm64 is not handling Unicode as expected for tree list,
#  while the much older tdm64 works fine...
ifeq ($(USE_CLANG),YES)
TOOLS=d:\clang\bin
else
TOOLS=d:\tdm64\bin
#TOOLS=c:\tdm-gcc-64\bin
endif
else
TOOLS=c:\tdm32\bin
endif

#  clang++ note: you don't need two separate toolchain installations to build for 32 and 64 bit; 
#  it's enough with one of them, and you can call e.g. 
#  x86_64-w64-mingw32-clang++ to build for 64 bit and 
#  i686-w64-mingw32-clang++ to build for 32 bit. 
#  The prefixless clang++ builds for the architecture that is the default 
#     for the toolchain you're using.

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
ifeq ($(USE_64BIT),YES)
CFLAGS += -DUSE_64BIT
endif
CFLAGS += -std=c++11

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

ifeq ($(USE_CLANG),YES)
LFLAGS += -static
endif

LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
IFLAGS += -Ider_libs

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
	rm -f $(OBJS) ndir*.exe *~ *.zip

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
