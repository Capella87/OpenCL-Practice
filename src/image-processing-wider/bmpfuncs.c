#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "bmpfuncs.h"

void storeImage(double* imageOut, const char* filename, int rows, int cols, int bits, const char* refFilename)
{

    FILE* ifp, * ofp;
    unsigned char tmp;
    int offset;
    unsigned char* buffer;
    int i, j;

    size_t bytes;

    int height, width, bit;

    ifp = fopen(refFilename, "rb");
    if (ifp == (void*)0)
    {
        perror(filename);
        exit(-1);
    }

    fseek(ifp, 10, SEEK_SET);
    fread(&offset, 4, 1, ifp);

    fseek(ifp, 18, SEEK_SET);
    fread(&width, 4, 1, ifp);
    fread(&height, 4, 1, ifp);

    fseek(ifp, 28, SEEK_SET);
    fread(&bit, sizeof(unsigned char), 1, ifp);

    fseek(ifp, 0, SEEK_SET);

    buffer = (unsigned char*)malloc(offset);
    if (buffer == (void*)0)
    {
        perror("malloc");
        exit(-1);
    }

    fread(buffer, 1, offset, ifp);

    printf("Writing output image to %s\n", filename);
    ofp = fopen(filename, "wb");
    if (ofp == (void*)0)
    {
        perror("opening output file");
        exit(-1);
    }
    bytes = fwrite(buffer, 1, offset, ofp);
    if (bytes != offset)
    {
        printf("error writing header!\n");
        exit(-1);
    }

    // NOTE bmp formats store data in reverse raster order (see comment in
    // readImage function), so we need to flip it upside down here.  
    int mod = width % 4;
    if (mod != 0)
        mod = 4 - mod;

    unsigned int temp;
    int color_bytes = bits / 8;
    //   printf("mod = %d\n", mod);
    for (i = height - 1; i >= 0; i--)
    {
        for (j = 0; j < width; j++)
        {
            temp = (unsigned int)imageOut[i * cols + j];
            for (int k = 0; k < color_bytes; k++)
            {
                temp &= 255;
                unsigned char value = (unsigned char)temp;
                fwrite(&value, sizeof(unsigned char), 1, ofp);
                temp >>= 8;
            }
            // tmp = (unsigned char)imageOut[i * cols + j];
            // fwrite(&tmp, sizeof(char), 1, ofp);
        }
        // In bmp format, rows must be a multiple of 4-bytes.  
        // So if we're not at a multiple of 4, add junk padding.
        for (j = 0; j < mod; j++)
            fwrite(&tmp, sizeof(char), 1, ofp);
    }

    fclose(ofp);
    fclose(ifp);

    free(buffer);
}

/*
* Read bmp image and convert to byte array. Also output the width and height
완료
*/
double* readImage(const char* filename, int* widthOut, int* heightOut, uchar* bitsOut)
{
    unsigned int* imageData;

    int height, width;
    uchar bits;
    uchar tmp;
    int offset;
    unsigned int i_temp;
    int i, j;

    printf("Reading input image from %s\n", filename);
    FILE* fp = fopen(filename, "rb");
    if (fp == (void*)0)
    {
        perror(filename);
        exit(-1);
    }

    fseek(fp, 10, SEEK_SET);
    fread(&offset, 4, 1, fp);

    fseek(fp, 18, SEEK_SET);
    fread(&width, 4, 1, fp);
    fread(&height, 4, 1, fp);

    fseek(fp, 28, SEEK_SET);
    fread(&bits, 2, 1, fp);

    printf("width = %d\n", width);
    printf("height = %d\n", height);
    printf("bit = %d\n", bits);

    *widthOut = width;
    *heightOut = height;
    *bitsOut = bits;

    imageData = (unsigned int*)malloc(width * height * sizeof(unsigned int));
    if (imageData == (void*)0)
    {
        perror("malloc");
        exit(-1);
    }

    fseek(fp, offset, SEEK_SET);
    fflush((void*)0);

    int bytes = bits / 8;
    int mod = width % 4;
    if (mod != 0)
        mod = 4 - mod;

    // NOTE bitmaps are stored in upside-down raster order.  So we begin
    // reading from the bottom left pixel, then going from left-to-right, 
    // read from the bottom to the top of the image.  For image analysis, 
    // we want the image to be right-side up, so we'll modify it here.

    // First we read the image in upside-down

    // Read in the actual image
    for (i = 0; i < height; i++)
    {

        // add actual data to the image
        for (j = 0; j < width; j++)
        {
            i_temp = 0x0;
            for (int k = 0; k < bytes; k++)
            {
                fread(&tmp, sizeof(char), 1, fp);
                i_temp = (i_temp << 8) | tmp;
            }
            imageData[i * width + j] = i_temp;
        }
        // For the bmp format, each row has to be a multiple of 4, 
        // so I need to read in the junk data and throw it away
        for (j = 0; j < mod; j++)
            fread(&tmp, sizeof(char), 1, fp);
    }

    // Then we flip it over
    int flipRow;
    int iter = height / 2;
    for (i = 0; i < iter; i++)
    {
        flipRow = height - (i + 1);
        for (j = 0; j < width; j++)
        {
            i_temp = imageData[i * width + j];
            imageData[i * width + j] = imageData[flipRow * width + j];
            imageData[flipRow * width + j] = i_temp;
        }
    }

    fclose(fp);

    // Input image on the host
    double* doubleImage = (void*)0;
    doubleImage = (double*)malloc(sizeof(double) * width * height);
    if (doubleImage == (void*)0)
    {
        perror("malloc");
        exit(-1);
    }

    // Convert the BMP image to float (not required)
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
            doubleImage[i * width + j] = (double)imageData[i * width + j];

    free(imageData);
    return doubleImage;
}