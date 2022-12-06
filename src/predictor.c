//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
// Get the k-th bit of x
#define GET_BIT(x, k) ((x >> k) & 1)

// Set other bits except k-th to zero
#define GET_BIT_INPLACE(x, k) (x & (1 << k))

// Get the lower k bits of x
#define GET_LOWER_K_BITS(x, k) (x & ((1 << k) - 1))

// Set the k-th bit of x into y
#define SET_BIT(x, k, y) ((x & ~ (1 << k)) | (y << k))

uint32_t global_history;

// GShare Predictor: each item contains 4 counters with 8 bits in total
uint8_t* two_bits_counters;


uint32_t get_index(uint32_t pc) {
  uint32_t index = global_history ^ pc;
  index = GET_LOWER_K_BITS(index, ghistoryBits);
  return index;
}

uint8_t get_counter(uint32_t index) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;
  
  uint8_t item = two_bits_counters[array_index];
  uint8_t counter = GET_BIT_INPLACE(item, (item_index << 1)) | GET_BIT_INPLACE(item, ((item_index << 1) + 1));
  return counter;
}

void set_counter(uint32_t index, uint8_t counter) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;
  uint8_t item = two_bits_counters[array_index];
  
  // Extract the (item_index)-th 2-bit counter in item
  item = SET_BIT(item, (item_index << 1), GET_BIT(counter, 0));
  item = SET_BIT(item, ((item_index << 1) + 1), GET_BIT(counter, 1));

  two_bits_counters[array_index] = item;
}

void decrease_counter(uint32_t index) {
  uint8_t counter = get_counter(index);
  uint8_t new_counter = 0;
  if (counter == SN) {
    new_counter = SN;
  } else if (counter == WN) {
    new_counter = SN;
  } else if (counter == WT) {
    new_counter = WN;
  } else {
    new_counter = WT;
  }
  set_counter(index, new_counter);
}

void increase_counter(uint32_t index) {
  uint8_t counter = get_counter(index);
  uint8_t new_counter = 0;
  if (counter == SN) {
    new_counter = WN;
  } else if (counter == WN) {
    new_counter = WT;
  } else if (counter == WT) {
    new_counter = ST;
  } else {
    new_counter = ST;
  }
  set_counter(index, new_counter);
}

void gshare_initializer() {
  uint32_t counter_arr_size = (1 << ghistoryBits) >> 2;
  if (counter_arr_size == 0) {
    counter_arr_size = 1;
  }
  two_bits_counters = (uint8_t*) malloc(sizeof(uint8_t) * counter_arr_size);
  // initialize each counter as WN
  for (uint16_t i = 0; i < counter_arr_size; ++i) {
    two_bits_counters[i] = 0b01010101;
  }

  global_history = 0;
}

uint8_t gshare_predictor(uint32_t pc) {
  uint32_t index = get_index(pc);
  uint8_t counter = get_counter(index);
  if (counter == SN || counter == WN) {
    return NOTTAKEN;
  } else {
    return TAKEN;
  }
}

uint8_t gshare_trainer(uint32_t pc, uint8_t outcome) {
  uint32_t index = get_index(pc);

  if (outcome == TAKEN) {
    increase_counter(index);
    global_history = (global_history << 1) | 1;
  } else {
    decrease_counter(index);
    global_history = global_history << 1;
  }

  global_history = GET_LOWER_K_BITS(global_history, ghistoryBits);
}

// Tournament Predictor
uint32_t* local_history;
uint8_t* two_bits_counters_global;
uint8_t* two_bits_counters_local;
uint8_t* two_bits_counters_choice;

// Get the index-th 2-bit counter in array
uint8_t get_counter_by_index(uint8_t* array, uint32_t index) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;

  uint8_t item = array[array_index];
  uint8_t counter = GET_BIT_INPLACE(item, (item_index << 1)) | GET_BIT_INPLACE(item, ((item_index << 1) + 1));
  return counter;
}

// Set the value of the index-th 2-bit counter in array
void set_counter_by_index(uint8_t* array, uint32_t index, uint8_t value) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;
  uint8_t item = array[array_index];
  
  item = SET_BIT(item, (item_index << 1), GET_BIT(value, 0));
  item = SET_BIT(item, ((item_index << 1) + 1), GET_BIT(value, 1));

  array[array_index] = item;
}

