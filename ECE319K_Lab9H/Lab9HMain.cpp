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
#include "sounds/sounds.h"

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
static void drawStr(int16_t startX, int16_t y, const char *s,
                    int16_t color, uint8_t size) {
  for(int16_t cx = startX; *s; s++, cx += 6*size)
    ST7735_DrawChar(cx, y, *s, color, ST7735_BLACK, size);
}
void drawStrCentered(int16_t x, int16_t y, const char *s, int16_t color, uint8_t size) {
    uint32_t len = 0;
    const char *p = s;
    while(*p++) len++; 
    int16_t totalWidth = len * 6 * size;
    int16_t centerX = (160 - totalWidth) / 2;
    if(centerX < 0) centerX = 0;

    drawStr(centerX, y, s, color, size);
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
bool playersReady = false;
uint8_t p1Wins = 0, p2Wins = 0;

typedef enum {
    ENGLISH = 0,
    SPANISH = 1,
    NUM_LANGUAGES
} Language_t;

Language_t currentLanguage = ENGLISH;

typedef enum {
    STR_MORTAL,
    STR_KOMBAT,
    STR_START,
    STR_ROUND,
    STR_FIGHT,
    STR_GAME,
    STR_OVER,
    STR_PLAYER1,
    STR_PLAYER2,
    STR_BEST3,
    STR_WINNER,
    STR_SELECTCHAR,
    STR_READY,
    NUM_PHRASES
} Phrase_t;

const char* const Phrases[NUM_PHRASES][NUM_LANGUAGES] = {
    [STR_MORTAL]    = {"MORTAL", "MORTAL"},
    [STR_KOMBAT]    = {"KOMBAT", "KOMBAT"},
    [STR_START]     = {"PRESS BUTTON", "PULSA BOTON"},
    [STR_ROUND]     = {"ROUND", "RONDA"},
    [STR_FIGHT]     = {"TO FIGHT!", "\xAD""A LUCHAR!"},
    [STR_GAME] = {"GAME", "JUEGO"},
    [STR_OVER]= {"OVER", "TERMINADO"},
    [STR_PLAYER1] = {"PLAYER 1", "JUGADOR 1"},
    [STR_PLAYER2] = {"PLAYER 2", "JUGADOR 2"},
    [STR_BEST3]     = {"BEST OF 3", "LO MEJOR DE 3"},
    [STR_SELECTCHAR] = {"SELECT CHARACTER","SELECCIONAR PERSONAJE"},
    [STR_READY] = {"READY", "LISTA"},
    [STR_WINNER]    = {"WINS!", "\xADGANA!"}
};

// ── Character select constants 
static const int16_t SEL_SX     = 28;              // x top-edge of all slot boxes
static const int16_t SEL_SY[3]  = {5, 62, 119};   // y left-edge of each slot box
static const int16_t SEL_SZ     = 36;              // box outer size (32×32 sprite inside 2px border)
static const char* const CHAR_NAMES[3] = {"CHAR 1", "CHAR 2", "CHAR 3"};
static const SpriteSet* const CHAR_OPTIONS[3] = {
    &CHAR1_SPRITES, &CHAR2_SPRITES, &CHAR3_SPRITES
};

// Draw a 2-px border box for one slot in the given color (rotation 0 coords).
static void drawSlotBorder(uint8_t slot, uint16_t color) {
    int16_t sx = SEL_SX, sy = SEL_SY[slot], sz = SEL_SZ;
    ST7735_FillRect(sx,        sy,        2,  sz, color);
    ST7735_FillRect(sx+sz-2,   sy,        2,  sz, color);
    ST7735_FillRect(sx+2,      sy,        sz-4, 2, color);
    ST7735_FillRect(sx+2,      sy+sz-2,   sz-4, 2, color);
}

// Redraw all three slot borders to reflect current selections.
static void refreshSlotBorders(uint8_t p1Sel, uint8_t p2Sel) {
    for (uint8_t i = 0; i < 3; i++) drawSlotBorder(i, 0x4228);  // default dim grey
    drawSlotBorder(p2Sel, ST7735_RED);
    drawSlotBorder(p1Sel, ST7735_BLUE);
}

// Show character select screen; blocks until both players pick.
void charSelectScreen(void) {
    ST7735_SetRotation(0);
    ST7735_FillScreen(ST7735_BLACK);
    for (uint8_t i = 0; i < 3; i++) {
        const SPRITE_ARRAY* spr = CHAR_OPTIONS[i]->idle;
        ST7735_DrawBitmap(SEL_SX+2, SEL_SY[i]+2 + spr->HEIGHT - 1,
                          spr->arr, spr->WIDTH, spr->HEIGHT);
    }

    //Horizontal text in rotation 1
    ST7735_SetRotation(1);

    // Title bar
    drawStrCentered(32, 2, Phrases[STR_SELECTCHAR][currentLanguage], ST7735_WHITE, 1);
    ST7735_FillRect(10, 12, 140, 1, 0x4228);
    for (uint8_t i = 0; i < 3; i++) {
        drawStr(SEL_SY[i], 50, CHAR_NAMES[i], 0x8410, 1);
    }

    // Control hints
    drawStr(10, 30, "BLK:CYCLE  KICK:SELECT", 0x4228, 1);
    ST7735_SetRotation(0);

    // Initial cursor positions: P1 on slot 0, P2 on slot 1
    uint8_t p1Sel = 0, p2Sel = 1;
    bool p1Confirmed = false, p2Confirmed = false;
    refreshSlotBorders(p1Sel, p2Sel);

    uint32_t prevSw = Switch_In();

    while (!p1Confirmed || !p2Confirmed) {
        Clock_Delay1ms(40);
        uint32_t sw     = Switch_In();
        uint32_t rising = sw & ~prevSw;
        prevSw = sw;

        bool needRedraw = false;

        //Player 1
        if (!p1Confirmed) {
            if (rising & 0x04) { p1Sel = (p1Sel + 1) % 3; needRedraw = true; }
            if (rising & 0x02) {
                p1Confirmed = true; needRedraw = true;
                ST7735_SetRotation(1);
                drawStr(SEL_SY[p1Sel], SEL_SX + SEL_SZ + 30, Phrases[STR_READY][currentLanguage], ST7735_BLUE, 1);
                ST7735_SetRotation(0);
            }
        }

        //Player 2
        if (!p2Confirmed) {
            if (rising & 0x10) { p2Sel = (p2Sel + 1) % 3; needRedraw = true; }
            if (rising & 0x20) {
                p2Confirmed = true; needRedraw = true;
                ST7735_SetRotation(1);
                drawStr(SEL_SY[p2Sel], SEL_SX + SEL_SZ + 39, Phrases[STR_READY][currentLanguage], ST7735_RED, 1);
                ST7735_SetRotation(0);
            }
        }

        if (needRedraw) refreshSlotBorders(p1Sel, p2Sel);
    }

    // Apply chosen sprites
    Player1.setSpriteSet(*CHAR_OPTIONS[p1Sel]);
    Player2.setSpriteSet(*CHAR_OPTIONS[p2Sel]);
    playersReady = true;
}

void initStartScreen(void)
{
  Sound_MusicStart(bgMusic,BG_MUSIC_LEN);

  ST7735_SetRotation(1);
  ST7735_FillScreen(ST7735_BLACK);

  // Gold border bars
  ST7735_FillRect(0,   0, 160, 6, ST7735_YELLOW);
  ST7735_FillRect(0, 122, 160, 6, ST7735_YELLOW);

  drawStr(26, 14, "MORTAL", ST7735_YELLOW, 3);
  drawStr(26, 42, "KOMBAT", ST7735_RED,    3);

  ST7735_FillRect(10, 72, 140, 2, ST7735_YELLOW);

  drawStrCentered(53, 77, Phrases[STR_BEST3][currentLanguage], ST7735_WHITE, 1);

  drawStrCentered(32, 92, Phrases[STR_START][currentLanguage], ST7735_WHITE,  1);
  drawStrCentered(56, 104, Phrases[STR_FIGHT][currentLanguage],        ST7735_YELLOW, 1);
}
volatile bool drawScreen = false;
uint32_t Data1, Data2;
int16_t pos1, pos2;

bool p1PPast, p1KPast, p1BPast, p2PPast, p2KPast, p2BPast, p1Punch, p1Kick, p1Block, p2Punch, p2Kick, p2Block;
uint32_t p1BlockTicks = 0, p2BlockTicks = 0; // counts ticks block has been held (30Hz, max 3s = 90)
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

    p1Punch = (sw & 0x01) && !p1PPast;
    p1Kick  = (sw & 0x02) && !p1KPast;
    p1Block = (sw & 0x04) && !p1BPast;
    p2Punch = (sw & 0x08) && !p2PPast;
    p2Block = (sw & 0x10) && !p2BPast;
    p2Kick  = (sw & 0x20) && !p2KPast;

    // save raw state for next tick
    p1PPast = (sw & 0x01);
    p1KPast = (sw & 0x02);
    p1BPast = (sw & 0x04);
    p2PPast = (sw & 0x08);
    p2BPast = (sw & 0x10);
    p2KPast = (sw & 0x20);

    // count ticks block is held; reset when released (cap at 91 so condition p<=90 expires)
    if (p1BPast) { if (p1BlockTicks <= 90) p1BlockTicks++; } else p1BlockTicks = 0;
    if (p2BPast) { if (p2BlockTicks <= 90) p2BlockTicks++; } else p2BlockTicks = 0;

    if (inStartScreen) {
      uint32_t sw = Switch_In();
      if (sw & 0x04) { // For example, use the P1 Block button to toggle
          currentLanguage = (currentLanguage == ENGLISH) ? SPANISH : ENGLISH;
          initStartScreen();
    }
}
    if (inStartScreen && (sw&0x10)) //Use P2 block button to start
    {
        inStartScreen = false;
    }
    else if (fighting)
    {
      if(p1Kick)                              p1State = CharacterState::KICK;
      else if(p1Punch)                        p1State = CharacterState::PUNCH;
      else if(p1BPast && p1BlockTicks <= 90)  p1State = CharacterState::DODGE;
      else                                    p1State = CharacterState::IDLE;

      if(p2Kick)                              p2State = CharacterState::KICK;
      else if(p2Punch)                        p2State = CharacterState::PUNCH;
      else if(p2BPast && p2BlockTicks <= 90)  p2State = CharacterState::DODGE;
      else                                    p2State = CharacterState::IDLE;


      // 1) sample slide pot
      // 2) read input switches
      // 3) move sprites
      // 4) start sounds
      // 5) set semaphore
      drawScreen = true;
    }
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
// Draw a string at pixel (startX, y) with given size and color.


static uint16_t healthColor(int16_t hp) {
    if (hp > 60) return 0x07E0;      // green
    if (hp > 30) return ST7735_YELLOW;
    return ST7735_RED;
}

#define DMG_TICKS 20   // ~0.67s at 30 Hz
#define DMG_X     55   // x pixel col for damage text (safe: chars are at x=20..51)

static void showDmgNum(int16_t py, int16_t amt) {
    if (py > 152) py = 152;
    char buf[4] = {'-', 0, 0, 0};
    if (amt >= 10) { buf[1]='1'; buf[2]='0'; }
    else           { buf[1]=(char)('0'+amt); }
    drawStr(DMG_X, py, buf, ST7735_YELLOW, 1);
}

static void eraseDmgNum(int16_t py, int16_t amt) {
    if (py > 152) py = 152;
    ST7735_FillRect(DMG_X, py, (amt >= 10) ? 18 : 12, 8, ST7735_BLACK);
}

// Draws the current series score (e.g. "1-0") in the VS gap of the health panel.
void drawScore(void) {
    char buf[4] = {(char)('0'+p1Wins), '-', (char)('0'+p2Wins), 0};
    ST7735_FillRect(100, 72, 28, 21, ST7735_BLACK);  // clear the VS gap area
    ST7735_SetRotation(1);
    drawStr(65, 35, buf, ST7735_YELLOW, 2);           // centered: 3×6=18px → x=109
    ST7735_SetRotation(0);
}

// Shows "ROUND N / FIGHT!" in landscape for 3 seconds, then returns.
void roundScreen(uint8_t round) {
    Sound_MusicStop();

    ST7735_SetRotation(1);
    ST7735_FillScreen(ST7735_BLACK);

    ST7735_FillRect(0,   0, 160, 6, ST7735_WHITE);
    ST7735_FillRect(0, 122, 160, 6, ST7735_WHITE);

    drawStrCentered(35, 18, Phrases[STR_ROUND][currentLanguage], ST7735_WHITE, 3);

    char rnum[2] = {(char)('0' + round), 0};
    drawStr(71, 50, rnum, ST7735_YELLOW, 3);

    ST7735_FillRect(20, 80, 120, 2, 0x8410);

    drawStrCentered(44, 88, Phrases[STR_FIGHT][currentLanguage], ST7735_RED, 2);

    Clock_Delay1ms(3000);
}



void deathScreen(void)
{

  ST7735_SetRotation(1); 
  ST7735_FillScreen(ST7735_BLACK);

  // Red border bars
  ST7735_FillRect(0,   0, 160, 6, ST7735_RED);
  ST7735_FillRect(0, 122, 160, 6, ST7735_RED);
  drawStrCentered(44, 12, Phrases[STR_GAME][currentLanguage], ST7735_RED, 2);
  drawStrCentered(44, 40, Phrases[STR_OVER][currentLanguage], ST7735_RED, 2);

  // Thin separator
  ST7735_FillRect(10, 70, 140, 2, ST7735_RED);

  const char *who = (p1Wins >= 2) ? Phrases[STR_PLAYER1][currentLanguage] : Phrases[STR_PLAYER2][currentLanguage];
  drawStrCentered(32, 82, who,    ST7735_YELLOW, 2);
  drawStrCentered(50, 102, Phrases[STR_WINNER][currentLanguage], ST7735_YELLOW, 2);
}

void initScreen(void)
{
  ST7735_InitPrintf(INITR_REDTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
}
void updateHealth(void)
{
  int16_t hp1 = Player1.getHealth();
  int16_t hp2 = Player2.getHealth();

  // P1 bar: fills from top, drains downward
  int16_t fill1 = hp1 * 56 / 100;
  ST7735_FillRect(104, 14,         20, fill1,      healthColor(hp1));
  ST7735_FillRect(104, 14 + fill1, 20, 56 - fill1, ST7735_BLACK);

  // P2 bar: fills from top, drains downward
  int16_t fill2 = hp2 * 56 / 100;
  ST7735_FillRect(104, 95,         20, fill2,      healthColor(hp2));
  ST7735_FillRect(104, 95 + fill2, 20, 56 - fill2, ST7735_BLACK);
}

void initGame(void)
{

  ST7735_SetRotation(0); 
  ST7735_FillScreen(ST7735_BLACK);

  // Ground strip with bright highlight edge
  ST7735_FillRect( 0, 0, 18, 160, ST7735_DARKGREY);
  ST7735_FillRect(18, 0,  2, 160, 0x8410);

  // Arena-to-panel divider
  ST7735_FillRect(98, 0,  2, 160, 0x4228);

  // P1 health section (top half of panel)
  drawStr(108, 2,  "P1", ST7735_WHITE, 1);
  ST7735_FillRect(102, 12, 24, 60, 0x4228);       // frame border
  ST7735_FillRect(104, 14, 20, 56, ST7735_BLACK); // inner fill area

  // Score in the VS gap — updated by drawScore() each round
  drawScore();

  // P2 health section (bottom half of panel)
  drawStr(108, 83, "P2", ST7735_WHITE, 1);
  ST7735_FillRect(102, 93, 24, 60, 0x4228);       // frame border
  ST7735_FillRect(104, 95, 20, 56, ST7735_BLACK); // inner fill area

  updateHealth();
}




uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

// typedef enum {English, Spanish, Portuguese, French} Language_t;
// Language_t myLanguage=English;
// typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
// const char Hello_English[] ="Hello";
// const char Hello_Spanish[] ="\xADHola!";
// const char Hello_Portuguese[] = "Ol\xA0";
// const char Hello_French[] ="All\x83";
// const char Goodbye_English[]="Goodbye";
// const char Goodbye_Spanish[]="Adi\xA2s";
// const char Goodbye_Portuguese[] = "Tchau";
// const char Goodbye_French[] = "Au revoir";
// const char Language_English[]="English";
// const char Language_Spanish[]="Espa\xA4ol";
// const char Language_Portuguese[]="Portugu\x88s";
// const char Language_French[]="Fran\x87" "ais";
// const char *Phrases[3][4]={
//   {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
//   {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
//   {Language_English,Language_Spanish,Language_Portuguese,Language_French}
// };
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
         //ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         //ST7735_OutString((char *)Phrases[myPhrase][myL]);
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
// void checkWin(){
//     if(Player1.getHealth() <= 0){
//         ST7735_FillScreen(ST7735_BLACK);
//         ST7735_OutString("Player 2 Wins!");
//         while(1){} // freeze game
//     }
//     if(Player2.getHealth() <= 0){
//         ST7735_FillScreen(ST7735_BLACK);
//         ST7735_OutString("Player 1 Wins!");
//         while(1){}
//     }
// }
int16_t myAbs(int16_t x){
    return (x < 0) ? -x : x;
}
// use main2 to observe graphics
int main(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Sound_Init();
  Sensor1.Init(5);
  Sensor2.Init(4);
  Switch_Init();        // must be before timer so ISR can read buttons on start screen
  initScreen();
  initStartScreen();
  TimerG12_IntArm(2666667, 0);
  __enable_irq();

  while(inStartScreen);

  charSelectScreen();   // blocks until both players pick; sets Player1/2 sprites + playersReady

  uint8_t currentRound = 1;

  while (p1Wins < 2 && p2Wins < 2) {
    roundScreen(currentRound);

    Player1.reset(startX, startY);
    Player2.reset(x, y);
    fighting = true;

    initGame();   // draws background, health bars, score
    Player1.draw();
    Player2.draw();

    int16_t dmgAmt1 = 0, dmgAmt2 = 0;
    uint8_t dmgTimer1 = 0, dmgTimer2 = 0;
    int16_t dmgY1 = 0, dmgY2 = 0;

    while (fighting) {
      while(!drawScreen);
      drawScreen = false;

      int16_t oldPos1 = Player1.getY();
      int16_t oldPos2 = Player2.getY();

      Player1.setPosition(Player1.getX(), pos1);
      Player2.setPosition(Player2.getX(), pos2);

      if(myAbs(Player1.getY() - Player2.getY()) < 20){
          if(pos1 < pos2) Player1.setPosition(Player1.getX(), pos2 - 20);
          else            Player2.setPosition(Player2.getX(), pos1 - 20);
      }

      CharacterState prev1 = Player1.getState();
      CharacterState prev2 = Player2.getState();

      Player1.update(p1State);
      Player2.update(p2State);

      if(p1State == CharacterState::PUNCH || p1State == CharacterState::KICK){
        Sound_Start(attackSound, ATTACK_SOUND_LEN);
          if(Player1.checkHit(Player2.getX(), Player2.getY())){
              if(Player2.takeDmg(p1State)){
                  updateHealth();
                  if(dmgTimer2 > 0) eraseDmgNum(dmgY2, dmgAmt2);
                  dmgAmt2 = (p1State == CharacterState::KICK) ? 10 : 5;
                  dmgTimer2 = DMG_TICKS;
                  dmgY2 = Player2.getY();
                  showDmgNum(dmgY2, dmgAmt2);
              }
          }
      }
      if(p2State == CharacterState::PUNCH || p2State == CharacterState::KICK){
          Sound_Start(attackSound, ATTACK_SOUND_LEN);
          if(Player2.checkHit(Player1.getX(), Player1.getY())){
              if(Player1.takeDmg(p2State)){
                  updateHealth();
                  if(dmgTimer1 > 0) eraseDmgNum(dmgY1, dmgAmt1);
                  dmgAmt1 = (p2State == CharacterState::KICK) ? 10 : 5;
                  dmgTimer1 = DMG_TICKS;
                  dmgY1 = Player1.getY();
                  showDmgNum(dmgY1, dmgAmt1);
              }
          }
      }

      if(dmgTimer1 > 0) { --dmgTimer1; if(dmgTimer1 == 0) eraseDmgNum(dmgY1, dmgAmt1); }
      if(dmgTimer2 > 0) { --dmgTimer2; if(dmgTimer2 == 0) eraseDmgNum(dmgY2, dmgAmt2); }

      if (Player1.getHealth() == 0 || Player2.getHealth() == 0) {
        if (Player1.getHealth() == 0) p2Wins++;
        else                          p1Wins++;
        drawScore();
        fighting = false;
        Clock_Delay1ms(1500);   // brief pause before next round / win screen
      }

      bool p1Changed = (Player1.getY() != oldPos1 || Player1.getState() != prev1);
      bool p2Changed = (Player2.getY() != oldPos2 || Player2.getState() != prev2);
      if(p1Changed) Player1.draw();
      if(p2Changed) Player2.draw();
    }

    currentRound++;
  }

  deathScreen();
  Clock_Delay1ms(1000);
  while(1);
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
