// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Your name
// Last Modified: January 12, 2026

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "./inc/ST7735.h"
#include "./inc/Clock.h"
#include "./inc/LaunchPad.h"
#include "./inc/TExaS.h"
#include "./inc/Timer.h"
#include "./inc/SlidePot.h"
#include "./inc/DAC5.h"
#include "./inc/SmallFont.h"
#include "./inc/LED.h"
#include "./inc/Switch.h"
#include "./inc/Sound.h"
#include "images/images.h"
#include "./inc/Character.h"
#include "./inc/sprite_data.h"

extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);

// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz


/*
Pin assignments
Player 1
Slidepot: PB17
Punch: PA8
Kick: PA24
Block: PA18 

Player 2
Slidepot: PB18
Punch: PA27
Block: PA28
Kick: PA17

*/

void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}
uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

SlidePot Sensor1(1667,337); // copy calibration from Lab 7
SlidePot Sensor2(1667, 337);
int16_t startX = 20;
int16_t startY = 0; //Put in bottom left
uint32_t interruptCounter = 0;
Character Player1(startX, startY, CHAR1_SPRITES);

int16_t x = 20;
int16_t y = 160 - CHAR2_SPRITES.idle->WIDTH;
Character Player2(x, y, CHAR2_SPRITES); //Put in bottom right
CharacterState p1State = CharacterState::IDLE;
CharacterState p2State = CharacterState::IDLE;

bool inStartScreen = true;
bool fighting = true;

void initStartScreen(void)
{
  //ST7735_SetRotation(3);
  // ST7735_DrawString(50, 50,"Mortal Kombat", ST7735_WHITE);
  // ST7735_DrawString(40, 10, "Press to Start", ST7735_YELLOW);
  //ST7735_SetRotation(0);
  ST7735_OutString("Mortal Kombat\n");
  ST7735_OutString("Press to Start");
}

void deathScreen(void)
{
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_OutString("Game Over\n");

  if (Player1.getHealth() == 0)
  {
    ST7735_OutString("Player 2 Won!\n");
  }
  else
  {
    ST7735_OutString("Player 1 Won!\n");
  }
}

void initScreen(void)
{
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
}

void initGame(void)
{
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_FillRect(0, 0, 20, 160, ST7735_DARKGREY); //floor

  ST7735_FillRect(100, 0, 10, 50, ST7735_RED); //Player1 health bar
  ST7735_FillRect(100, 110, 10, 50, ST7735_RED); //Player2 health bar
}

void updateHealth(void)
{
  //Update Player1 health
  int16_t health1 = Player1.getHealth();
  int16_t health2 = 100 - (Player2.getHealth());

  ST7735_FillRect(100, 0, 10, 50, ST7735_BLACK); //Player1 health bar
  ST7735_FillRect(100, 110, 10, 50, ST7735_BLACK); //Player2 health bar

  ST7735_FillRect(100, 0, 10, health1 / 2, ST7735_RED); //Player1 health bar
  ST7735_FillRect(100, 110 + health2 / 2, 10, 50, ST7735_RED); //Player2 health bar
}

bool drawScreen = false;
uint32_t Data1, Data2;
int16_t pos1, pos2;
// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
  interruptCounter++;
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here
    Data2=Sensor2.SlidePot_Running();
    Data1 = Sensor1.SlidePot_Running();
    pos2 = (Data2 * (160 - 18)) / 4095;
    pos1 = 128-(Data1 * (160 - 32)) / 4095;
 
    uint32_t sw = Switch_In();

    // Player 1 — active low so 0 = pressed
    bool p1Punch = (sw & 0x01);  // PA8
    bool p1Kick  = (sw & 0x02);  // PA24
    bool p1Block = (sw & 0x04);  // PA18

    // Player 2 — active high so 1 = pressed
    bool p2Punch = (sw & 0x08);  // PA27
    bool p2Block = (sw & 0x10);  // PA28
    bool p2Kick  = (sw & 0x20);  // PA17

    if (inStartScreen && sw)
    {
        inStartScreen = false;
    }
    else if (fighting)
    {
    
    
      if(p1Kick)       p1State = CharacterState::KICK;
      else if(p1Punch) p1State = CharacterState::PUNCH;
      else if(p1Block) p1State = CharacterState::DODGE;
      else             p1State = CharacterState::IDLE;

      if(p2Kick)       p2State = CharacterState::KICK;
      else if(p2Punch) p2State = CharacterState::PUNCH;
      else if(p2Block) p2State = CharacterState::DODGE;
      else             p2State = CharacterState::IDLE;


      // 1) sample slide pot
      // 2) read input switches
      // 3) move sprites
      // 4) start sounds
      // 5) set semaphore
      drawScreen = true;
    }
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};
// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(0x0000);            // set screen to black
  for(int myPhrase=0; myPhrase<= 2; myPhrase++){
    for(int myL=0; myL<= 3; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
int main(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Sensor1.Init(5);
  Sensor2.Init(4);
  initScreen();
  initStartScreen();
  TimerG12_IntArm(2666667, 0);
  __enable_irq();

  while(inStartScreen);
  initGame();
  Switch_Init();
  Player1.draw();
  Player2.draw();
  while(1)
  {
    /*if (Player1.checkHit(Player2.getX(), Player2.getY()))
    {continue;} //Cat stops moving once it hits the other dude
    Player2.moveX(-10);
    Player2.draw();
    Player2.takeDmg(CharacterState::KICK);
    Player1.takeDmg(CharacterState::KICK);
    updateHealth();*/
    if(drawScreen) {
      drawScreen = false;

      // int16_t oldPos1 = Player1.getY();
      // int16_t oldPos2 = Player2.getY();
      // CharacterState oldState1 = p1State;
      // CharacterState oldState2 = p2State;

      Player1.setPosition(Player1.getX(), pos1);
      Player2.setPosition(Player2.getX(), pos2);
      Player1.update(p1State);
      Player2.update(p2State);

      // bool p1Changed = (Player1.getY() != oldPos1 || p1State != oldState1);
      // bool p2Changed = (Player2.getY() != oldPos2 || p2State != oldState2);

      // only erase/redraw what actually changed
      if(1) Player1.draw();
      if(1) Player2.draw();
      //if(p1Changed) Player1.redraw();
      //if(p2Changed) Player2.redraw();
    }
    if (Player1.getHealth() == 0 || Player2.getHealth() == 0)
    {
      fighting = false;
      deathScreen();
      Clock_Delay1ms(1000);
    }
    //Clock_Delay1ms(16);

  }
}

// use main3 to test switches and LEDs
int main3(void){ // main3
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  TimerG12_IntArm(2666667, 0);
  Sensor1.Init(5);
  Sensor2.Init(4);
  //Player1.draw();
  __enable_irq();
  while(1){
    // write code to test switches and LEDs
    Sensor1.Sync();
    Sensor2.Sync();

  }
}
// use main4 to test sound outputs
int main4(void){ uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  __enable_irq();
  while(1){
    now = Switch_In(); // one of your buttons
    if((last == 0)&&(now == 1)){
      Sound_Shoot(); // call one of your sounds
    }
    if((last == 0)&&(now == 2)){
      Sound_Killed(); // call one of your sounds
    }
    if((last == 0)&&(now == 4)){
      Sound_Explosion(); // call one of your sounds
    }
    if((last == 0)&&(now == 8)){
      Sound_Fastinvader1(); // call one of your sounds
    }
    // modify this to test all your sounds
  }
}
// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main5(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
  Sensor1.Init(5); // PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  
  // initialize all data structures
  __enable_irq();

  while(1){
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
  }
}
