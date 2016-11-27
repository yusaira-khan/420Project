

UNAME_S := $(shell uname -s)

LDFLAGS := -lavcodec    -lavformat -lavutil -lavfilter
CFLAGS :=  $(shell pkg-config --cflags libavformat libavcodec libavutil)

ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
endif

all: baseline pthread test

baseline:
	gcc -g video.c $(CFLAGS) $(LDFLAGS) -o estimate 

pthread:
	gcc -g video_pthread.c $(CFLAGS) $(LDFLAGS) -o estimate_pthread

test:
	gcc test.c $(CFLAGS) -o test

clean:
	rm test estimate estimate_pthread
