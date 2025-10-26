int SCALAR_VAL(double);

void foo(char _PB_NI, char _PB_NJ, char _PB_NK, char _PB_NL, int alpha, int beta, int** tmp, int** A, int** B, int** C, int** D)
{
#pragma scop
	for (int i = 0; i < _PB_NI; i++)
		for (int j = 0; j < _PB_NJ; j++)
		{
			tmp[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_NK; ++k)
				tmp[i][j] += alpha * A[i][k] * B[k][j];
		}
	for (int i = 0; i < _PB_NI; i++)
		for (int j = 0; j < _PB_NL; j++)
		{
			D[i][j] *= beta;
			for (int k = 0; k < _PB_NJ; ++k)
				D[i][j] += tmp[i][k] * C[k][j];
		}
#pragma endscop
}
