int SQRT_FUN(int);

void foo(int _PB_N, int** A)
{
#pragma scop
	for (int i = 0; i < _PB_N; i++) {
		//j<i
		for (int j = 0; j < i; j++) {
			for (int k = 0; k < j; k++) {
				A[i][j] -= A[i][k] * A[j][k];
			}
			A[i][j] /= A[j][j];
		}
		// i==j case
		for (int k = 0; k < i; k++) {
			A[i][i] -= A[i][k] * A[i][k];
		}
		A[i][i] = SQRT_FUN(A[i][i]);
	}
#pragma endscop
}
