#pragma once

#ifndef __BMPFUNCS__
#define __BMPFUNCS__

typedef unsigned char uchar;

double* readImage(const char* filename, int* widthOut, int* heightOut, uchar* bitsOut);
void storeImage(double* imageOut, const char* filename, int rows, int cols, int bit,
    const char* refFilename);
void rotate(const double* input, double* output, const int width, const int height, const int bit, char* degree);

#endif