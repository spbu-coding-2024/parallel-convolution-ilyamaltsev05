#include <stb_image.h>
#include <stb_image_write.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <time.h>

#define NTHREADS 4

void generate_grid(int *grid, int x, int y)
{
    srand(time(NULL));
    for (int i = 0; i < y; i++)
    {
        for (int j = 0; j < x; j++)
        {
            grid[i * x + j] = (rand() % NTHREADS);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "incorrect number of command line arguments\n");
        return -1;
    }

    int x, y, channels;
    unsigned char *data = stbi_load(argv[1], &x, &y, &channels, 0);
    if (data == NULL)
    {
        fprintf(stderr, "failed to load input image\n");
        return 1;
    }

    FILE *convolution_file = fopen(argv[3], "r");
    if (convolution_file == NULL)
    {
        fprintf(stderr, "failed to open file with convolution matrix\n");
        free(data);
        return -2;
    }
    int convolution_size;
    if (fscanf(convolution_file, "%d", &convolution_size) != 1)
    {
        fprintf(stderr, "failed to read from convolution matrix file\n");
        free(data);
        fclose(convolution_file);
        return -3;
    }
    int convolution[convolution_size * convolution_size];
    for (int i = 0; i < convolution_size * convolution_size; i++)
    {
        if (fscanf(convolution_file, "%d", convolution + i) != 1)
        {
            fprintf(stderr, "failed to read from convolution matrix file\n");
            free(data);
            fclose(convolution_file);
            return -3;
        }
    }

    fclose(convolution_file);

    unsigned char *new_data = malloc(sizeof(unsigned char) * x * y * channels);
    if (new_data == NULL)
    {
        fprintf(stderr, "failed to allocate output buffer\n");
        free(data);
        return -11;
    }

    int *grid = malloc(sizeof(int) * x * y);
    if (grid == NULL)
    {
        fprintf(stderr, "failed to allocate grid\n");
        free(data);
        free(new_data);
        return -11;
    }
    generate_grid(grid, x, y);

    #pragma omp parallel num_threads(NTHREADS)
    {
        int thread_num = omp_get_thread_num();
        for (int row = 0; row < y; row++)
        {
            for (int col = 0; col < x; col++)
            {
                if (grid[row * x + col] != thread_num)
                    continue;
                int new_pixel[4] = {0};
                for (int conv_y = 0; conv_y < convolution_size; conv_y++)
                {
                    int i = row - (convolution_size / 2) + conv_y;
                    if (i < 0 || i >= y)
                        continue;
                    for (int conv_x = 0; conv_x < convolution_size; conv_x++)
                    {
                        int j = col - (convolution_size / 2) + conv_x;
                        if (j < 0 || j >= x)
                            continue;
                        for (int channel_i = 0; channel_i < channels; channel_i++)
                        {
                            new_pixel[channel_i] += convolution[conv_y * convolution_size + conv_x] *
                                                    data[i * x * channels + j * channels + channel_i];
                        }
                    }
                }
                for (int i = 0; i < channels; i++)
                {
                    if (new_pixel[i] < 0)
                        new_pixel[i] = 0;
                    if (new_pixel[i] > 255)
                        new_pixel[i] = 255;
                    new_data[row * x * channels + col * channels + i] = (unsigned char)(new_pixel[i]);
                }
            }
        }
    }

    int write_code = stbi_write_bmp(argv[2], x, y, channels, new_data);

    free(data);
    free(new_data);
    free(grid);

    if (!write_code)
    {
        fprintf(stderr, "write failed\n");
        return 2;
    }
    return 0;
}
