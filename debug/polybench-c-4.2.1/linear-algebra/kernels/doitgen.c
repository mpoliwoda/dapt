int SCALAR_VAL(double);

void foo(int _PB_NR, int _PB_NQ, int _PB_NP, int* sum, int** C4, int*** A)
{
#pragma scop
	for (int r = 0; r < _PB_NR; r++)
		for (int q = 0; q < _PB_NQ; q++)
		{
			for (int p = 0; p < _PB_NP; p++)
			{
				sum[p] = SCALAR_VAL(0.0);
				for (int s = 0; s < _PB_NP; s++)
					sum[p] += A[r][q][s] * C4[s][p];
			}
			for (int p = 0; p < _PB_NP; p++)
				A[r][q][p] = sum[p];
		}
#pragma endscop
}
