__kernel void rotate(__global const float* input, 
                    __global float* output, const int W, const int H, 
                    const float x0, const float y0, const float sin_theta, const float cos_theta)
{
    int dest_x = get_global_id(0);
    int dest_y = get_global_id(1);

    float xOff = dest_x - x0;
    float yOff = dest_y - y0;
    int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
    int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
    if ((src_x >= 0) && (src_x < W) && (src_y >= 0) && (src_y < H))
        output[dest_y * W + dest_x] = input[src_y * W + src_x];
    else output[dest_y * W + dest_x] = 0.0f;
}