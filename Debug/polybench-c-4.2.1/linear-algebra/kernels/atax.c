int SCALAR_VAL(double);

void foo(int _PB_N, int _PB_M, int* tmp, int* y, int** A, int** B, int* x, int alpha, int beta)
{
#pragma scop
	for (int i = 0; i < _PB_N; i++)
		y[i] = 0;
	for (int i = 0; i < _PB_M; i++)
    {
		tmp[i] = SCALAR_VAL(0.0);
		for (int j = 0; j < _PB_N; j++)
			tmp[i] = tmp[i] + A[i][j] * x[j];
		for (int j = 0; j < _PB_N; j++)
			y[j] = y[j] + A[i][j] * tmp[i];
	}
#pragma endscop
}
