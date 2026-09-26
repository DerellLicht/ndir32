# ndir_ubsan.mak : UBSan (undefined-behavior sanitizer) build of ndir
# Usage:  make -f ndir_ubsan.mak
# Test:   ndir_ubsan.exe [args] 2> ubsan.txt     (keeps reports out of the color output)
#
# Requires clang (USE_CLANG = YES); the sanitizer runtime ships with LLVM.
# tool_select.mak is expected to set TOOLS and GNAME (presumably clang++ here).

USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES
USE_CLANG = YES

USE_CYGWIN = NO
USE_LEGACY = NO

include der_libs\tool_select.mak
include der_libs\release.mak

# NEW: sanitizer switches.  These must be on BOTH the compile and link steps
#  -fsanitize=undefined            : insert the runtime UB checks
#  -fno-sanitize-recover=undefined : abort at the first report (no continue-and-print)
UBSAN = -fsanitize=undefined -fno-sanitize-recover=undefined

# NEW: keep the optimization level of the real build (-O3), since that is
# where latent UB tends to show up.  Change to -O2 to compare.
OPT = -O3

# CHANGED: always -g (file/line numbers in reports); -fno-omit-frame-pointer
# gives better stack traces; -MMD -MP writes .d header-dependency files
# (replaces the makedepend list at the bottom of the original makefile).
CFLAGS = -Wall $(OPT) -g -fno-omit-frame-pointer $(UBSAN) -MMD -MP -c
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings

# CHANGED: no -s (stripping symbols would defeat line numbers in reports)
LFLAGS = $(OPT) -g $(UBSAN)

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
endif

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

ifeq ($(USE_LEGACY),YES)
CFLAGS += -DLEGACY_QUALIFY
endif

CFLAGS += -Ider_libs

CPPSRC=Ndir32.cpp cmd_line.cpp config.cpp conio32.cpp Diskparm.cpp err_exit.cpp Filelist.cpp \
Fileread.cpp Ndisplay.cpp nio.cpp nsort.cpp treelist.cpp tdisplay.cpp mediatype.cpp \
read_link.cpp GetLinkTarget.cpp ubsan_hooks.cpp \
der_libs/common_funcs.cpp 

ifeq ($(USE_LEGACY),YES)
CPPSRC+=der_libs/qualify_orig.cpp 
else
CPPSRC+=der_libs/qualify.cpp 
endif

# CHANGED: separate .ubsan.o suffix so these objects never mix with the normal build's .o files
OBJS = $(CPPSRC:.cpp=.ubsan.o)

# uuid.lib, ole32.lib : used in read_link.cpp
LIBS=-lmpr -lshlwapi -luuid -lole32 

# CHANGED: distinct output name so the normal ndir64.exe is not overwritten
BIN = ndir_ubsan.exe

#*************************************************************************
# CHANGED: compile rule matches the .ubsan.o suffix
%.ubsan.o: %.cpp
	$(TOOLS)/$(GNAME) $(CFLAGS) $< -o $@

all: $(BIN)

# NEW: remove UBSan objects, dependency files, and the exe (leaves the normal build alone)
clean:
	rm -vf $(OBJS) $(OBJS:.o=.d) $(BIN)

# Link with the sanitizer flags so the driver adds the UBSan runtime libraries
$(BIN): $(OBJS)
	$(TOOLS)/$(GNAME) $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 

# NEW: pull in the generated header dependencies (silently skipped on first build)
-include $(OBJS:.o=.d)
