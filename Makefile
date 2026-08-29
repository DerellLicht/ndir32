USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES
USE_CLANG = YES

# sadly, cygwin mingw does not support gdiplus...
USE_CYGWIN = NO
# use -static for clang and cygwin/mingw
#  clang vs tdm
#  clang gives *much* clearer compiler error messages...
#  However, programs built with clang++ will require libc++.dll and libunwind.dll
#  in order to be used elsewhere 
#  (unless built with -static, which significantly boosts file size)

# the legacy version of qualify.cpp, does not depend upon c++ string class
USE_LEGACY = NO

include der_libs\tool_select.mak

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LiFLAGS += -dUNICODE -d_UNICODE
IFLAGS += -DUNICODE -D_UNICODE
endif

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

ifeq ($(USE_LEGACY),YES)
CFLAGS += -DLEGACY_QUALIFY
endif

LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
IFLAGS += -Ider_libs

# This is required for *some* versions of makedepend
IFLAGS += -DNOMAKEDEPEND

CPPSRC=Ndir32.cpp cmd_line.cpp config.cpp conio32.cpp Diskparm.cpp err_exit.cpp Filelist.cpp \
Fileread.cpp Ndisplay.cpp nio.cpp nsort.cpp treelist.cpp tdisplay.cpp mediatype.cpp \
read_link.cpp GetLinkTarget.cpp \
der_libs/common_funcs.cpp 

ifeq ($(USE_LEGACY),YES)
CPPSRC+=der_libs/qualify_orig.cpp 
else
CPPSRC+=der_libs/qualify.cpp 
endif

OBJS = $(CPPSRC:.cpp=.o)

# uuid.lib, ole32.lib : used in read_link.cpp
LIBS=-lmpr -lshlwapi -luuid -lole32 

BASE := ndir

# Automatically parse the latest version block
VERSION := $(shell grep -oE '\[[0-9]+\.[0-9]+\]' CHANGELOG.md | head -n 1 | tr -d '[]')
DIST_ZIP := $(BASE)V$(VERSION).zip
#*************************************************************************
%.o: %.cpp
	$(TOOLS)/$(GNAME) $(CFLAGS) $< -o $@

ifeq ($(USE_64BIT),NO)
BIN = $(BASE)32.exe
else
BIN = $(BASE)64.exe
endif

all: $(BIN)

clean:
	rm -f $(OBJS) ndir*.exe *~ *.zip
	
testme:	
	@cmd /C "@echo Preparing GitHub release for v$(VERSION)...dist: $(DIST_ZIP) "

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:\llvm\bin\clang-tidy.exe $(CPPSRC)"

dist:
	rm -f $(DIST_ZIP)
	zip $(DIST_ZIP) $(BIN) readme.txt LICENSE.txt CHANGELOG.md

# Your new automated release workflow
release:
	cmd /C "@echo Preparing GitHub release for v$(VERSION)..."
	sed -n '/## \['$(VERSION)'\]/,/## \[/p' CHANGELOG.md | sed '$$d' > temp_notes.md
	gh release create v$(VERSION) ./$(DIST_ZIP) --notes-file temp_notes.md
	rm temp_notes.md
	cmd /C "@echo Release v$(VERSION) successfully uploaded to GitHub!"wc:
	wc -l $(CPPSRC)

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp der_libs/lintdefs.ref.h $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC) $(CXXSRC)

$(BIN): $(OBJS)
	$(TOOLS)/$(GNAME) $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 

# DO NOT DELETE

Ndir32.o: der_libs/common.h ndir32.h conio32.h der_libs/qualify.h treelist.h
cmd_line.o: der_libs/common.h ndir32.h conio32.h
config.o: der_libs/common.h ndir32.h conio32.h
conio32.o: der_libs/common.h ndir32.h conio32.h
Diskparm.o: der_libs/common.h ndir32.h
err_exit.o: der_libs/common.h ndir32.h conio32.h
Filelist.o: der_libs/common.h ndir32.h conio32.h
Fileread.o: der_libs/common.h ndir32.h conio32.h
Ndisplay.o: der_libs/common.h ndir32.h conio32.h
nio.o: der_libs/common.h ndir32.h conio32.h
nsort.o: der_libs/common.h ndir32.h conio32.h
treelist.o: der_libs/common.h ndir32.h conio32.h treelist.h
tdisplay.o: der_libs/common.h ndir32.h conio32.h treelist.h
mediatype.o: scsi_defs.h
read_link.o: der_libs/common.h ndir32.h
GetLinkTarget.o: der_libs/common.h ndir32.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/qualify.o: der_libs/common.h der_libs/qualify.h
