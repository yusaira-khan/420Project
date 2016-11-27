/**
modified from
 * @file
 * API example for decoding andfiltering
 * @example filtering_video.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>

#include <limits.h>
#include <pthread.h>
#include <stdio.h>

#define BOX_WIDTH 16
#define SEARCH_BOUNDARY 7
#define DEFAULT_NUM_PTHREADS 8

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static int video_stream_index = -1;

pthread_mutex_t lock;

static int open_input_file(const char *filename) {
  int ret;
  AVCodec *dec;

  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  // select the video stream
  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find a video stream in the input file\n");
    return ret;
  }
  video_stream_index = ret;
  dec_ctx = fmt_ctx->streams[video_stream_index]->codec;
  av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);

  // init the video decoder
  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
    return ret;
  }
  // printf("%d\n", dec_ctx->pix_fmt);
  av_dump_format(fmt_ctx, 0, filename, 0);
  return 0;
}

int gather_frame(AVFrame *frame) { // fetch a frame
  int ret, got_frame;
  AVPacket packet;

  if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
    return 0;
  if (packet.stream_index ==
      video_stream_index) { // check if this the sream we are reading from
    got_frame = 0;
    ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
    }

    if (got_frame) { // successsfully read stream
      return 1;
    }
  }
  return 2;
}

unsigned char *grey_color(AVFrame *frame) { // get grayscale color from a frame
  unsigned int total = (dec_ctx->width * dec_ctx->height), i = 0;
  unsigned char *arr = NULL;
  arr = malloc(sizeof(char) * total);

  for (int y = 0; y < dec_ctx->height; y++) { // for each vertical row
    int yy =
        frame->linesize[0] * y; // offset of next row stored inside linesize
    for (int x = 0; x < dec_ctx->width;
         x++) { // for each horizontal cell in row
      arr[i] = frame->data[0][yy + x] / 2;
      i++;
    }
  }
  return arr;
}

struct mad_arg_struct {
  const unsigned char *image1;
  const unsigned char *image2;
  int width;
  int height;
  unsigned int x2;
  unsigned int y2;
  unsigned int x1;
  unsigned int y1;
  int  m;
  int  n;  
  unsigned int* global_min_cost;
  int*  dx;
  int*  dy;  
};

// mean absolute value
// Note that this (obviously) doesn't return anything and instead mutates 'global' variables
void getMAD(void *args) {
  struct mad_arg_struct *mad_args = args;

  int i, j, m1, n1, m2, n2, diff;
  unsigned char im1, im2;
  unsigned int sum, MAD, cost;
  sum = 0;

  for (i = 0; i < BOX_WIDTH; i++) {
    m1 = mad_args->x1 + i;
    m2 = mad_args->x2 + i;
    if (m1 < 0 || m2 < 0 || m1 >= mad_args->height || m2 >= mad_args->height) {
      cost = 63557;
      break;
    }
    for (j = 0; j < BOX_WIDTH; j++) {

      n1 = mad_args->y1 + j;
      n2 = mad_args->y2 + j;
      if (n1 < 0 || n2 < 0 || n1 >= mad_args->width || n2 >= mad_args->width) {
        cost = 63557;
        break;
      }
      im1 = mad_args->image1[m1 + n1 * mad_args->width];
      im2 = mad_args->image2[m2 + mad_args->width * n2];
      diff = im1 - im2;
      if (diff < 0) {
        diff = -diff;
      }
      sum += diff;
    }
  }
  cost = sum / (BOX_WIDTH * BOX_WIDTH);
  
  pthread_mutex_lock(&lock);
  if (cost < *(mad_args->global_min_cost)) {
    *(mad_args->global_min_cost) = cost;
    *(mad_args->dx) = mad_args->m;
    *(mad_args->dy) = mad_args->n;
  }
  pthread_mutex_unlock(&lock);
}

// Use exhaustive search Block Matching Motion Estimation algorithm
void estimate(const unsigned char *image1, const unsigned char *image2,
              int width, int height, float *mean_x, float *mean_y) {
  unsigned int total = (width * height), x2, y2, min_cost, curr_cost, box_count,
               total_x, total_y, box_size;
  int x1, y1, m, n, dx, dy;
  box_size = (total / BOX_WIDTH / BOX_WIDTH * 2);
  unsigned char *vectors = malloc(sizeof(char) * box_size);
  box_count = 0;
  total_x = 0;
  total_y = 0;
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

          struct mad_arg_struct mad_args;
          mad_args.image1 = image1;
          mad_args.image2 = image2;
          mad_args.width = width;
          mad_args.height = height;
          mad_args.x2 = x2;
          mad_args.y2 = y2;
          mad_args.x1 = x1;
          mad_args.y1 = y1;
          mad_args.m = m;
          mad_args.n = n;
          mad_args.global_min_cost = &min_cost;
          mad_args.dy = &dy;
          mad_args.dx = &dx;

          getMAD((void *) &mad_args);  // this will update min_cost, dy, and dx
        }
      }
      if (min_cost < 65537) {
        total_y += dy;
        total_x += dx;
        vectors[2 * box_count] = dx;
        vectors[2 * box_count + 1] = dy;
        box_count++;
      }
    }
  }
  *mean_y = total_y * 1.0 / box_count;
  *mean_x = total_x * 1.0 / box_count;
  free(vectors); // other calculation can be done with this
}

// Save frams byte in a .c file. to generate tests
void save_frame(unsigned char *potato, int num, int width, int height) {
  int i = 0;
  FILE *file;
  char args[10];
  snprintf(args, sizeof(args), "%d.c", num);
  file = fopen(args, "w+");

  fprintf(file, "int width_%d = %d, height_%d = %d;\n", num, width, num,
          height);

  fprintf(file, "unsigned char frame_%d[] = {\n\t%d", num, potato[0]);
  for (i = 1; i < width * height; i++) {
    fprintf(file, ",\n%d\t%d", num, potato[i]);
  }

  fprintf(file, "\n};\n");
  fclose(file);
}

int main(int argc, char **argv) {
  int ret, first_turn = 1, num_frames = 0;
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();

  int got_frame;
  unsigned char *color_old, *color_new, *color_temp;
  float mean_x, mean_y, total_x = 0, total_y = 0;

  if (!frame) {
    perror("Could not allocate frame");
    exit(1);
  }
  if (argc != 2 && argc != 3) {
    fprintf(stderr, "Usage: %s file [NUM_PTHREADS]\n", argv[0]);
    exit(1);
  }

  av_register_all();
  avfilter_register_all();

  if ((ret = open_input_file(argv[1])) < 0) {
    goto end;
  }

  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Pthread mutex initiation failed\n"); 
    goto end;
  }

  while (1) {
    got_frame = gather_frame(frame);
    if (got_frame < 1) { // needs toexit
      break;
    }
    if (got_frame == 2) { // stream was different
      continue;
    }
    if (first_turn) {
      color_old = grey_color(frame);
      av_frame_unref(frame);
      first_turn = 0;
      num_frames++;

      continue;
    }
    color_new = grey_color(frame);
    av_frame_unref(frame);

    estimate(color_old, color_new, dec_ctx->width, dec_ctx->height, &mean_x,
             &mean_y);
    num_frames++;
    total_x += mean_x;
    total_y += mean_y;

    color_temp = color_old; // swap nw and old frames
    color_old = color_new;
    free(color_temp);
  }

  // av_frame_unref(frame_new);
  if (num_frames > 1) {

    mean_x = total_x / num_frames;
    mean_y = total_y / num_frames;

    printf("Gather information from %d frames\n", num_frames);
    printf("Mean Motion Vector\n \tX:\t %f\n \tY: \t%f", mean_x, mean_y);

    free(color_new);
  } else {
    printf("Whoops!\n");
  }
end:

  avcodec_close(dec_ctx);
  avformat_close_input(&fmt_ctx);
  av_frame_free(&frame);
  pthread_mutex_destroy(&lock);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
    exit(1);
  }

  exit(0);
}
