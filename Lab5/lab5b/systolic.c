#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fifo.c"
#include "pe.c"

// extern N;

typedef struct {
    PE grid[N][N];
    FIFO top[N];
    FIFO left[N];
    int cycle;
} SystolicArray;


// ========== SYSTOLIC ARRAY (PRINT STATE) ==============

void sysarray_print_state(const SystolicArray *arr){
  const int LW = 6;   // width for left label column
  const int W  = 14;  // width for each ACC column (wider to fit "acc (+a*b)")

  printf("Cycle = %d\n", arr->cycle);

  // Column headers (left-aligned to match cells)
  printf("%-*s", LW, "i \\ j");
  for (int j = 0; j < N; j++) {
    printf("%-*d", W, j);
  }
  printf("\n");

  // Rows
  for (int i = 0; i < N; i++) {
    printf("%-*d", LW, i);

    for (int j = 0; j < N; j++) {
      char cell[64];
      snprintf(cell, sizeof(cell), "%d", arr->grid[i][j].acc);

      // Truncate if longer than column width to preserve alignment
      if ((int)strlen(cell) > W) {
        // print first W characters
        printf("%-*.*s", W, W, cell);
      } else {
        printf("%-*s", W, cell);
      }
    }
    printf("\n");
  }
  printf("\n");
}

//----------Returns true if all PEs have done==true---------
bool sysarray_all_done(const SystolicArray *arr){
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      if (!arr->grid[i][j].done) {
        return false; }}}
  return true;
}

// Copy the PE accumulators into C[row][col]
void sysarray_readC(const SystolicArray *arr, int C[N][N]){
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      C[i][j] = arr->grid[i][j].acc;
    }
  }
}


// ========== INIT SYSTOLIC ARRAY (TODO) ==============

// Init the systolic arrays
//   (1) NxN PEs 
//   (2) N fifos on left 
//   (3) N fifos on top 
//   (4) Init both sets of fifos appropriately


void sysarray_init(SystolicArray *arr, int A[N][N], int B[N][N]){
  for(int i = 0; i < N; i++) {
    fifo_init(&(arr->left[i]));
    fifo_init(&(arr->top[i]));
    for(int j = 0; j < N; j++) {
      pe_init(&(arr->grid[i][j]));
      fifo_push(&(arr->left[i]), A[i][j]);
      fifo_push(&(arr->top[i]), B[j][i]);
    }
  }

}

// ========== CYCLE SYSTOLIC ARRAY (TODO) ==============


// Advance the systolic array by one cycle.
// Steps:
//   (1) Cycle PEs 
//   (2) If PE did useful work, then invalidate west_in and north_in
//   (3) If PE did useful work, then transfer east_out of PE to west_in next PE
//   (4) If PE did useful work, then transfer sout_out of PE to north_in of next PE
//   (5) Check if Col-0 can consume from left FIFOs
//   (6) Check if Row-0 can consume from top FIFOs



void sysarray_cycle(SystolicArray *arr){
  for(int i = N-1; i >= 0; i--) {
    for(int j = N-1; j >= 0; j--) {
      PE *pe = &(arr->grid[i][j]);
      pe_cycle(pe);
      if(pe->worked) {
        pe->west_in.valid = false;
        pe->north_in.valid = false;
        if(i != N-1)
          arr->grid[i+1][j].north_in = pe->south_out;
        if(j != N-1) {
          arr->grid[i][j+1].west_in = pe->east_out;
        }        
      }
    }
  }
  for(int i = 0; i < N; i++) {
    if(i > arr->cycle) {
      break;
    }
    FIFO *left = &(arr->left[i]), *top = &(arr->top[i]);
    PE *pe_left = &(arr->grid[i][0]), *pe_top = &(arr->grid[0][i]);
    pe_left->west_in.valid = fifo_pop(left, &(pe_left->west_in.data)); 
    pe_top->north_in.valid = fifo_pop(top, &(pe_top->north_in.data)); 
  }
  // printf("Cycle:%d\n", arr->cycle);
  // for(int i = 0; i < N; i++) {
  //   for(int j = 0; j < N; j++) {
  //     printf("%d\t", arr->grid[i][j].done);
  //   }
  //   printf("\n");
  // }
  arr->cycle++;
}






