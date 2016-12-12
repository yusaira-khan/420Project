#include "all_frames.h"
#include <stdio.h>
#include <stdlib.h>

#define BOX_WIDTH 16
#define SEARCH_BOUNDARY 7

__global__ void sum(float * d_x, float * d_y, float * dans, int num_frames) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	printf("%i", idx);
	
	dans[0] += d_x[idx];
	dans[1] += d_y[idx];
	
	__syncthreads();
}

unsigned int getMAD(const unsigned char *image1, const unsigned char *image2,
                    int width, int height, unsigned int x2, unsigned int y2,
                    unsigned int x1, unsigned int y1) {
  int i, j, m1, n1, m2, n2, diff;
  unsigned char im1, im2;
  unsigned int sum, MAD;
  sum = 0;

  for (i = 0; i < BOX_WIDTH; i++) {
    m1 = x1 + i;
    m2 = x2 + i;
    if (m1 < 0 || m2 < 0 || m1 >= height || m2 >= height) {
      return 63557;
    }
    for (j = 0; j < BOX_WIDTH; j++) {

      n1 = y1 + j;
      n2 = y2 + j;
      if (n1 < 0 || n2 < 0 || n1 >= width || n2 >= width) {
        return 63557;
      }
      im1 = image1[m1 + n1 * width];
      im2 = image2[m2 + width * n2];
      diff = im1 - im2;
      if (diff < 0) {
        diff = -diff;
      }
      sum += diff;
    }
  }
  MAD = sum / (BOX_WIDTH * BOX_WIDTH);
  // printf("%d\n",MAD );
  return MAD;
}

// Use exhaustive search Block Matching Motion Estimation algorithm
void estimate(const unsigned char *image1, const unsigned char *image2,
              float *mean_x, float *mean_y) {
  unsigned int  x2, y2, box_count;
  int width = 480, height = 360;
  float total_x, total_y;
  int m, n, dy, dx, x1, y1, min_cost, curr_cost;
  box_count = 1;
  for (y2 = 0; y2 < height - BOX_WIDTH; y2 += BOX_WIDTH) {
    for (x2 = 0; x2 < width - BOX_WIDTH; x2 += BOX_WIDTH) {

      min_cost = 65537;
      dy = 0;
      dx = 0;

      for (m = -SEARCH_BOUNDARY; m < SEARCH_BOUNDARY; m++) {
        for (n = -SEARCH_BOUNDARY; n < SEARCH_BOUNDARY; n++) {
          x1 = x2 + m;
          y1 = y2 + n;
          if (x1 < 0 || y1 < 0 || x1 + BOX_WIDTH >= width ||
              y1 + BOX_WIDTH >= height) { // dont execute if out f bounds
            continue;
          }
          curr_cost = getMAD(image1, image2, width, height, x2, y2, x1, y1);
          if (curr_cost < min_cost) { // calculate minimum cost
            min_cost = curr_cost;
            dx = m;
            dy = n;
          }
        }
      }
      if (min_cost >= 0 && min_cost < 65537) {
        total_y += dy;
        total_x += dx;
        box_count++;
        // printf("y2: %d, x2: %d, box: %d\n",y2,x2,box_count );
      }
    }
  }

  *mean_x = total_x / box_count; // other calculation can be done with this
  *mean_y = total_y / box_count;
}

int main(int argc, char **argv) {
  unsigned char *frame_1, *frame_2;
  int i;
  const int FRAME_SIZE = sizeof(float) * (num_frames - 1);

  float *mean_x_array = (float *)malloc(sizeof(float) * (num_frames - 1));
  float *mean_y_array = (float *)malloc(sizeof(float) * (num_frames - 1)); 
  float *ans = (float *)malloc(sizeof(float) * 2);
  
  float *d_x;
  float *d_y;
  float *dans;
  
  
  cudaMalloc(&d_x, FRAME_SIZE);
  cudaMalloc(&d_y, FRAME_SIZE);
  cudaMalloc(&dans, sizeof(float)*2);
  printf("%f \n", num_frames);
  
  for (i = 1; i < num_frames; i++) {
    frame_1 = frames[i - 1];
	printf("%c \n", frame_1);
    frame_2 = frames[i];
    estimate(frame_1, frame_2, &mean_x_array[i], &mean_y_array[i]);
  }
  
  cudaMemcpy(d_x, mean_x_array, FRAME_SIZE, cudaMemcpyHostToDevice);
  cudaMemcpy(d_y, mean_y_array, FRAME_SIZE, cudaMemcpyHostToDevice);
  cudaMemcpy(dans, ans, sizeof(float)*2, cudaMemcpyHostToDevice);
  
  sum<<<(num_frames - 1)/512, 512>>>(d_x, d_y, dans, num_frames);
  //sum<<<65535, (num_frames-1)/65535>>>(d_x, d_y, dans, num_frames);
  //sum<<<(num_frames - 1)/256, 265>>>(d_x, d_y, dans, num_frames);
  //sum<<<(num_frames - 1)/128, 128>>>(d_x, d_y, dans, num_frames);
  //sum<<<(num_frames - 1)/384, 384>>>(d_x, d_y, dans, num_frames);
  
  cudaMemcpy(dans, ans, sizeof(float)*2, cudaMemcpyDeviceToHost);
  cudaFree(d_x);
  cudaFree(d_y);
  
  printf("mean_x: %f, mean_y %f\n", ans[0] / (num_frames - 1), ans[1] / (num_frames - 1));
  
  free(mean_x_array);
  free(mean_y_array);

  return 0;
}


