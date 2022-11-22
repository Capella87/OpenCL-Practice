__kernel void rotate(__global const double* input, 
                    __global double* output, const int W, const int H, 
                    const double x0, const double y0, const double sin_theta, const double cos_theta)
{
    int dest_x = get_global_id(0);
    int dest_y = get_global_id(1);

    double xOff = dest_x - x0;
    double yOff = dest_y - y0;
    int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
    int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
    if ((src_x >= 0) && (src_x < W) && (src_y >= 0) && (src_y < H))
        output[dest_y * W + dest_x] = input[src_y * W + src_x];
    else output[dest_y * W + dest_x] = 0.0f;
}