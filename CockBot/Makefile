.POSIX:
.PHONY: all clean

MAJOR = 4
MINOR = 1.0

CC=clang
AR=ar

# Flags for shared library
#TARGET= -Dlinux  # riscos MOTO_CPU32
SHFLAGS = -fPIC
SHUTUPFLAGS=
CFLAGS= -O2 $(SHFLAGS) $(SHUTUPFLAGS) -w -g -Wpedantic -ffunction-sections -fdata-sections
CPPFLAGS= $(TARGET) -I.
LIBRARIES= -lm -lcurl -ldiscord -lpthread

FICLOBJECTS=ficl/dictionary.o ficl/system.o ficl/fileaccess.o ficl/float.o ficl/double.o ficl/prefix.o ficl/search.o ficl/softcore.o ficl/stack.o ficl/tools.o ficl/vm.o ficl/primitives.o ficl/bit.o ficl/lzuncompress.o ficl/ficlplatform/unix.o ficl/utility.o ficl/hash.o ficl/callback.o ficl/word.o ficl/extras.o
FICLHEADERS= ficl/ficl.h ficl/ficlplatform/unix.h ficl/ficllocal.h ficl/ficltokens.h
CBOBJECTS= cockbot.o discordEvents.o forthFunctions.o
CBHEADERS= cockbot.h discordEvents.h forthFucntions.h
HEADERS= $(FICLHEADERS) $(CBHEADERS)

all: cockbot libficl.so ficl-cli

cockbot: ficl $(CBOBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CBOBJECTS) libficl.a -o bot/cockbot -L. $(LIBRARIES)

ficl: libficl.a libficl.so

# static library build
libficl.a: $(FICLOBJECTS)
	$(AR) crs $@ $(FICLOBJECTS)

libficl.so: libficl.so.$(MAJOR).$(MINOR)
	ln -sf libficl.so.$(MAJOR).$(MINOR) libficl.so.$(MAJOR)
	ln -sf $< $@

ficl-cli: libficl.a
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) ficl-cli.c libficl.a -o ficl-cli -L. $(LIBRARIES)

# shared library build
libficl.so.$(MAJOR).$(MINOR): $(FICLOBJECTS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libficl.so.$(MAJOR).$(MINOR) -o $@ $(FICLOBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Generic cleanup code
clean:
	rm -f *.o *.a libficl.*
	rm -f ficl/*.o ficl/*.a ficl/libficl.* ficl/ficlplatform/*.o
	rm -rf bot/cockbot
