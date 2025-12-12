#include <stdio.h>
#include <stdlib.h>

#define VERBOSE 0

#define N 4


#include "systolic.c"




//--------------- Pretty-Print a Matrix -----------------------

static void print_mat(const char *name, int M[N][N]) {
    printf("%s =\n", name);
    for (int i = 0; i < N; i++) {
        printf("  ");
        for (int j = 0; j < N; j++) {
            printf("%6d ", M[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

//--------------- Test Harness -----------------------

int main(void) {
  srand(1234);
  int A[N][N];
  int B[N][N];
  int golden[N][N];
     
  // Example input matrices
  for(int i=0; i<N; i++){
    for(int j=0; j<N; j++){
      A[i][j]=rand()%8;
      B[i][j]=rand()%4;
    }
  }


  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      golden[i][j] = 0;
      for (int k = 0; k < N; k++) {
        golden[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  // Initialize systolic array
  SystolicArray arr;
  sysarray_init(&arr, A, B);  

    // We need enough cycles to:
    // - inject all A[i][k] and B[k][j] with the proper skew,
    // - and allow them to propagate through the 4x4 mesh.

    int max_cycles = 3*N;

    for (int t = 0; t < max_cycles; t++) {
        sysarray_cycle(&arr);
	if(VERBOSE){
	  sysarray_print_state(&arr);
	}
        if(sysarray_all_done(&arr)) {
            break;
        }
    }

    // Read out the hardware result matrix C
    int hwC[N][N];
    sysarray_readC(&arr, hwC);

    // Print matrices
    print_mat("A", A);
    print_mat("B", B);
    print_mat("Golden = A*B", golden);
    print_mat("HW result", hwC);

    printf("Cycle = %d\n", arr.cycle);

    // Check PASS / FAIL
    int pass = 1;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (hwC[i][j] != golden[i][j]) {
                pass = 0;
            }
        }
    }

    if (pass) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }

    return 0;
}
