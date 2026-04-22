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
    //Player 2
  IOMUX->SECCFG.PINCM[PA28INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA27INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA17INDEX] = 0x00050081; // input, pull down

  //Player 1
  IOMUX->SECCFG.PINCM[PA8INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA18INDEX] = 0x00050081; // input, pull down
  IOMUX->SECCFG.PINCM[PA24INDEX] = 0x00050081; // input, pull down

}
// return current state of switches
uint32_t Switch_In(void){
    uint32_t data = GPIOA->DIN31_0;
    uint32_t result = 0;
    if(data & (1<<8))  result |= 0x01;  // PA8  → bit0 P1 Punch
    if(data & (1<<24)) result |= 0x02;  // PA24 → bit1 P1 Kick
    if(data & (1<<18)) result |= 0x04;  // PA18 → bit2 P1 Block
    if(data & (1<<27)) result |= 0x08;  // PA27 → bit3 P2 Punch
    if(data & (1<<28)) result |= 0x10;  // PA28 → bit4 P2 Block
    if(data & (1<<17)) result |= 0x20;  // PA17 → bit5 P2 Kick
    return result;
}