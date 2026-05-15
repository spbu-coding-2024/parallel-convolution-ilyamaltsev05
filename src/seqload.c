#include <stb_image.h>
#include <stb_image_write.h>
#include <stdio.h>

#define error(msg) fprintf(stderr, msg);

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

extern convolution_result *convolute(convolution *argument);

int main(int argc, char **argv)
{
    if ((argc - 1) % 3 != 0 || argc == 1)
    {
        error("bad usage, should be: input.bmp convolution.conv output.bmp ...\n");
        return -1;
    }

    convolution *all = calloc((argc - 1) / 3, sizeof(convolution));
    if (!all)
    {
        error("failed to allocate memory for input\n");
        return -2;
    }

    for (int i = 0; i < (argc - 1); i += 3)
    {
        convolution *curr = all + (i / 3);
        curr->pixels = stbi_load(argv[i + 1], &curr->x, &curr->y, &curr->channels, 0);
        if (curr->pixels == NULL)
        {
            error("failed to read pixels\n");
            continue;
        }

        FILE *convolution_file = fopen(argv[i + 2], "r");
        if (convolution_file == NULL)
        {
            error("failed to open file with convolution matrix\n");
            free(curr->pixels);
            curr->pixels = NULL;
            continue;
        }

        if (fscanf(convolution_file, "%d", &curr->convolution_size) != 1)
        {
            error("failed to read from convolution matrix file\n");
            free(curr->pixels);
            curr->pixels = NULL;
            fclose(convolution_file);
            continue;
        }

        curr->convolution_matrix = calloc(curr->convolution_size * curr->convolution_size, sizeof(int));
        int read_error = 0;
        for (int j = 0; j < curr->convolution_size * curr->convolution_size; j++)
        {
            if (fscanf(convolution_file, "%d", curr->convolution_matrix + j) != 1)
            {
                error("failed to read from convolution matrix file\n");
                read_error = 1;
                break;
            }
        }

        if (read_error)
        {
            free(curr->convolution_matrix);
            curr->convolution_matrix = NULL;
            free(curr->pixels);
            curr->pixels = NULL;
            fclose(convolution_file);
            continue;
        }

        fclose(convolution_file);
        curr->output_filename = argv[i + 3];
    }

    convolution_result **res = calloc((argc - 1) / 3, sizeof(convolution_result *));
    if (!res)
    {
        error("failed to allocate buffer for output\n");
        return -3;
    }

    for (int i = 0; i < (argc - 1); i += 3)
    {
        convolution *curr_input = all + (i / 3);
        if (!res)
        {
            free(curr_input->pixels);
            curr_input->pixels = NULL;
            free(curr_input->convolution_matrix);
            curr_input->convolution_matrix = NULL;
            continue;
        }
        if (curr_input->pixels && curr_input->convolution_matrix)
        {
            res[i / 3] = convolute(curr_input);
        }
        free(curr_input->pixels);
        curr_input->pixels = NULL;
        free(curr_input->convolution_matrix);
        curr_input->convolution_matrix = NULL;
    }

    free(all);

    for (int i = 0; i < (argc - 1); i += 3)
    {
        if (res)
        {
            convolution_result *outp = res[i / 3];
            if (outp && outp->pixels)
            {
                int code = stbi_write_bmp(outp->filename, outp->x, outp->y, outp->channels, outp->pixels);
                if (!code)
                {
                    error("failed write\n");
                }
                free(outp->pixels);
            }
            free(outp);
        }
    }

    free(res);

    return 0;
}
