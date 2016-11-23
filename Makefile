

UNAME_S := $(shell uname -s)

LDFLAGS := -lavcodec    -lavformat -lavutil -lavfilter
CFLAGS :=  $(shell pkg-config --cflags libavformat libavcodec libavutil)

ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
endif

all:
	gcc video.c $(CFLAGS) $(LDFLAGS) -o estimate 

test:
	gcc test.c $(CFLAGS) -o test

clean:
	rm test estimate