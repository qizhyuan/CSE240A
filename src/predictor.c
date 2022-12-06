//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
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
// Get the bit-th bit of x
#define GET_BIT(x, bit) ((x >> bit) & 1)

// Get the lower k bits of x
#define GET_LOWER_K_BITS(x, k) (x & ((1 << k) - 1))

// Set the bit-th bit of x into y
#define SET_BIT(x, bit, y) ((x & ~ (1 << bit)) | (y << bit))

uint32_t globalHistory;
uint8_t* twoBitsCounters;


uint32_t get_index(uint32_t pc) {
  return globalHistory ^ pc;
}

uint8_t get_counter(uint32_t index) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;
  
  uint32_t item = twoBitsCounters[array_index];
  uint8_t counter = GET_BIT(item, item_index << 1) | GET_BIT(item, item_index << 1 + 1);
  return counter;
}

void set_counter(uint32_t index, uint8_t counter) {
  uint32_t array_index = index >> 2;
  uint32_t item_index = index & 0b11;
  uint32_t item = twoBitsCounters[array_index];
  
  item = SET_BIT(item, item_index << 1, counter & 0b01);
  item = SET_BIT(item, item_index << 1 + 1, counter & 0b10);

  twoBitsCounters[array_index] = item;
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
  twoBitsCounters = (uint8_t*) malloc(sizeof(uint8_t) * counter_arr_size);
  memset(twoBitsCounters, 0, sizeof(uint8_t) * counter_arr_size);
  globalHistory = 0;
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

  if (outcome == NOTTAKEN) {
    decrease_counter(index);
    globalHistory = globalHistory << 1 | 1;
  } else {
    increase_counter(index);
    globalHistory = globalHistory >> 1;
  }

  globalHistory = GET_LOWER_K_BITS(globalHistory, ghistoryBits);
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
    case GSHARE:
      gshare_initializer();
    case TOURNAMENT:
    case CUSTOM:
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
    case CUSTOM:
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
    case GSHARE:
      gshare_trainer(pc, outcome);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}
