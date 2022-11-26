#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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

// Time measurement using OpenCL event profiling
void show_profiling_info(cl_event* kernel_event, cl_event* read_event, int event_count)
{ 
    cl_ulong start, end, result = 0ll;

    for (int i = 0; i < event_count; i++)
    {
        clGetEventProfilingInfo(kernel_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, (void*)0);
        clGetEventProfilingInfo(kernel_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, (void*)0);
        printf("Started : %llu\tEnded: %llu\n", start, end);
        result = end - start;
        printf("Elapsed Time of Kernel Event %d : %llu ns (%llu ms)\n\n", i + 1, result, result / 1000000);

        clGetEventProfilingInfo(read_event[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, (void*)0);
        clGetEventProfilingInfo(read_event[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, (void*)0);
        result = end - start;
        printf("Started : %llu\tEnded: %llu\n", start, end);
        printf("Elapsed Time of Read Event %d : %llu ns (%llu ms)\n", i + 1, result, result / 1000000);

        puts("\n");
    }
}

void sequential_integral(size_t global_size, float dx)
{
    clock_t start_time, end_time;
    float* arr = (float*)calloc(global_size, sizeof(float));
    float sum = .0f;

    puts("== Sequential Integral Calculation ==");
    start_time = clock();

    for (size_t i = 0; i < global_size; i++)
    {
        float x = dx * i;
        arr[i] = (3 * x * x + 2 * x + 1) * dx;
    }

    for (size_t i = 0; i < global_size; i++)
        sum += arr[i];

    end_time = clock();
    printf("%lf\n", sum);
    printf("Elapsed Time : %lf sec\n", (double)(end_time - start_time) / CLK_TCK);

    free(arr);
}

int main(void)
{
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

    // Create two command queues
    cl_command_queue queue[2];
    cl_queue_properties prop[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };

    for (int i = 0; i < 2; i++)
    {
        queue[i] = clCreateCommandQueueWithProperties(context, device, prop, &err);
        CHECK_ERROR(err);
    }

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
    cl_kernel kernel = clCreateKernel(program, "integral", &err);
    CHECK_ERROR(err);

    // Define global size and variables
    size_t global_size = 536870912;
    size_t half_size = global_size / 2l;
    size_t buffer_size = half_size * sizeof(float);
    int kernel_offset[2] = { 0, (int)half_size };

    int end = 1000, start = 0;
    float dx = (float)(end - start) / global_size;
    float* arr = (float*)calloc(global_size, sizeof(float));
    float sum = .0f;

    cl_mem buffer[2];
    cl_event kernel_event[2];
    cl_event read_event[2];

    for (int i = 0; i < 2; i++)
    {
        buffer[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, (void*)0, &err);
        CHECK_ERROR(err);
    }

    err = clSetKernelArg(kernel, 0, sizeof(cl_float), &dx);
    CHECK_ERROR(err);

    puts("== Parallel Integral Calculation ==");
    // Begin measuring 
    time_t start_time, end_time;
    start_time = clock();

    for (int i = 0; i < 2; i++)
    {
        err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer[i]);
        CHECK_ERROR(err);

        err = clSetKernelArg(kernel, 2, sizeof(cl_int), &kernel_offset[i]);
        CHECK_ERROR(err);

        err = clEnqueueNDRangeKernel(queue[0], kernel, 1, (void*)0, &half_size,
            (void*)0, 0, (void*)0, &kernel_event[i]);
        CHECK_ERROR(err);

        err = clEnqueueReadBuffer(queue[1], buffer[i], CL_FALSE, 0,
            buffer_size, arr + kernel_offset[i], 1, &kernel_event[i], &read_event[i]);
        CHECK_ERROR(err);

        clFinish(queue[i]);
    }

    for (size_t i = 0; i < global_size; i++)
        sum += arr[i];

    end_time = clock();

    printf("%lf\n", sum);
    printf("Elapsed Time : %lf sec\n", (double)(end_time - start_time) / CLK_TCK);
    show_profiling_info(kernel_event, read_event, 2);

    for (int i = 0; i < 2; i++)
        clReleaseMemObject(buffer[i]);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    for (int i = 0; i < 2; i++)
        clReleaseCommandQueue(queue[i]);
    clReleaseContext(context);
    free(arr);
    
    sequential_integral(global_size, dx);

    return 0;
}