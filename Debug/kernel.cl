__kernel void MyCLAdd(__global float *ballx, __global float *bally, __global float *angle)
{
	int index = get_global_id(0);
	ballx[index] = ballx[index] + sin(angle[index]);
	bally[index] = bally[index] + cos(angle[index]); 
}