void increase_counter_by_index(uint8_t* array, uint32_t index) {
    uint8_t counter = get_counter_by_index(array, index);
    uint8_t new_counter = 0;
    if (counter == SN) {
      new_counter = WN;
    } else if (counter == WN) {
      new_counter = WT;
    } else if (counter == WT) {
      new_counter = ST;
    } else {
      new_counter = ST;
    }
    set_counter_by_index(array, index, new_counter);
}

void decrease_counter_by_index(uint8_t* array, uint32_t index) {
    uint8_t counter = get_counter_by_index(array, index);
    uint8_t new_counter = 0;
    if (counter == SN) {
      new_counter = SN;
    } else if (counter == WN) {
      new_counter = SN;
    } else if (counter == WT) {
      new_counter = WN;
    } else {
      new_counter = WT;
    }
    set_counter_by_index(array, index, new_counter);
}


void tournament_initializer() {
  uint32_t global_size = (1 << ghistoryBits) >> 2;
  uint32_t local_size = (1 << lhistoryBits) >> 2;
  uint32_t choice_size = (1 << pcIndexBits) >> 2;

  global_size = (global_size == 0) ? 1 : global_size;
  local_size = (local_size == 0) ? 1 : local_size;
  choice_size = (choice_size == 0) ? 1 : choice_size;


  two_bits_counters_global = (uint8_t*) malloc(sizeof(uint8_t) * global_size);
  two_bits_counters_local = (uint8_t*) malloc(sizeof(uint8_t) * local_size);
  two_bits_counters_choice = (uint8_t*) malloc(sizeof(uint8_t) * choice_size);
  
  // initialize each counter as weak negative (i.e., 0b01)
  for (uint16_t i = 0; i < global_size; ++i) {
    two_bits_counters_global[i] = 0b01010101;
  }

  for (uint16_t i = 0; i < local_size; ++i) {
    two_bits_counters_local[i] = 0b01010101;
  }

  for (uint16_t i = 0; i < choice_size; ++i) {
    two_bits_counters_choice[i] = 0b01010101;
  }

  uint32_t local_history_key_num = (1 << pcIndexBits);
  local_history = (uint32_t*) malloc(sizeof(uint32_t) * local_history_key_num);
  memset(local_history, 0, sizeof(uint32_t) * local_history_key_num);

  global_history = 0;
}


uint8_t tournament_predictor(uint32_t pc) {
  pc = GET_LOWER_K_BITS(pc, pcIndexBits);
  uint8_t choice_of_pc = get_counter_by_index(two_bits_counters_choice, pc);
  uint8_t prediction = SN;

  if (choice_of_pc == SN || choice_of_pc == WN) {
      prediction = get_counter_by_index(two_bits_counters_global, global_history);
  } else {
      uint32_t local_history_of_pc = local_history[pc];
      prediction = get_counter_by_index(two_bits_counters_local, local_history_of_pc);
  }
  
  if (prediction == SN || prediction == WN) {
    return NOTTAKEN;
  } else {
    return TAKEN;
  }
}

void tournament_trainer(uint32_t pc, uint8_t outcome) {
  pc = GET_LOWER_K_BITS(pc, pcIndexBits);
  uint8_t choice_of_pc = get_counter_by_index(two_bits_counters_choice, pc);
  uint8_t pred_global = get_counter_by_index(two_bits_counters_global, global_history);
  pred_global = (pred_global == WN || pred_global == SN) ? NOTTAKEN : TAKEN;

  uint32_t local_history_of_pc = local_history[pc];
  uint8_t pred_local = get_counter_by_index(two_bits_counters_local, local_history_of_pc);
  pred_local = (pred_local == WN || pred_local == SN) ? NOTTAKEN : TAKEN;

  // Global is better, so we decrease choice preferring the global strategy
  if (pred_global == outcome && pred_local != outcome) {
    decrease_counter_by_index(two_bits_counters_choice, pc);
  }
  // Local is better, so we increase choice preferring the local strategy
  if (pred_global != outcome && pred_local == outcome) {
    increase_counter_by_index(two_bits_counters_choice, pc);
  }
  
  // Update history records and counters
  if (outcome == TAKEN) {
    increase_counter_by_index(two_bits_counters_global, global_history);
    increase_counter_by_index(two_bits_counters_local, local_history_of_pc);

    global_history = (global_history << 1) | 1;
    local_history[pc] = (local_history_of_pc << 1) | 1;
  } else {
    decrease_counter_by_index(two_bits_counters_global, global_history);
    decrease_counter_by_index(two_bits_counters_local, local_history_of_pc);

    global_history = (global_history << 1);
    local_history[pc] = (local_history_of_pc << 1);
  }

  global_history = GET_LOWER_K_BITS(global_history, ghistoryBits);
  local_history[pc] = GET_LOWER_K_BITS(local_history[pc], lhistoryBits);
}

