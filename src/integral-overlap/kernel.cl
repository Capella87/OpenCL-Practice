__kernel void integral(const float dx, __global float* sum, const int offset)
{
    int idx = get_global_id(0);
    int location = idx + offset;
    float x = dx * (float)location;

    sum[idx] = (3 * x * x + 2 * x + 1) * dx;
}