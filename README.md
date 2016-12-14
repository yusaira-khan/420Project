To run and calculate timings, run:
```
$ make
$ make run
```

If allframes.c is not present, only run:
```
$ make extract

```
To compile and run open mp implementation:
```
$ make extract
$ ./extract <video>
$ make omp
$ ./estimate_omp <num_threads>
```
To compile and run pthread implementation:
```
$ make extract
$ ./extract <video>
$ make pthread
$ ./estimate_pthread <num_threads>
```
To compile and run sequential implementation:
```
$ make extract
$ ./extract <video>
$ make pthread
$ ./estimate_pthread <num_threads>
```
To compile & run the CUDA implementation:

- SSH into the queens1 server
- Copy over baseline_cuda.cu, all_frames.h, videoplayback.mp4
- Run:
```
$ make extract
$ ./extract <video>
$ nvcc baseline_cuda.cu -I/usr/local/cuda-6.5/samples/0_Simple/simplePrintf/
$ ./a.out
```
*Note: Block & thread configurations can be changed inside baseline_cuda.cu
by changing the values of num_blocks & num_threads on line 110*
