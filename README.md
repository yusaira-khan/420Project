To run and calculate timings, run:
```
$ make
$ make run
```


If allframes.c is not present, only run:
```
$ make extract

```
To compile & run the CUDA implementation:

- SSH into the queens1 server
- Copy over baseline_cuda.cu, all_frames.h, videoplayback.mp4
- Run:
```
$ nvcc baseline_cuda.cu -I/usr/local/cuda-6.5/samples/0_Simple/simplePrintf/
$ ./a.out
```
*Note: Block & thread configurations can be changed inside baseline_cuda.cu
by changing the values of num_blocks & num_threads on line 110*
