#include "structs.h"
#include <stdlib.h>
#include <stdio.h>

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
        fprintf(stderr, "failed to allocate output buffer");
        free(res);
        return NULL;
    }

    int pixel_shared = 0;
    #pragma omp parallel
    {
        int pixel;
        while (1)
        {
            #pragma omp atomic capture
            {
                pixel = pixel_shared;
                pixel_shared++;
            }
            if (pixel >= x * y)
                break;
            int new_pixel[4] = {0};
            int row = pixel / x;
            int col = pixel % x;
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
                        new_pixel[channel_i] += argument->convolution_matrix[conv_y * convolution_size + conv_x] *
                                                argument->pixels[cur_y * x * channels + cur_x * channels + channel_i];
                    }
                }
            }
            for (int i = 0; i < channels; i++)
            {
                if (new_pixel[i] < 0)
                    new_pixel[i] = 0;
                if (new_pixel[i] > 255)
                    new_pixel[i] = 255;
                res->pixels[row * x * channels + col * channels + i] = (unsigned char)(new_pixel[i]);
            }
        }
    }
    return res;
}
