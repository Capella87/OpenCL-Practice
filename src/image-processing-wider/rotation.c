#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include "rotation.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CHECK_ERROR(err) \
    if(err != CL_SUCCESS) { \
        printf("[%s:%d] OpenCL error %d\n", __FILE__, __LINE__, err); \
        exit(EXIT_FAILURE); \
    }

char* get_source_code(const char* file_name, size_t* len)
{
    FILE* file = fopen(file_name, "rb");
    if (file == (void*)0)
    {
        printf("[%s:%d] Failed to open %s\n", __FILE__, __LINE__, file_name);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    rewind(file);

    char* source_code = (char*)malloc(length + 1);
    fread(source_code, length, 1, file);
    source_code[length] = '\0';
    fclose(file);
    *len = length;

    return source_code;
}

void build_error(cl_program program, cl_device_id device, cl_int err)
{
    if (err == CL_BUILD_PROGRAM_FAILURE)
    {
        size_t log_size;
        char* log;

        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, (void*)0, &log_size);
        CHECK_ERROR(err);

        log = (char*)malloc(log_size + 1);
        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, (void*)0);
        CHECK_ERROR(err);

        log[log_size] = '\0';
        printf("Compiler error:\n%s\n", log);
        free(log);
        exit(0);
    };
}

void rotate(const double* input, double* output, const int width, const int height, const int bit, char* degree)
{
    const double theta = (double)(atof(degree) * M_PI / 180);
    const double sin_theta = (double)sinf(theta);
    const double cos_theta = (double)cosf(theta);

    cl_int err;

    // Platform ID
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, (void*)0);
    CHECK_ERROR(err);

    // Device ID
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, (void*)0);
    CHECK_ERROR(err);

    // Create Context
    cl_context context = clCreateContext((void*)0, 1, &device, (void*)0, (void*)0, &err);
    CHECK_ERROR(err);

    // Create Command Queue
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    CHECK_ERROR(err);

    // Create Program Object
    size_t kernel_source_size;
    char* kernel_source = get_source_code("kernel.cl", &kernel_source_size);
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_source_size, &err);
    CHECK_ERROR(err);

    // Build Program
    err = clBuildProgram(program, 1, &device, "", (void*)0, (void*)0);
    build_error(program, device, err);
    CHECK_ERROR(err);

    /*
     * 여기서부터 병렬 처리를 위한 호스트 코드를 작성하세요.
     */
    cl_kernel kernel = clCreateKernel(program, "rotate", &err);
    CHECK_ERROR(err);


    size_t size = (size_t)(width * height * sizeof(double));
    cl_mem input_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, size, (void*)0, &err);
    CHECK_ERROR(err);

    cl_mem output_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, size, (void*)0, &err);
    CHECK_ERROR(err);

    err = clEnqueueWriteBuffer(queue, input_clmem, CL_FALSE, 0, size, input, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    // Initial x0 and y0
    double x0 = width / 2.0f;
    double y0 = height / 2.0f;

    // Set kernel arguments

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_clmem);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_clmem);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 2, sizeof(cl_int), &width);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 3, sizeof(cl_int), &height);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 4, sizeof(cl_double), &x0);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 5, sizeof(cl_double), &y0);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 6, sizeof(cl_double), &sin_theta);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 7, sizeof(cl_double), &cos_theta);
    CHECK_ERROR(err);


    // Initialize global size and execute over the kernel

    // Warning
    size_t global_size[2] = { width, height };

    err = clEnqueueNDRangeKernel(queue, kernel, 2, (void*)0, global_size, (void*)0, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    err = clEnqueueReadBuffer(queue, output_clmem, CL_TRUE, 0, size, output, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);


    // Finish the work

    clFinish(queue);
    clReleaseMemObject(input_clmem);
    clReleaseMemObject(output_clmem);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}