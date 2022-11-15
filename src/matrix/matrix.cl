__kernel void mult_mat(__global float* A, __global float* B, __global float* C, 
					const int ROW_A, const int COL_B)
{
	__private float value = 0.0f;

	__private int task_x = get_global_id(0);
	__private int task_y = get_global_id(1);
	
	for (int i = 0; i < ROW_A; ++i)
    {
        float temp_a = A[task_y * ROW_A + i];
        float temp_b = B[COL_B * i + task_x];
        
        value += temp_a * temp_b;
    }

    C[task_y * ROW_A + task_x] = value;
}