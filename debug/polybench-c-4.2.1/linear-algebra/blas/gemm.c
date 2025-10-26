void foo(int _PB_NI, int _PB_NJ, int beta, int alpha, int _PB_NK, int** A, int** B, int** C)
{
#pragma scop
	for (int i = 0; i < _PB_NI; i++) {
		for (int j = 0; j < _PB_NJ; j++)
			C[i][j] *= beta;
		for (int k = 0; k < _PB_NK; k++) {
			for (int j = 0; j < _PB_NJ; j++)
				C[i][j] += alpha * A[i][k] * B[k][j];
		}
	}
#pragma endscop
}
