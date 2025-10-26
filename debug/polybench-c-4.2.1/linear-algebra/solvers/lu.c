void foo(char _PB_N, int** A)
{
#pragma scop
	for (int i = 0; i < _PB_N; i++) {
		for (int j = 0; j <i; j++) {
			for (int k = 0; k < j; k++) {
				A[i][j] -= A[i][k] * A[k][j];
			}
			A[i][j] /= A[j][j];
		}
		for (int j = i; j < _PB_N; j++) {
			for (int k = 0; k < i; k++) {
				A[i][j] -= A[i][k] * A[k][j];
			}
		}
	}
#pragma endscop
}
