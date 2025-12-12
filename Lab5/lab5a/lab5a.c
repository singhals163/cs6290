#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



#ifndef MAX_N
#define MAX_N  4096
#endif

#ifndef BLK_SIZE
#define BLK_SIZE  128
#endif


double A[MAX_N][MAX_N];
double B[MAX_N][MAX_N];
double C[MAX_N][MAX_N];


//------- init ------------
void init_AB(int N){
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      A[i][j]=  (double)rand() / RAND_MAX;
      B[i][j]=  (double)rand() / RAND_MAX;
      C[i][j]=  0; 
    }
  }
}

void init_C(int N){
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      C[i][j]=  0; 
    }
  }
}


//----- time now in milliseconds ----
double now_ms() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1.0e6;
}


//----- GIVEN: matrix addition  ----
void matrix_add(int N) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      C[i][j]=  A[i][j]+B[i][j]; 
    }
  }
}


//----- TODO: simple matrix multiplication  ----
void simple_matmul(int N) {
  for(int i = 0; i < N; i++) {
    for(int k = 0; k < N; k++) {
      for(int j = 0; j < N; j++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
 
}

//----- TODO: blocked matrix multiplication  ----
void blocked_matmul(int N) {
  int S = BLK_SIZE; 
  for(int blocki = 0; blocki < N; blocki += S) {
    for(int blockk = 0; blockk < N; blockk += S) {
      for(int blockj = 0; blockj < N; blockj += S) {
        for(int i = 0; i < S; i++) {
          for(int k = 0; k < S; k++) {
            for(int j = 0; j < S; j++) {
              C[i + blocki][j + blockj] += A[i + blocki][k + blockk] * B[k + blockk][j + blockj];
            }
          }
        }
      }
    }
  }
}
 


//----- main function, for init, call, time  ----

int main(void){
  double start, end, elapsed;
  srand(42);
   
  int niter = 5;
  printf("%10s%20s%20s%20s\n", "Size (N)", "ADD-Time (ms)", "SMM-Time (ms)", "BMM-Time (ms)");
  double prev_bmm_time = 0;
  for (int N = 256; N <= MAX_N; N *= 2) {
    init_AB(N);
    printf("%10d", N);

    init_C(N);
    start = now_ms();
    for (int i = 0; i < niter; i++) {
      matrix_add(N);
    }
    end = now_ms();
    elapsed = (end - start) / niter;
    printf("%20.1f", elapsed);

    init_C(N);
    start = now_ms();
    simple_matmul(N);
    end = now_ms();
    elapsed = (end - start);
    printf("%20.1f", elapsed);

    init_C(N);
    start = now_ms();
    for (int i = 0; i < niter; i++) {
      blocked_matmul(N);
    }
    end = now_ms();
    elapsed = (end - start) / niter;
    printf("%20.1f", elapsed);
    if (prev_bmm_time != 0) {
      printf(" (%.1f)\n", elapsed / prev_bmm_time);
    } else {
      printf("\n");
    }
    prev_bmm_time = elapsed;

  }
  
  return 0;
}
