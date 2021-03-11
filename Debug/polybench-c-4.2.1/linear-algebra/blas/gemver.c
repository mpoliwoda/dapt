void foo(char _PB_N, int** A, int* u1, int* v1, int* u2, int* v2, int* x, int* z, int* y, int* w, int alpha, int beta)
{
#pragma scop
  for (int i = 0; i < _PB_N; i++)
    for (int j = 0; j < _PB_N; j++)
      A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];

  for (int i = 0; i < _PB_N; i++)
    for (int j = 0; j < _PB_N; j++)
      x[i] = x[i] + beta * A[j][i] * y[j];

  for (int i = 0; i < _PB_N; i++)
    x[i] = x[i] + z[i];

  for (int i = 0; i < _PB_N; i++)
    for (int j = 0; j < _PB_N; j++)
      w[i] = w[i] +  alpha * A[i][j] * x[j];
#pragma endscop
}
