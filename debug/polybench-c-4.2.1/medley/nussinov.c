int max_score(int, int);
int match(int, int);

void foo(int _PB_N, int** table, int* seq)
{
#pragma scop
	for (int i = _PB_N-1; i >= 0; i--) {
		for (int j=i+1; j<_PB_N; j++) {
			
			if (j-1>=0)
				table[i][j] = max_score(table[i][j], table[i][j-1]);
			if (i+1<_PB_N)
				table[i][j] = max_score(table[i][j], table[i+1][j]);

			if (j-1>=0 && i+1<_PB_N) {
				/* don't allow adjacent elements to bond */
				if (i<j-1)
					table[i][j] = max_score(table[i][j], table[i+1][j-1]+match(seq[i], seq[j]));
				else
					table[i][j] = max_score(table[i][j], table[i+1][j-1]);
			}

			for (int k=i+1; k<j; k++) {
				table[i][j] = max_score(table[i][j], table[i][k] + table[k+1][j]);
			}
		}
	}
#pragma endscop
}
