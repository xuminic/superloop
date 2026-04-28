/**
  ******************************************************************************
  * @file           : board.c
  * @brief          : the board abstract interface
  ******************************************************************************
*/

#ifdef	SIMULATOR
#include "board_simulator.inc"
#else
#include "board_stm32f4xx.inc"
#endif

