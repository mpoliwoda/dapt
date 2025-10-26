void foo(int _PB_N, int** A, int* x1, int* x2, int* y_1, int* y_2 )
{
#pragma scop
	for (int i = 0; i < _PB_N; i++)
		for (int j = 0; j < _PB_N; j++)
			x1[i] = x1[i] + A[i][j] * y_1[j];
	for (int i = 0; i < _PB_N; i++)
		for (int j = 0; j < _PB_N; j++)
			x2[i] = x2[i] + A[j][i] * y_2[j];
#pragma endscop
}

