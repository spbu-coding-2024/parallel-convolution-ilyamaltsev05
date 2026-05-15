#include <stb_image.h>
#include <stb_image_write.h>
#include "structs.h"

#define error(message) fprintf(stderr, message)

#define READER_THREADS 1

#define CONVOLUTE_THREADS 1

#define WRITER_THREADS 1

enum {
    OKAY,
    INVALID_NUMBER_OF_ARGUMENTS,
};

void *reader_function(void *argument)
{
    reader_args *rarg = argument;
    int curr_argc;
    while (1)
    {
        pthread_mutex_lock(&rarg->current_mutex);
        curr_argc = rarg->current++;
        pthread_mutex_unlock(&rarg->current_mutex);

        if (curr_argc * 3 + 1 >= rarg->argc)
          break;

        const char *input = rarg->argv[curr_argc * 3 + 1];
        const char *conv = rarg->argv[curr_argc * 3 + 2];
        const char *output = rarg->argv[curr_argc * 3 + 3];

        convolution *work = malloc(sizeof(convolution));

        work->pixels = stbi_load(input, &work->x, &work->y, &work->channels, 0);
        if (work->pixels == NULL)
        {
            error("failed to get image\n");
            free(work);
            continue;
        }

        FILE *convolution_file = fopen(conv, "r");
        if (convolution_file == NULL)
        {
            error("failed to open file with convolution matrix\n");
            free(work->pixels);
            free(work);
            continue;
        }

        if (fscanf(convolution_file, "%d", &work->convolution_size) != 1)
        {
            error("failed to read from convolution matrix file\n");
            free(work->pixels);
            free(work);
            fclose(convolution_file);
            continue;
        }

        work->convolution_matrix = malloc(work->convolution_size * work->convolution_size * sizeof(int));
        int read_error = 0;
        for (int i = 0; i < work->convolution_size * work->convolution_size; i++)
        {
            if (fscanf(convolution_file, "%d", work->convolution_matrix + i) != 1)
            {
                error("failed to read from convolution matrix file\n");
                read_error = 1;
                break;
            }
        }

        if (read_error)
        {
            free(work->convolution_matrix);
            free(work->pixels);
            free(work);
            fclose(convolution_file);
            continue;
        }

        fclose(convolution_file);

        work->output_filename = output;

        pthread_mutex_lock(&rarg->to_convolute->queue_mutex);
        g_queue_push_tail(rarg->to_convolute->queue, work);
        pthread_cond_signal(&rarg->to_convolute->queue_not_empty);
        pthread_mutex_unlock(&rarg->to_convolute->queue_mutex);
    }

    return NULL;
}

void *convoluter_function(void *argument)
{
    conv_args *carg = argument;

    while (1)
    {
        pthread_mutex_lock(&carg->to_convolute->queue_mutex);

        while (g_queue_is_empty(carg->to_convolute->queue) && !carg->to_convolute->done)
        {
            pthread_cond_wait(&carg->to_convolute->queue_not_empty,
                            &carg->to_convolute->queue_mutex);
        }

        if (g_queue_is_empty(carg->to_convolute->queue) && carg->to_convolute->done)
        {
            pthread_mutex_unlock(&carg->to_convolute->queue_mutex);
            break;
        }

        convolution *work = g_queue_pop_head(carg->to_convolute->queue);
        pthread_mutex_unlock(&carg->to_convolute->queue_mutex);

        convolution_result *res = convolute(work);

        free(work->convolution_matrix);
        free(work->pixels);
        free(work);

        if (res == NULL)
        {
            continue;
        }

        pthread_mutex_lock(&carg->to_write->queue_mutex);
        g_queue_push_tail(carg->to_write->queue, res);
        pthread_cond_signal(&carg->to_write->queue_not_empty);
        pthread_mutex_unlock(&carg->to_write->queue_mutex);
    }

    return NULL;
}

void *writer_function(void *argument) {
    writer_args *warg = argument;

    while (1)
    {
        pthread_mutex_lock(&warg->to_write->queue_mutex);

        while (g_queue_is_empty(warg->to_write->queue) && !warg->to_write->done)
        {
            pthread_cond_wait(&warg->to_write->queue_not_empty, &warg->to_write->queue_mutex);
        }

        if (g_queue_is_empty(warg->to_write->queue) && warg->to_write->done)
        {
            pthread_mutex_unlock(&warg->to_write->queue_mutex);
            break;
        }

        convolution_result *res = g_queue_pop_head(warg->to_write->queue);
        pthread_mutex_unlock(&warg->to_write->queue_mutex);

        int write_code = stbi_write_bmp(res->filename, res->x, res->y, res->channels,
                        res->pixels);

        if (!write_code)
        {
            error("write error\n");
        }
        free(res->pixels);
        free(res);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    if ((argc - 1) % 3 != 0 || argc == 1)
    {
        error("invalid number of arguments provided\n");
        return INVALID_NUMBER_OF_ARGUMENTS;
    }

    pthread_t readers[READER_THREADS];
    pthread_t convoluters[CONVOLUTE_THREADS];
    pthread_t writers[WRITER_THREADS];

    queue to_convolute = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, 0};
    to_convolute.queue = g_queue_new();

    queue to_write = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, 0};
    to_write.queue = g_queue_new();

    reader_args rargs = {argc, 0, PTHREAD_MUTEX_INITIALIZER, argv,
                         &to_convolute};

    for (int i = 0; i < READER_THREADS; i++)
      pthread_create(readers + i, NULL, &reader_function, &rargs);

    conv_args cargs = {&to_convolute, &to_write};

    for (int i = 0; i < CONVOLUTE_THREADS; i++)
      pthread_create(convoluters + i, NULL, &convoluter_function, &cargs);

    writer_args wargs = {&to_write};

    for (int i = 0; i < WRITER_THREADS; i++)
      pthread_create(writers + i, NULL, &writer_function, &wargs);

    for (int i = 0; i < READER_THREADS; i++)
      pthread_join(readers[i], NULL);

    pthread_mutex_lock(&to_convolute.queue_mutex);
    to_convolute.done = 1;
    pthread_cond_broadcast(&to_convolute.queue_not_empty);
    pthread_mutex_unlock(&to_convolute.queue_mutex);

    for (int i = 0; i < CONVOLUTE_THREADS; i++)
      pthread_join(convoluters[i], NULL);

    pthread_mutex_lock(&to_write.queue_mutex);
    to_write.done = 1;
    pthread_cond_broadcast(&to_write.queue_not_empty);
    pthread_mutex_unlock(&to_write.queue_mutex);

    for (int i = 0; i < WRITER_THREADS; i++)
      pthread_join(writers[i], NULL);

    g_queue_free(to_convolute.queue);
    g_queue_free(to_write.queue);

    return OKAY;
}
