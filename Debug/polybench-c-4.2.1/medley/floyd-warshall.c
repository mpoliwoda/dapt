void foo(int _PB_N, int** path)
{
#pragma scop
	for (int k = 0; k < _PB_N; k++) {
		for (int i = 0; i < _PB_N; i++)
			for (int j = 0; j < _PB_N; j++)
				path[i][j] = path[i][j] < path[i][k] + path[k][j] ? path[i][j] : path[i][k] + path[k][j];
	}
#pragma endscop
}

