void foo(int _PB_M, int _PB_N, int** A, int** B, int alpha)
{
#pragma scop
  for (int i = 0; i < _PB_M; i++)
     for (int j = 0; j < _PB_N; j++) {
        for (int k = i+1; k < _PB_M; k++)
           B[i][j] += A[k][i] * B[k][j];
        B[i][j] = alpha * B[i][j];
     }
#pragma endscop
}
