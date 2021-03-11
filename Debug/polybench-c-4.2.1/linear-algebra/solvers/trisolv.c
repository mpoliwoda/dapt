void foo(char _PB_N, int** L, int* b, int* x)
{
#pragma scop
	for (int i = 0; i < _PB_N; i++) {
		x[i] = b[i];
		for (int j = 0; j <i; j++)
			x[i] -= L[i][j] * x[j];
		x[i] = x[i] / L[i][i];
	}
#pragma endscop
}
