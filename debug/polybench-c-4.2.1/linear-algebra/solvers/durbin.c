int SCALAR_VAL(int);

void foo(int _PB_N, int* y, int* r, int* z, int alpha, int beta, int sum)
{
#pragma scop
	y[0] = -r[0];
	beta = SCALAR_VAL(1.0);
	alpha = -r[0];

	for (int k = 1; k < _PB_N; k++) {
		beta = (1-alpha*alpha)*beta;
		sum = SCALAR_VAL(0.0);
		for (int i=0; i<k; i++) {
			sum += r[k-i-1]*y[i];
		}
		alpha = - (r[k] + sum)/beta;
		
		for (int i=0; i<k; i++) {
			z[i] = y[i] + alpha*y[k-i-1];
		}
		for (int i=0; i<k; i++) {
			y[i] = z[i];
		}
		y[k] = alpha;
	}
#pragma endscop
}
