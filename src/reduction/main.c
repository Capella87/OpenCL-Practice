#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <time.h>


#define COUNT 16777216

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

    // The kernel code goes here

    cl_kernel kernel = clCreateKernel(program, "reduction", &err);
    CHECK_ERROR(err);


    size_t global_size = COUNT;
    size_t local_size = 256;

    size_t size = COUNT * sizeof(int);
    size_t half = size / 2;
    int* input_arr = (int*)malloc(size);
    for (int i = 0; i < COUNT; i++)
        input_arr[i] = 2;
    const size_t group_count = COUNT / local_size;

    int* global_sum_arr = (int*)malloc(sizeof(int) * group_count);
    for (int i = 0; i < group_count; i++)
        global_sum_arr[i] = 0;
    int totalNum = COUNT;


    
    cl_mem input = clCreateBuffer(context, CL_MEM_READ_ONLY, size, (void*)0, &err);
    CHECK_ERROR(err);

    cl_mem global_sum = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * group_count, (void*)0, &err);
    CHECK_ERROR(err);

    err = clEnqueueWriteBuffer(queue, input, CL_FALSE, 0, size, input_arr, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    err = clEnqueueWriteBuffer(queue, global_sum, CL_FALSE, 0, sizeof(int) * group_count, global_sum_arr, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    // Set kernel arguments

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &global_sum);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)0);
    CHECK_ERROR(err);

    err = clSetKernelArg(kernel, 3, sizeof(cl_int), &totalNum);
    CHECK_ERROR(err);

    // Initialize global size and execute over the kernel
    clock_t start_time, end_time;

    // Begin elapsed time measurement
    start_time = clock();

    err = clEnqueueNDRangeKernel(queue, kernel, 1, (void*)0, &global_size, &local_size, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    err = clEnqueueReadBuffer(queue, global_sum, CL_TRUE, 0, sizeof(int) * group_count, global_sum_arr, 0, (void*)0, (void*)0);
    CHECK_ERROR(err);

    clFinish(queue);

    int sum = 0;
    for (int i = 0; i < group_count; i++)
        sum += global_sum_arr[i];

    end_time = clock();

    printf("%d\n", sum);
    printf("Elapsed time: %f sec\n", (double)(end_time - start_time) / CLK_TCK);

    // Finish the work

    clReleaseMemObject(input);
    clReleaseMemObject(global_sum);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    // Sequential Calculation

    start_time = clock();

    sum = 0;
    for (int i = 0; i < COUNT; i++)
        sum += input_arr[i];

    end_time = clock();

    printf("%d\n", sum);
    printf("Elapsed time: %f sec\n", (double)(end_time - start_time) / CLK_TCK);
    

    // Deallocate arrays

    free(input_arr);
    free(global_sum_arr);

    return 0;
}