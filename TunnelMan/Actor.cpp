#include "Actor.h"
#include "StudentWorld.h"
#include "GameConstants.h"

// Tunnelman implementation
void Tunnelman::doSomething()
{
    if (!isAlive())
        return;

    // 1. Digging Logic
    // If we overlap with earth, remove it and play a sound.
    // Note: Tunnelman is 4x4, so we check the 4x4 grid at current location.
    if (getWorld()->removeEarth(getX(), getY()))
    {
        getWorld()->playSound(SOUND_DIG);
        // Per spec logic, if we dig, we might technically "return" here to simulate delay,
        // but for smooth gameplay in Part 1, we often allow digging + moving or just check input.
        // We'll continue to input handling to allow fluid movement.
    }

    // 2. User Input
    int ch;
    if (getWorld()->getKey(ch))
    {
        switch (ch)
        {
        case KEY_PRESS_LEFT:
            if (getDirection() != left)
                setDirection(left);
            else if (getX() > 0)
                moveTo(getX() - 1, getY());
            break;

        case KEY_PRESS_RIGHT:
            if (getDirection() != right)
                setDirection(right);
            else if (getX() < 60) // Width is 64, sprite is 4. 64-4 = 60
                moveTo(getX() + 1, getY());
            break;

        case KEY_PRESS_UP:
            if (getDirection() != up)
                setDirection(up);
            else if (getY() < 60) // Height is 64, sprite is 4. 64-4 = 60
                moveTo(getX(), getY() + 1);
            break;

        case KEY_PRESS_DOWN:
            if (getDirection() != down)
                setDirection(down);
            else if (getY() > 0)
                moveTo(getX(), getY() - 1);
            break;

        case KEY_PRESS_ESCAPE:
            setDead(); // Kill player to restart level
            break;
        }
    }
}