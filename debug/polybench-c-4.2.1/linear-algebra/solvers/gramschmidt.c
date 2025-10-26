int SCALAR_VAL(double);
int SQRT_FUN(int);

void foo(int _PB_N, int _PB_M, int k, int nrm, int ** R, int ** Q, int ** A)
{
	for (int k = 0; k < _PB_N; k++)	{
		nrm = SCALAR_VAL(0.0);
		for (int i = 0; i < _PB_M; i++)
			nrm += A[i][k] * A[i][k];
		R[k][k] = SQRT_FUN(nrm);
#pragma scop
		for (int i = 0; i < _PB_M; i++)
			Q[i][k] = A[i][k] / R[k][k];
		for (int j = k + 1; j < _PB_N; j++)	{
			R[k][j] = SCALAR_VAL(0.0);
			for (int i = 0; i < _PB_M; i++)
				R[k][j] += Q[i][k] * A[i][j];
			for (int i = 0; i < _PB_M; i++)
				A[i][j] = A[i][j] - Q[i][k] * R[k][j];
		}
#pragma endscop
	}

}

