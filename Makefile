TARGET = Luminex
OBJS = luminex.o graphics.o framebuffer.o screenshot.o mp3player.o

CFLAGS = -O2 -G0 -Wall -fno-strict-aliasing
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgu -lpng -lz -lm -lpspaudiolib -lpspaudio -lmad -lpsppower -lpsprtc 
LDFLAGS =


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak