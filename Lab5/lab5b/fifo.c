#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>



typedef struct {
    int data[N];
    int head;   // index of next element to pop
    int tail;   // index of next free slot to push
    int count;  // number of valid elements
} FIFO;


// Initialize FIFO
void fifo_init(FIFO *f){
  f->head = 0;
  f->tail = 0;
  f->count = 0;
}

// Push a value 
void fifo_push(FIFO *f, int val){
  assert(f->count < N);
  f->data[f->tail] = val;
  f->tail = (f->tail + 1) % N;
  f->count++;
}


// Pop a value. Returns true if we popped into *out
bool fifo_pop(FIFO *f, int *out){
  if (f->count == 0) {
    return false;
  }
  *out = f->data[f->head];
  f->head = (f->head + 1) % N;
  f->count--;
  return true;
}
