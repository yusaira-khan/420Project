#include "1.c"
#include "2.c"
#include <stdio.h>

#define BOX_WIDTH 16
#define SEARCH_BOUNDARY 7
unsigned int getMAD(unsigned char *image1, unsigned char *image2, int width,
                    int height, unsigned int x2, unsigned int y2,
                    unsigned int x1, unsigned int y1) {
  int i, j, m1, n1, m2, n2, diff;
  unsigned char im1, im2;
  unsigned int sum, MAD;
  sum = 0;

  for (i = 0; i < BOX_WIDTH; i++) {
    m1 = x1 + i;
    m2 = x2 + i;
    if (m1 < 0 || m2 < 0 || m1 >= width || m2 >= width) {
      printf("width out of bound x1 %d x2 %d\n", m1, m2);
      return 63557;
    }
    for (j = 0; j < BOX_WIDTH; j++) {

      n1 = y1 + j;
      n2 = y2 + j;

      if (n1 < 0 || n2 < 0 || n1 >= height || n2 >= height) {
        printf("height out of bound y1 %d y2 %d %d\n", n1, n2, height);
        return 63557;
      }
      // printf("Computing sum\n");

      im1 = image1[m1 * width + n1];
      im2 = image2[m2 * width + n2];
      diff = im1 - im2;
      if (diff < 0) {
        diff = -diff;
      }
      sum += diff;
      // printf("Computing sum %d \n", sum);
    }
  }

  MAD = sum / (BOX_WIDTH * BOX_WIDTH);
  // printf("%d\n",MAD );
  return MAD;
}

void estimate(unsigned char *image1, unsigned char *image2, int width,
              int height, float *mean_x, float *mean_y) {
  int computations = 0,
      peas = (2 * SEARCH_BOUNDARY + 1) * (2 * SEARCH_BOUNDARY + 1), x1, y1;
  unsigned int total = (width * height), x2, y2, min_cost, curr_cost, box_count,
               total_x, total_y, box_size;
  int m, n, dy, dx;
  box_size = (total / BOX_WIDTH / BOX_WIDTH * 2);
  // unsigned char* vectors = malloc( sizeof(char) * box_size);
  // unsigned int* costs = malloc( sizeof(char) * peas );
  box_count = 0;
  total_x = 0;
  total_y = 0;
  for (x2 = 0; x2 < width - BOX_WIDTH; x2 += BOX_WIDTH) {
    for (y2 = 0; y2 < height - BOX_WIDTH; y2 += BOX_WIDTH) {

      min_cost = 65537;
      dy = 0;
      dx = 0;

      for (m = -SEARCH_BOUNDARY; m < SEARCH_BOUNDARY; m++) {

        for (n = -SEARCH_BOUNDARY; n < SEARCH_BOUNDARY; n++) {
          x1 = x2 + m;
          y1 = y2 + n;
          if (x1 < 0 || y1 < 0 || x1 + BOX_WIDTH >= width ||
              y1 + BOX_WIDTH >= height) {
            printf(" Exiting width out of bound x1 %d x2 %d %d\n", x1, x2,
                   width);
            printf(" Exiting height out of bound y1 %d y2 %d %d\n", y1, y2,
                   height);

            continue;
            printf("still\n");
          }
          curr_cost = getMAD(image1, image2, width, height, x2, y2, x1, y1);
          // printf("%d %d\n",m,m< SEARCH_BOUNDARY);

          if (curr_cost < min_cost) {
            min_cost = curr_cost;
            dx = m;
            dy = n;
          }
          computations++;
        }
      }
      // printf("%s\n", );
      // vectors[box_count]=dx;
      // vectors[box_count+1]=dy;
      total_y += dy;
      total_x += dx;
      box_count++;
      printf("box_count %d box_size %d \n", box_count, box_size);
      // printf("min_cost %d box_count %d",min_cost,box_count);
    }
  }
  printf("Fin\n");
  *mean_y = total_y * 1.0 / box_count;
  *mean_x = total_x * 1.0 / box_count;
  // free(vectors);
}

int main() {
  float mean_x, mean_y;
  estimate(frame_1, frame_2, width_1, height_1, &mean_x, &mean_y);
  printf("%f %f\n", mean_x, mean_y);
}
