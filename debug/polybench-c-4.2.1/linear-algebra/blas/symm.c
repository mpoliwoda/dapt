void foo(int _PB_N, int _PB_M, int beta, int alpha, int** A, int** B, int** C, int temp2)
{
#pragma scop
   for (int i = 0; i < _PB_M; i++)
      for (int j = 0; j < _PB_N; j++ )
      {
        temp2 = 0;
        for (int k = 0; k < i; k++) {
           C[k][j] += alpha*B[i][j] * A[i][k];
           temp2 += B[k][j] * A[i][k];
        }
        C[i][j] = beta * C[i][j] + alpha*B[i][j] * A[i][i] + alpha * temp2;
     }
#pragma endscop
}

/*

temp2 -> temp2[i][j] lastprivate temp2

void foo(int _PB_N, int _PB_M, int beta, int alpha, int** A, int** B, int** C, int **temp2)
{
#pragma scop
   for (int i = 0; i < _PB_M; i++)
      for (int j = 0; j < _PB_N; j++ )
      {
        temp2[i][j] = 0;
        for (int k = 0; k < i; k++) {
           C[k][j] += alpha*B[i][j] * A[i][k];
           temp2[i][j] += B[k][j] * A[i][k];
        }
        C[i][j] = beta * C[i][j] + alpha*B[i][j] * A[i][i] + alpha * temp2[i][j];
     }
#pragma endscop
}
*/