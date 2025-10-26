int SCALAR_VAL(double);

void foo(int _PB_N, int _PB_M, int* s, int* q, int* r, int* p, int** A)
{
#pragma scop
	for (int i = 0; i < _PB_M; i++)
		s[i] = 0;
	for (int i = 0; i < _PB_N; i++)
	{
		q[i] = SCALAR_VAL(0.0);
		for (int j = 0; j < _PB_M; j++)
		{
			s[j] = s[j] + r[i] * A[i][j];
			q[i] = q[i] + A[i][j] * p[j];
		}
	}
#pragma endscop
}
