#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "all_frames.c"

#define BOX_WIDTH 16
#define SEARCH_BOUNDARY 7

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
void estimate(const unsigned char *image1, const unsigned char *image2, float* mean_x, float *mean_y) {
  unsigned int total = (width * height), x2, y2, box_count;
  float total_x, total_y;
  int m, n, dy, dx, x1, y1,min_cost, curr_cost;
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
  
  *mean_x= total_x  / box_count; // other calculation can be done with this
  *mean_y= total_y  / box_count;
}

struct estimate_args_struct {
  const unsigned char *image1;
  const unsigned char *image2;
  float *mean_x;
  float *mean_y;
};

struct worker_arg_struct {
  struct estimate_args_struct * work_queue;
};

void *work(void *args) {
  printf("yup\n");
  struct estimate_args_struct * work_queue = args;    
  // printf("%f\n", *(work_queue[2].mean_x));
  return NULL;
}

int main(int argc, char **argv) {
  unsigned char *frame_1, *frame_2;
  int i, num_pthreads = 8;
  float sum_x = 0, sum_y = 0;

  float *mean_x_array = (float *)malloc(sizeof(float) * (num_frames - 1));
  float *mean_y_array = (float *)malloc(sizeof(float) * (num_frames - 1));
  if (argc > 1) {
    num_pthreads = atoi(argv[1]);
  }
  pthread_t tid[num_pthreads];

  struct estimate_args_struct * work_queue = malloc(num_frames * sizeof(struct estimate_args_struct));

  for (i = 1; i < num_frames; i++) {
    frame_1 = frames[i - 1];
    frame_2 = frames[i];
    struct estimate_args_struct estimate_args;
    estimate_args.image1 = frame_1;
    estimate_args.image2 = frame_2;
    estimate_args.mean_x = &mean_x_array[i];
    estimate_args.mean_y = &mean_y_array[i];
    
    work_queue[i - i] = estimate_args; 
    printf("%f\n", *(estimate_args.mean_x));
    printf("%d %f\n", i - 1, *(work_queue[i - 1].mean_x));
    printf("%d %f\n", i - 1, *(work_queue[i - 1].mean_x));
  }

  printf("here\n");
  for (i = 0; i < num_frames; i++) {
    printf("%d \n", i);
    printf("%d %f\n", i, *(work_queue[i].mean_x));
  }
  printf("yooo\n");

  for (i = 0; i < num_pthreads; i++) {
   printf("%d\n", i);
   pthread_create(&(tid[i]), NULL, &work, (void *) work_queue); 
  } 
  
  for (i = 0; i < num_pthreads; i++) {
    pthread_join(tid[i], NULL); 
  }
  
  /**
  for (i = 0; i < num_frames - 1; i++) {
    sum_x += mean_x_array[i];
  }
  for (i = 0; i < num_frames - 1; i++) {
    sum_y += mean_y_array[i];
  }
  printf("mean_x: %f, mean_y %f\n", sum_x / (num_frames - 1),
         sum_y / (num_frames - 1));
  */ 	

 
  free(mean_x_array);
  free(mean_y_array);

  return 0;
}
