include config.mak

CFLAGS += -Wall -O2 -g -std=c17

# FFMPEG >>
CFLAGS += -I$(FFMPEG_SDK_DIR)/include
LDFLAGS += -L$(FFMPEG_SDK_DIR)/lib
LDLIBS += -lavformat -lavcodec -lswresample -lswscale -lavutil
# << FFMPEG

# NDI >>
CFLAGS += -I$(NDI_SDK_DIR)/include
LDFLAGS += -L$(NDI_LIB) -rpath $(NDI_LIB)
LDLIBS += -lndi
# << NDI

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))

.PHONY: all clean

all: build/ndi-streamer

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/ndi-streamer: $(OBJECTS)
	$(CC) -o build/ndi-streamer $(OBJECTS) $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(OBJECTS) bin/ndi-streamer
