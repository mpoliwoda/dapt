int SCALAR_VAL(double);

void foo(int _PB_TMAX, int _PB_NY, int _PB_NX, int** ey, int** ex, int** hz, int* _fict_, int t)
{
#pragma scop
	for (int t = 0; t < _PB_TMAX; t++) {
		for (int j = 0; j < _PB_NY; j++)
			ey[0][j] = _fict_[t];
		for (int i = 1; i < _PB_NX; i++)
			for (int j = 0; j < _PB_NY; j++)
				ey[i][j] = ey[i][j] - SCALAR_VAL(0.5)*(hz[i][j]-hz[i-1][j]);
		for (int i = 0; i < _PB_NX; i++)
			for (int j = 1; j < _PB_NY; j++)
				ex[i][j] = ex[i][j] - SCALAR_VAL(0.5)*(hz[i][j]-hz[i][j-1]);
		for (int i = 0; i < _PB_NX - 1; i++)
			for (int j = 0; j < _PB_NY - 1; j++)
				hz[i][j] = hz[i][j] - SCALAR_VAL(0.7)*  (ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
	}
#pragma endscop
}

