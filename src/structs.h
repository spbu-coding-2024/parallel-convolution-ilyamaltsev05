#ifndef CONVOLUTION
#define CONVOLUTION

#include <glib.h>
#include <pthread.h>

typedef struct {
    int x;
    int y;
    int channels;
    int convolution_size;
    unsigned char *pixels;
    int *convolution_matrix;
    const char *output_filename;
} convolution;

typedef struct {
    int x;
    int y;
    int channels;
    unsigned char *pixels;
    const char *filename;
} convolution_result;

typedef struct {
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    GQueue *queue;
    int done;
} queue;

typedef struct {
    queue *to_convolute;
    queue *to_write;
} conv_args;

typedef struct {
    const int argc;
    int current;
    pthread_mutex_t current_mutex;
    char **argv;
    queue *to_convolute;
} reader_args;

typedef struct {
    queue *to_write;
} writer_args;

convolution_result *convolute(convolution *argument);

#endif
