#pragma once

#ifndef __BMPFUNCS__
#define __BMPFUNCS__

typedef unsigned char uchar;

float* readImage(const char* filename, int* widthOut, int* heightOut);
void storeImage(float* imageOut, const char* filename, int rows, int cols,
    const char* refFilename);
void rotate(const float* input, float* output, const int width, const int height, char* degree);

#endif