.POSIX:
OBJECTS= forthFunctions.o ficl/dictionary.o ficl/system.o ficl/fileaccess.o ficl/float.o ficl/double.o ficl/prefix.o ficl/search.o ficl/softcore.o ficl/stack.o ficl/tools.o ficl/vm.o ficl/primitives.o ficl/bit.o ficl/lzuncompress.o ficl/unix.o ficl/utility.o ficl/hash.o ficl/callback.o ficl/word.o ficl/extras.o
HEADERS= ficl/ficl.h ficl/ficlplatform/unix.h
#
# Flags for shared library
#TARGET= -Dlinux  # riscos MOTO_CPU32
SHFLAGS = -fPIC
SHUTUPFLAGS= -Wno-int-conversion
CFLAGS= -O $(SHFLAGS) $(SHUTUPFLAGS) \
				-w -Wall -g -Wpedantic -fsanitize=address \
				-I../extern/concord/include -I../extern/concord/core/ -I../extern/concord/gencodecs
CC=clang
CPPFLAGS= $(TARGET) -I.
LIB = ar crs

MAJOR = 4
MINOR = 1.0

.PHONY: all clean lib

all: ficl

ficl: lib cockbot.o forthFunctions.o discordEvents.o $(HEADERS) libficl.a
	$(CC) $(CFLAGS) $(LDFLAGS) discordEvents.o  forthFunctions.o cockbot.o -o bot/cockbot -L. -lficl -lm -lcurl -ldiscord -lpthread -L../extern/concord/lib -ldiscord

lib: ../extern/concord/lib/libdiscord.a libficl.so.$(MAJOR).$(MINOR)

# static library build
libficl.a: $(OBJECTS)
	$(LIB) libficl.a $(OBJECTS)

# shared library build
libficl.so.$(MAJOR).$(MINOR): $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libficl.so.$(MAJOR).$(MINOR) \
	-o libficl.so.$(MAJOR).$(MINOR) $(OBJECTS)
	ln -sf libficl.so.$(MAJOR).$(MINOR) libficl.so

.PHONY: ../extern/concord/lib/libdiscord.a
../extern/concord/lib/libdiscord.a:
	cd ../extern/concord/ && $(MAKE) -f Makefile
	cd ../extern/concord/ && $(MAKE) -f Makefile gencodecs

cockbot: discordEvents.o cockbot.o  forthFunctions.o ficl/ficl.h ficl/sysdep.h libficl.so.$(MAJOR).$(MINOR)
	$(CC) $(CFLAGS) $(LDFLAGS) cockbot.o discordEvents.o forthFunctions.o -o bot/cockbot -L. -lficl -lm -ldiscord -lcurl -lpthread
	ln -sf libficl.so.$(MAJOR).$(MINOR) libficl.so.$(MAJOR)


# depend explicitly to help finding source files in another subdirectory,
# and repeat commands since gmake doesn't understand otherwise
ficl/ansi.o: ficl/ficlplatform/ansi.c $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
ficl/unix.o: ficl/ficlplatform/unix.c $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

#
#       generic object code
#
.SUFFIXES: .cxx .cc .c .o

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

#
#       generic cleanup code
#
clean:
	rm -f *.o *.a libficl.*
	rm -f ficl/*.o ficl/*.a ficl/libficl.*
	rm -rf bot/cockbot
