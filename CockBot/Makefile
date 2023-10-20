OBJECTS= forthFunctions.o ficl/dictionary.o ficl/system.o ficl/fileaccess.o ficl/float.o ficl/double.o ficl/prefix.o ficl/search.o ficl/softcore.o ficl/stack.o ficl/tools.o ficl/vm.o ficl/primitives.o ficl/bit.o ficl/lzuncompress.o ficl/unix.o ficl/utility.o ficl/hash.o ficl/callback.o ficl/word.o ficl/extras.o
HEADERS= ficl/ficl.h ficl/ficlplatform/unix.h 
#
# Flags for shared library
#TARGET= -Dlinux  # riscos MOTO_CPU32 
SHFLAGS = -fPIC
CFLAGS= -O $(SHFLAGS) -w -Wall -g -fsanitize=address
CPPFLAGS= $(TARGET) -I.
CC = clang
LIB = ar cr
RANLIB = ranlib

MAJOR = 4
MINOR = 1.0

ficl: forthBlocks.o cockbot.o forthFunctions.o discordEvents.o $(HEADERS) libficl.a
	$(CC) $(CFLAGS) $(LDFLAGS) discordEvents.o forthBlocks.o forthFunctions.o cockbot.o -o bot/cockbot -L. -lficl -lm -lcurl -ldiscord -lpthread

lib: libficl.so.$(MAJOR).$(MINOR)

# static library build
libficl.a: $(OBJECTS)
	$(LIB) libficl.a $(OBJECTS)
	$(RANLIB) libficl.a

# shared library build
libficl.so.$(MAJOR).$(MINOR): $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libficl.so.$(MAJOR).$(MINOR) \
	-o libficl.so.$(MAJOR).$(MINOR) $(OBJECTS)
	ln -sf libficl.so.$(MAJOR).$(MINOR) libficl.so

cockbot: discordEvents.o cockbot.o forthBlocks.o forthFunctions.o ficl.h sysdep.h libficl.so.$(MAJOR).$(MINOR)
	$(CC) $(CFLAGS) $(LDFLAGS) cockbot.o forthBlocks.o discordEvents.o forthFunctions.o -o bot/cockbot -L. -lficl -lm -ldiscord -lcurl -lpthread
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

.cxx.o:
	$(CPP) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

.cc.o:
	$(CPP) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
#
#       generic cleanup code
#
clean:
	rm -f *.o *.a libficl.* 
	rm -f ficl/*.o ficl/*.a ficl/libficl.* 
	rm -rf bot/cockbot