// Custom Predictor
double* weights;
double* biases;

double lr = 1e-2;

double sigmoid(double x) {
  return 1.0 / (1.0 + exp(-x));
}

void custom_initializer() {
  ghistoryBits = 16;
  lhistoryBits = 16;
  pcIndexBits = 10;

  uint32_t pc_num = 1 << pcIndexBits;

  local_history = (uint32_t*) malloc(sizeof(uint32_t) * pc_num);
  memset(local_history, 0, sizeof(uint32_t) * pc_num);

  global_history = 0;

  biases = (double*) malloc(sizeof(double) * pc_num);


  for (uint32_t i = 0; i < pc_num; ++i) {
    biases[i] = 0.0;
  }

  uint32_t weight_dim = ghistoryBits + lhistoryBits;
  weights = (double*) malloc(sizeof(double) * weight_dim);
  for (uint32_t i = 0; i < weight_dim; ++i) {
    weights[i] = -1.0 + (2.0 * rand() / RAND_MAX);
  }
}

uint8_t custom_predictor(uint32_t pc) {
  pc = GET_LOWER_K_BITS(pc, pcIndexBits);
  double bias = biases[pc];

  uint32_t local_history_of_pc = local_history[pc];
  uint32_t feature = (global_history << 16) | (local_history_of_pc);

  double logits = 0.0;
  for (uint32_t i = 0; i < 32; ++i) {
    logits += GET_BIT(feature, i) ? weights[i] : 0.0;
  }

  logits += bias;
  
  if (logits >= 0.) {
    return TAKEN;
  } else {
    return NOTTAKEN;
  }
}

void custom_trainer(uint32_t pc, uint8_t outcome) {
  pc = GET_LOWER_K_BITS(pc, pcIndexBits);
  double bias = biases[pc];
  uint32_t local_history_of_pc = local_history[pc];
  uint32_t feature = (global_history << 16) | (local_history_of_pc);

  double logits = 0.0;
  for (uint32_t i = 0; i < 32; ++i) {
    logits += GET_BIT(feature, i) ? weights[i] : 0.0;
  }

  logits += bias;
  double pred = sigmoid(logits);
  
  // Update history records using gradient descent
  if (outcome == TAKEN) {
    for (uint32_t i = 0; i < 32; ++i) {
        double xi = GET_BIT(feature, i) ? 1.0 : 0.0;
        weights[i] = weights[i] + lr * (1 - pred) * xi;
        biases[pc] = weights[i] + lr * (1 - pred);
    }

    global_history = (global_history << 1) | 1;
    local_history[pc] = (local_history_of_pc << 1) | 1;
  } else {
    for (uint32_t i = 0; i < 32; ++i) {
        double xi = GET_BIT(feature, i) ? 1.0 : 0.0;
        weights[i] = weights[i] - lr * pred * xi;
        biases[pc] = weights[i] - lr * pred;
    }

    global_history = (global_history << 1);
    local_history[pc] = (local_history_of_pc << 1);
  }

  global_history = GET_LOWER_K_BITS(global_history, ghistoryBits);
  local_history[pc] = GET_LOWER_K_BITS(local_history[pc], lhistoryBits);
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      gshare_initializer();
      break;
    case TOURNAMENT:
      tournament_initializer();
      break;
    case CUSTOM:
      custom_initializer();
      break;
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predictor(pc);
    case TOURNAMENT:
      return tournament_predictor(pc);
    case CUSTOM:
      return custom_predictor(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      gshare_trainer(pc, outcome);
      break;
    case TOURNAMENT:
      tournament_trainer(pc, outcome);
      break;
    case CUSTOM:
      custom_trainer(pc, outcome);
      break;
    default:
      break;
  }
}
