void foo(int _PB_W, int _PB_H, int** y1, int** y2, int c1, int** imgOut)
{
#pragma scop
    for (int i=0; i<_PB_W; i++)
        for (int j=0; j<_PB_H; j++) {
            imgOut[i][j] = c1 * (y1[i][j] + y2[i][j]);
        }
#pragma endscop
}

