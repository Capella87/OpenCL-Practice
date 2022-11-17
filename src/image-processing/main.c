#include "rotation.h"
#include "bmpfuncs.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s <src file> <dst file> <degree>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width, height;
    float* input = readImage(argv[1], &width, &height);
    float* output = (float*)malloc(sizeof(float) * width * height);

    rotate(input, output, width, height, argv[3]);
    storeImage(output, argv[2], height, width, argv[1]);

    return 0;
}