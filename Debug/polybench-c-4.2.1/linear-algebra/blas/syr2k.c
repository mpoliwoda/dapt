void foo(int _PB_N, int _PB_M, int beta, int alpha, int** A, int** B, int** C)
{
#pragma scop
  for (int i = 0; i < _PB_N; i++) {
    for (int j = 0; j <= i; j++)
      C[i][j] *= beta;
    for (int k = 0; k < _PB_M; k++)
      for (int j = 0; j <= i; j++)
	{
	  C[i][j] += A[j][k]*alpha*B[i][k] + B[j][k]*alpha*A[i][k];
	}
  }
#pragma endscop
}

