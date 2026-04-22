#pragma once
#include <stdint.h>
#include "./inc/sprite_data.h"

#define FRAME_HOLD_TICKS 6   // how long an action sprite stays visible before returning to idle

enum class CharacterState {
    IDLE,
    PUNCH,
    KICK,
    DODGE
};

enum class Direction {
    LEFT,
    RIGHT
};

class Character {
public:
    Character(int16_t startX, int16_t startY, const SpriteSet& sprites);

    // Call once per game loop tick.
    // Pass the new state when a button is pressed, otherwise pass IDLE
    void update(CharacterState requestedState);

    // Erases old position and draws new sprite.
    // bgColor is the background fill color used to erase the previous frame.
    void draw();
    
    
    void setPosition(int16_t x, int16_t y);

    bool takeDmg(CharacterState attack);
    bool checkHit(int16_t eX, int16_t eY);

    void moveX(int16_t amount);
    void moveY(int16_t amount);

    int16_t getX() const { return x; }
    int16_t getY() const { return y; }
    CharacterState getState() const { return state; }
    int16_t getHealth() const { return health; }

private:
    int16_t x, y;
    int16_t prevX, prevY;

    int16_t health;
    bool isAlive;

    CharacterState state;
    Direction facing;

    // Counts down from FRAME_HOLD_TICKS to 0 while an action is playing.
    // When it hits 0 the character returns to IDLE automatically.
    uint8_t actionTimer;

    const SPRITE_ARRAY* currentSprite;
    const SPRITE_ARRAY* prevSprite;

    // Cached pointers from the SpriteSet passed at construction
    const SPRITE_ARRAY* spriteIdle;
    const SPRITE_ARRAY* spritePunch;
    const SPRITE_ARRAY* spriteKick;
    const SPRITE_ARRAY* spriteDodge;

    void selectSprite();

    void blitSprite(int16_t drawX, int16_t drawY, const SPRITE_ARRAY* sprite,
                    bool transparent, uint16_t transparentColor);
};