int SCALAR_VAL(int y);

void foo(char _PB_NI, char _PB_NJ, char _PB_NL, char _PB_NK, char _PB_NM, int** A,int** B,int** C,int** D,int** E, int** F, int** G)
{
#pragma scop
	/* E := A*B */
	for (int i = 0; i < _PB_NI; i++)
		for (int j = 0; j < _PB_NJ; j++)
		{
			E[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_NK; ++k)
				E[i][j] += A[i][k] * B[k][j];
		}
	/* F := C*D */
	for (int i = 0; i < _PB_NJ; i++)
		for (int j = 0; j < _PB_NL; j++)
		{
			F[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_NM; ++k)
				F[i][j] += C[i][k] * D[k][j];
		}
	/* G := E*F */
	for (int i = 0; i < _PB_NI; i++)
		for (int j = 0; j < _PB_NL; j++)
		{
			G[i][j] = SCALAR_VAL(0.0);
			for (int k = 0; k < _PB_NJ; ++k)
				G[i][j] += E[i][k] * F[k][j];
		}
#pragma endscop
}
