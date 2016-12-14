

UNAME_S := $(shell uname -s)

LD_VIDEO_FLAGS := -lavcodec    -lavformat -lavutil -lavfilter
VIDEO_FLAGS :=  $(shell pkg-config --cflags libavformat libavcodec libavutil)
NO_FLAGS := -Wall

ifeq ($(UNAME_S),Darwin)
    VIDEO_FLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
    NO_FLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/

endif

all: baseline omp pthread

extract:
	gcc -g extract.c $(VIDEO_FLAGS) $(LD_VIDEO_FLAGS) -o extract 
	rm  -f all_frames.c && time ./extract videoplayback.mp4 && 
omp:
	time gcc -g openmp_video.c $(NO_FLAGS) -fopenmp -o estimate_omp
run_omp:
	time ./estimate_omp 2
pthread:
	gcc -g pthread_video.c $(NO_FLAGS) -o estimate_pthread

baseline:
	time gcc -g baseline.c $(NO_FLAGS) -o estimate_baseline
cuda:
	time nvcc baseline_cuda.cu -o estimate_cuda
run_cuda:
	time ./estimate_cuda
run_baseline:
	time ./estimate_baseline
run: run_baseline run_omp run_cuda

clean:
	rm  -f estimate estimate_pthread estimate_omp extract all_frames.c
