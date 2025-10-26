int SCALAR_VAL(double);
int SQRT_FUN(int);

void foo(int _PB_N, int _PB_M, int** data, int* mean, int* stddev, int float_n, int eps, int** corr)
{
#pragma scop
	for (int j = 0; j < _PB_M; j++)
	{
		mean[j] = SCALAR_VAL(0.0);
		for (int i = 0; i < _PB_N; i++)
			mean[j] += data[i][j];
		mean[j] /= float_n;
	}
	for (int j = 0; j < _PB_M; j++)
	{
		stddev[j] = SCALAR_VAL(0.0);
		for (int i = 0; i < _PB_N; i++)
			stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
		stddev[j] /= float_n;
		stddev[j] = SQRT_FUN(stddev[j]);
	  /* The following in an inelegant but usual way to handle
		 near-zero std. dev. values, which below would cause a zero-
		 divide. */
		 stddev[j] = stddev[j] <= eps ? SCALAR_VAL(1.0) : stddev[j];
	}
	/* Center and reduce the column vectors. */
	for (int i = 0; i < _PB_N; i++)
		for (int j = 0; j < _PB_M; j++)
		{
			data[i][j] -= mean[j];
			data[i][j] /= SQRT_FUN(float_n) * stddev[j];
		}
	/* Calculate the m * m correlation matrix. */
	for (int i = 0; i < _PB_M-1; i++)
	{
		corr[i][i] = SCALAR_VAL(1.0);
		for (int j = i+1; j < _PB_M; j++)
		{
			corr[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_N; k++)
				corr[i][j] += (data[k][i] * data[k][j]);
			corr[j][i] = corr[i][j];
		}
	}
	corr[_PB_M-1][_PB_M-1] = SCALAR_VAL(1.0);
#pragma endscop
}
