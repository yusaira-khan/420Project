

/**
modified from
 * @file
 * API example for decoding andfiltering
 * @example filtering_video.c
 */

#define _XOPEN_SOURCE 600 /* for usleep */
// #include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>

#include <stdio.h>
#include <libswscale/swscale.h>

/*{
  unsigned error1, error2;
  unsigned char *image1, *image2;
  unsigned width1, height1, width2, height2;

  error1 = 0;//lodepng_decode32_file(&image1, &width1, &height1, input_filename_1);
  error2 = 0;//lodepng_decode32_file(&image2, &width2, &height2, input_filename_2);
  if(error1) printf("error %u: %s\n", error1, lodepng_error_text(error1));
  if(error2) printf("error %u: %s\n", error2, lodepng_error_text(error2));
  if(width1 != width2) printf("images do not have same width\n");
  if(height1 != height2) printf("images do not have same height\n");

  // process image
  float im1, im2, diff, sum, MSE;
  sum = 0;
  for (int i = 0; i < width1 * height1; i++) {
    im1 = (float)image1[i];
    im2 = (float)image2[i];
    diff = im1 - im2;
    sum += diff * diff;
  }
  MSE = sqrt(sum) / (width1 * height1);

  free(image1);
  free(image2);

  return MSE;
}
*/
const char *filter_descr = "scale=78:24,transpose=cclock";
/* other way:
   scale=78:24 [scl]; [scl] transpose=cclock // assumes "[in]" and "[out]" to be input output pads respectively
 */

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;
static struct SwsContext      *sws_ctx =NULL;



static int open_input_file(const char *filename)
{
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

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    dec_ctx = fmt_ctx->streams[video_stream_index]->codec;
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }
    printf("%d\n", dec_ctx->pix_fmt);
    av_dump_format(fmt_ctx, 0, filename, 0);
    return 0;
}

int gather_frame(AVFrame *frame){
    int ret,got_frame;
    AVPacket packet;
    if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
        return 0;
    if (packet.stream_index == video_stream_index) {
        got_frame = 0;
        ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
        }

        if (got_frame) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            // sws_scale(
            //     sws_ctx, (uint8_t const * const *)frame->data,
            //     frame->linesize, 0, dec_ctx->height,
            //     rgb_frame->data, rgb_frame->linesize);
            return 1;
        }
    }
    return 2;
}

unsigned char* grey_color(AVFrame *frame ){
    unsigned int total = (dec_ctx->width * dec_ctx->height), i;
    unsigned char* arr=NULL;
    arr = malloc( sizeof(char) * total);
    for (int y = 0; y < dec_ctx->height; y++){
        int yy=frame->linesize[0]*y;
        for (int x = 0; x < dec_ctx->width; x++){
            arr[i]=frame->data[0][yy+x]/2;
            // printf("(%d %d)",x,arr[i] );
            i++;
        }
        // printf("(%d %d)\n",y,i );
    }
    return arr;
}


int main(int argc, char **argv)
{
    int ret, first_turn=1;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    // AVFrame *rgb_frame = av_frame_alloc(); 
    AVFrame *frame_new = av_frame_alloc();
    AVFrame *frame_old = NULL;
    int got_frame;
    unsigned char * color_old, *color_new,*color_temp;

    if (!frame  ||!frame_new) {
        perror("Could not allocate frame");
        exit(1);
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        exit(1);
    }
    

    // sws_ctx =sws_getContext (
    //     dec_ctx->width, dec_ctx->height,
    //     dec_ctx->pix_fmt,
    //     dec_ctx->width, dec_ctx->height,
    //     2,// AVPixelFormat.PIX_FMT_RGB24,
    //     SWS_BILINEAR, NULL, NULL,  NULL);


    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    // if ((ret = init_filters(filter_descr)) < 0)
    //     goto end;

    /* read all packets */

    while (1) {
        got_frame = gather_frame(frame);
       // printf("FIRST\n");
        if (got_frame <1){//needs toexit
            break;
        }
        //printf("FIN\n");
        if(got_frame==2){//stream was different
            continue;
        }
                printf("FIN\n");

        //av_frame_unref(frame);
        color_old=grey_color(frame);
        break;
    }//get the 1st frame


    while (1) {
        got_frame = gather_frame(frame);
        

        if (got_frame <1){//needs toexit
            break;
        }
        if(got_frame==2){//stream was different
            continue;
        }

        color_new=grey_color(frame);

        color_temp = color_old;
        color_old = color_new;
        free(color_temp);
        av_frame_unref(frame);
    }
    //av_frame_unref(frame_new);
    free(color_new);
    end:
    sws_freeContext (sws_ctx);

    avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
   // av_frame_free(&rgb_frame);
    av_frame_free(&frame_new);


    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

    exit(0);
}