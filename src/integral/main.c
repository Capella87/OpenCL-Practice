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
    cl_kernel kernel = clCreateKernel(program, "integral", &err);
    CHECK_ERROR(err);


    // const float dx, __global float* sum

    size_t global_size = 1000;
    
    size_t result_size = sizeof(float) * global_size;

    int end = 1, start = 0;
    float dx = (float)(end - start) / global_size;

    float* arr = (float*)calloc(global_size, sizeof(float));

    cl_mem result = clCreateBuffer(context, CL_MEM_READ_WRITE, result_size, (void*)0, &err);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_float), &dx);
    CHECK_ERROR(err);
    
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &result);
    CHECK_ERROR(err);

    time_t start_time, end_time;

    start_time = clock();

    err = clEnqueueNDRangeKernel(queue, kernel, 1, (void*)0, &global_size, (void*)0, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    err = clEnqueueReadBuffer(queue, result, CL_FALSE, 0, result_size, arr, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    // Finish the work

    clFinish(queue);

    float sum = .0f;
    for (size_t i = 0; i < global_size; i++)
        sum += arr[i];
    end_time = clock();

    printf("%f\n", sum);
    printf("Elapsed Time : %lf sec\n", (end_time - start_time) / CLK_TCK);

    clReleaseMemObject(result);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(arr);

    arr = (float*)calloc(global_size, sizeof(float));
    sum = 0;
    start = clock();
    for (size_t i = 0; i < global_size; i++)
    {
        float x = dx * i;
        arr[i] = (3 * x * x + 2 * x + 1) * dx;
        sum += arr[i];
    }
    end_time = clock();
    printf("%f\n", sum);
    printf("Elapsed Time : %lf sec\n", (end_time - start_time) / CLK_TCK);

    free(arr);
    return 0;
}