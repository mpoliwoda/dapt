void foo(int _PB_N, int* A, int* B, int _PB_TSTEPS)
{
#pragma scop
	for (int t = 0; t < _PB_TSTEPS; t++) {
		for (int i = 1; i < _PB_N - 1; i++)
			B[i] = 0.33333 * (A[i-1] + A[i] + A[i + 1]);
		for (int i = 1; i < _PB_N - 1; i++)
			A[i] = 0.33333 * (B[i-1] + B[i] + B[i + 1]);
	}
#pragma endscop
}

