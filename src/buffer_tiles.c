#include "structs.h"
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

convolution_result *convolute(convolution *argument)
{
    convolution_result *res = malloc(sizeof(convolution_result));
    int convolution_size = argument->convolution_size;
    int channels = argument->channels;
    int x = argument->x;
    int y = argument->y;

    res->x = x;
    res->y = y;
    res->channels = channels;
    res->pixels = malloc(x * y * channels * sizeof(unsigned char));
    res->filename = argument->output_filename;

    if (res->pixels == NULL) {
        fprintf(stderr, "failed to allocate output buffer\n");
        free(res);
        return NULL;
    }

    int tile_x = 32;
    int tile_y = 32;

    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int ty = 0; ty < y; ty += tile_y)
    {
        for (int tx = 0; tx < x; tx += tile_x)
        {
            for (int y_offset = 0; y_offset < tile_y; y_offset++)
            {
                int row = ty + y_offset;
                if (row >= y) continue;
                for (int x_offset = 0; x_offset < tile_x; x_offset++)
                {
                    int col = tx + x_offset;
                    if (col >= x) continue;
                    int new_pixel[4] = {0};
                    for (int conv_y = 0; conv_y < convolution_size; conv_y++)
                    {
                        int cur_y = row - (convolution_size / 2) + conv_y;
                        if (cur_y < 0 || cur_y >= y)
                            continue;
                        for (int conv_x = 0; conv_x < convolution_size; conv_x++)
                        {
                            int cur_x = col - (convolution_size / 2) + conv_x;
                            if (cur_x < 0 || cur_x >= x)
                                continue;
                            for (int channel_i = 0; channel_i < channels; channel_i++)
                            {
                                new_pixel[channel_i] +=
                                argument->convolution_matrix[conv_y * convolution_size + conv_x] *
                                            argument->pixels[cur_y * x * channels +
                                            cur_x * channels + channel_i];
                            }
                        }
                    }
                    for (int i = 0; i < channels; i++)
                    {
                        if (new_pixel[i] < 0)
                            new_pixel[i] = 0;
                        if (new_pixel[i] > 255)
                            new_pixel[i] = 255;
                        res->pixels[row * x * channels + col * channels + i] =
                            (unsigned char)(new_pixel[i]);
                    }
                }
            }
        }
    }
    return res;
}
