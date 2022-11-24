__kernel void integral(const float dx, __global float* sum)
{
    int idx = get_global_id(0);
    float x = dx * (float)idx;
    
    sum[idx] = (3 * x * x + 2 * x + 1) * dx;
}