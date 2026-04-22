/*
 * Switch.cpp
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "./inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    // write this
    //Player 1
  IOMUX->SECCFG.PINCM[PA28INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA27INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA17INDEX] = 0x00050081; // input, pull down

  //Player 2
  IOMUX->SECCFG.PINCM[PA8INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA18INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA24INDEX] = 0x00050081; // input, pull down

}
// return current state of switches
uint32_t Switch_In(void){
    // write this
  uint32_t data = GPIOA->DIN31_0;
  data = ((data>>15)&0x07) | ((data&((1<<28)|(1<<27)))>>25);
  return data; // return 0; //replace this your code
}
