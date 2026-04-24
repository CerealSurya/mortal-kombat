#include "./inc/Character.h"
#include "./inc/ST7735.h"
#include "./inc/sprite_data.h"

Character::Character(int16_t startX, int16_t startY, const SpriteSet& sprites)
    : x(startX), y(startY),
      prevX(startX), prevY(startY),
      state(CharacterState::IDLE),
      facing(Direction::RIGHT),
      actionTimer(0),
      currentSprite(sprites.idle),
      prevSprite(sprites.idle),
      spriteIdle(sprites.idle),
      spritePunch(sprites.punch),
      spriteKick(sprites.kick),
      spriteDodge(sprites.dodge),
      health(100),
      isAlive(true)
{}

bool Character::takeDmg(CharacterState attack)
{
    if(state == CharacterState::DODGE) return false;
    if (attack == CharacterState::KICK)
    {
        health -= 10;
        moveX(10); //Knockback
    }
    else if (attack == CharacterState::PUNCH)
    {
        health -= 5;
        moveX(5);
    }
    if (health <= 0)
    {
        health = 0;
        isAlive = false;
    }
    return 1;
}
void Character::erase(){
    ST7735_FillRect(prevX, prevY, prevSprite->WIDTH, prevSprite->HEIGHT, ST7735_BLACK);
}

void Character::redraw(){
    selectSprite();
    blitSprite(x, y, currentSprite, true, SPRITE_TRANSPARENT_COLOR);
    prevX = x;
    prevY = y;
    prevSprite = currentSprite;
}

bool Character::checkHit(int16_t eX, int16_t eY)
{
    //Assume that eX and eY is the x, y coords of the other character
    //Creates (32x32) square based on those x,y coords and determines if there is any overlap with our own coords

    int16_t ourLeft   = x;
    int16_t ourRight  = x  + 32;
    int16_t ourBottom = y;
    int16_t ourTop    = y  + 32;

    int16_t eLeft     = eX;
    int16_t eRight    = eX + 32;
    int16_t eBottom   = eY;
    int16_t eTop      = eY + 32;

    //Checks no overlap
    bool noOverlap = (ourRight  <= eLeft)
                  || (ourLeft   >= eRight)
                  || (ourTop    <= eBottom) 
                  || (ourBottom >= eTop);

    return !noOverlap;
}

void Character::moveX(int16_t amount)
{ //Swapped x, y because horizontal screen
    y += amount;

    if (amount > 0) facing = Direction::RIGHT; //DO WE NEED THIs???
    else if (amount < 0) facing = Direction::LEFT;
}

void Character::moveY(int16_t amount)
{
    x += amount;
}

void Character::update(CharacterState requestedState)
{
    if (state == CharacterState::IDLE) {
        if (requestedState != CharacterState::IDLE)
        {
            state = requestedState;
            actionTimer = FRAME_HOLD_TICKS;
        }
    }
    else if (state == CharacterState::DODGE && requestedState == CharacterState::DODGE)
    {
        // Held block: keep DODGE as long as the ISR keeps requesting it
    }
    else
    {
        if (actionTimer > 0) {
            actionTimer--;
        }
        if (actionTimer == 0) {
            state = CharacterState::IDLE;
        }
    }
    //Bound
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > ST7735_TFTWIDTH  - currentSprite->WIDTH)  x = ST7735_TFTWIDTH  - currentSprite->WIDTH;
    if (y > ST7735_TFTHEIGHT - currentSprite->HEIGHT) y = ST7735_TFTHEIGHT - currentSprite->HEIGHT;

    selectSprite();
}

void Character::selectSprite()
{
    prevSprite = currentSprite; 
    switch (state)
    {
        case CharacterState::PUNCH: currentSprite = spritePunch; break;
        case CharacterState::KICK: currentSprite = spriteKick;  break;
        case CharacterState::DODGE: currentSprite = spriteDodge; break;
        case CharacterState::IDLE: currentSprite = spriteIdle;  break;
        default: currentSprite = spriteIdle;  break;
    }
}

void Character::draw()
{
    ST7735_FillRect(prevX, prevY, prevSprite->WIDTH, prevSprite->HEIGHT, ST7735_BLACK);
    //ST7735_DrawBitmap(prevX, prevY, 0x0000, prevSprite->WIDTH, prevSprite->HEIGHT);

    blitSprite(x, y, currentSprite, true, SPRITE_TRANSPARENT_COLOR);

    prevX      = x;
    prevY      = y;
    prevSprite = currentSprite;
}

void Character::blitSprite(int16_t drawX, int16_t drawY, const SPRITE_ARRAY* sprite, bool transparent, uint16_t transparentColor)
{
    ST7735_DrawBitmap(drawX, drawY + sprite->HEIGHT - 1, sprite->arr, sprite->WIDTH, sprite->HEIGHT);
}

void Character::setSpriteSet(const SpriteSet& sprites) {
    spriteIdle  = sprites.idle;
    spritePunch = sprites.punch;
    spriteKick  = sprites.kick;
    spriteDodge = sprites.dodge;
    currentSprite = prevSprite = spriteIdle;
}

void Character::reset(int16_t newX, int16_t newY) {
    x = prevX = newX;
    y = prevY = newY;
    health = 100;
    isAlive = true;
    state = CharacterState::IDLE;
    actionTimer = 0;
    currentSprite = prevSprite = spriteIdle;
}

void Character::setPosition(int16_t newX, int16_t newY)
{
    prevX = x;
    prevY = y;
    x     = newX;
    y     = newY;
}
/*int16_t Character::getX(){
    return x;
}
int16_t Character::getY(){
    return y;
}*/