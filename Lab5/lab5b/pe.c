#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

// extern N;

typedef struct {
  bool valid;
  int data;
} DataPacket;


// ================= PE   =======================
//
// A Processing Element (PE) sits at (row i, col j).
// Each cycle:
//  - It checkes if values exists at north_in and west_in
//  - If so:
//    - it does acc += A*B
//    - passes north_in to south_out, west_in to east_out in the same cycle
//    - if mac_count == N, it sets done=true and stops


typedef struct {
    DataPacket west_in;   // A values arriving from the west
    DataPacket north_in;   // B values arriving from the north

    DataPacket east_out;   // Pass through for west 
    DataPacket south_out;   // Pass through for north

    int acc;          // accumulated partial sum for C[i][j]
    int mac_count;    // how many MACs we've performed so far
    bool worked;      // track if PE did useful work this cycle (not stalled)
    bool done;        // true once we have N MACs (N=4 here)
} PE;


// Initialize PE internal state and FIFOs
void pe_init(PE *p){

  p->west_in.valid  = false;
  p->north_in.valid = false;
  p->east_out.valid  = false;
  p->south_out.valid = false;
  
  p->acc = 0;
  p->mac_count = 0;
  p->done = false;
}


// One simulation cycle of this PE's computation.
// Steps:
//   1. Check if both west and north inputs are available
//   2. If yes, do acc += A * B, mac_count++, and if mac_count == N, set done=true.
//   3. Transfer inputs to outputs (north to south, west to east, set valid bits)
//   4. If PE did work this cycle (not stalled), then worked=true, else false


void pe_cycle(PE *p){
  // TODO
  if(p->west_in.valid && p->north_in.valid) {
    p->acc += p->west_in.data * p->north_in.data;
    p->mac_count++;
    if(p->mac_count == N) {
      p->done = true;
    }
    p->worked = true;
    p->east_out = p->west_in;
    p->south_out = p->north_in;
    p->north_in.valid = false;
    p->west_in.valid = false;
  } else {
    p->worked = false;
  }
}

