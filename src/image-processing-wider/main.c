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
    uchar bits;
    double* input = readImage(argv[1], &width, &height, &bits);
    double* output = (double*)malloc(sizeof(double) * width * height);

    rotate(input, output, width, height, bits, argv[3]);
    storeImage(output, argv[2], height, width, bits, argv[1]);

    return 0;
}