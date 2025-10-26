int SCALAR_VAL(double);

void foo(int _PB_N, int _PB_M, int* mean, int** data, int** cov, int float_n)
{
#pragma scop
	for (int j = 0; j < _PB_M; j++)
    {
		mean[j] = SCALAR_VAL(0.0);
		for (int i = 0; i < _PB_N; i++)
			mean[j] += data[i][j];
		mean[j] /= float_n;
    }
	for (int i = 0; i < _PB_N; i++)
		for (int j = 0; j < _PB_M; j++)
			data[i][j] -= mean[j];
	for (int i = 0; i < _PB_M; i++)
		for (int j = i; j < _PB_M; j++)
		{
			cov[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_N; k++)
				cov[i][j] += data[k][i] * data[k][j];
			cov[i][j] /= (float_n - SCALAR_VAL(1.0));
			cov[j][i] = cov[i][j];
		}
#pragma endscop
}
