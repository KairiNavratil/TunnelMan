#include "Actor.h"
#include "StudentWorld.h"
#include "GameConstants.h"
#include <cmath>
#include <cstdlib>

// ============================================================================
// HELPERS
// ============================================================================

double Actor::getDistanceTo(int x, int y) const {
    return std::sqrt(std::pow(getX() - x, 2) + std::pow(getY() - y, 2));
}

double Actor::getDistanceTo(Actor* other) const {
    return getDistanceTo(other->getX(), other->getY());
}

void Agent::decHP(int amount) {
    m_hp -= amount;
    // Base Agent doesn't auto-die anymore, derived classes handle it
}

// ============================================================================
// TUNNELMAN
// ============================================================================

Tunnelman::Tunnelman(StudentWorld* world)
    : Agent(TID_PLAYER, 30, 60, world, 10, right), m_gold(0), m_sonar(1), m_water(5)
{
}

void Tunnelman::decHP(int amount) {
    Agent::decHP(amount);
    if (getHP() <= 0) setDead();
}

void Tunnelman::doSomething()
{
    if (!isAlive()) return;

    if (getWorld()->removeEarth(getX(), getY())) {
        getWorld()->playSound(SOUND_DIG);
    }

    int ch;
    if (getWorld()->getKey(ch)) {
        switch (ch) {
        case KEY_PRESS_LEFT:
            if (getDirection() != left) setDirection(left);
            else if (getX() > 0 && !getWorld()->isBoulderAt(getX() - 1, getY()))
                moveTo(getX() - 1, getY());
            break;
        case KEY_PRESS_RIGHT:
            if (getDirection() != right) setDirection(right);
            else if (getX() < 60 && !getWorld()->isBoulderAt(getX() + 1, getY()))
                moveTo(getX() + 1, getY());
            break;
        case KEY_PRESS_UP:
            if (getDirection() != up) setDirection(up);
            else if (getY() < 60 && !getWorld()->isBoulderAt(getX(), getY() + 1))
                moveTo(getX(), getY() + 1);
            break;
        case KEY_PRESS_DOWN:
            if (getDirection() != down) setDirection(down);
            else if (getY() > 0 && !getWorld()->isBoulderAt(getX(), getY() - 1))
                moveTo(getX(), getY() - 1);
            break;
        case KEY_PRESS_ESCAPE:
            setDead();
            break;
        case KEY_PRESS_SPACE:
            if (m_water > 0) {
                m_water--;
                getWorld()->playSound(SOUND_PLAYER_SQUIRT);
                int sx = getX(), sy = getY();
                if (getDirection() == left) sx -= 4;
                if (getDirection() == right) sx += 4;
                if (getDirection() == up) sy += 4;
                if (getDirection() == down) sy -= 4;

                if (getWorld()->isLocationAccessible(sx, sy)) {
                    getWorld()->addActor(new Squirt(sx, sy, getWorld(), getDirection()));
                }
            }
            break;
        case 'z':
        case 'Z':
            if (m_sonar > 0) {
                m_sonar--;
                getWorld()->scanForItems(getX(), getY(), 12);
                getWorld()->playSound(SOUND_SONAR);
            }
            break;
        case KEY_PRESS_TAB:
            if (m_gold > 0) {
                m_gold--;
                getWorld()->addActor(new GoldNugget(getX(), getY(), getWorld(), true, false, true));
            }
            break;
        }
    }
}

// ============================================================================
// PROTESTER
// ============================================================================

Protester::Protester(StudentWorld* world, int imageID, int hp)
    : Agent(imageID, 60, 60, world, hp, left),
    m_numSquaresToMove(rand() % 53 + 8), m_ticksToWait(0), m_leaving(false),
    m_stunTicks(0), m_ticksSincePerpendicularTurn(200), m_ticksSinceLastShout(15)
{
    m_ticksToWait = std::max(0, 3 - (int)world->getLevel() / 4);
}

bool Protester::canBeAnnoyed() const {
    return !m_leaving;
}

void Protester::doSomething()
{
    if (!isAlive()) return;

    // 1. Check Rest
    if (m_ticksToWait > 0) {
        m_ticksToWait--;
        return;
    }

    // Calculate normal wait time for reset (Standard speed)
    int restingTicks = std::max(0, 3 - (int)getWorld()->getLevel() / 4);

    // Default next wait is the standard speed
    m_ticksToWait = restingTicks;

    m_ticksSincePerpendicularTurn++;
    m_ticksSinceLastShout++;

    // 2. Leaving State
    if (m_leaving) {
        // We use m_ticksToWait = 0 to make them move as fast as possible when leaving, 
        // OR we leave it as restingTicks if we want them to walk out at normal speed.
        // Spec usually implies they give up and leave quickly, but "resting states" might still apply.
        // If they retreat "too fast", uncomment the line below to make them respect speed:
        m_ticksToWait = 0; // Force fast exit (moves every tick)

        if (getX() == 60 && getY() == 60) {
            setDead();
            return;
        }
        Direction dir = getWorld()->getDirectionToExit(getX(), getY());
        if (dir != none) {
            setDirection(dir);
            if (dir == up) moveTo(getX(), getY() + 1);
            if (dir == down) moveTo(getX(), getY() - 1);
            if (dir == left) moveTo(getX() - 1, getY());
            if (dir == right) moveTo(getX() + 1, getY());
        }
        return;
    }

    // 3. Shout at Player
    if (getDistanceTo(getWorld()->getPlayer()) <= 4.0 && isFacingPlayer()) {
        if (m_ticksSinceLastShout >= 15) {
            getWorld()->playSound(SOUND_PROTESTER_YELL);
            getWorld()->getPlayer()->decHP(2);
            m_ticksSinceLastShout = 0;
            m_ticksToWait = std::max(15, restingTicks); // Pause after shout
            return;
        }
    }

    // 4. Hardcore Tracking
    // Added > 4.0 check to prevent oscillation when already close
    if (isHardcore() && getDistanceTo(getWorld()->getPlayer()) > 4.0) {
        int M = 16 + getWorld()->getLevel() * 2;
        Direction d = getWorld()->getDirectionToPlayer(getX(), getY(), M);
        if (d != none) {
            setDirection(d);
            if (d == up) moveTo(getX(), getY() + 1);
            if (d == down) moveTo(getX(), getY() - 1);
            if (d == left) moveTo(getX() - 1, getY());
            if (d == right) moveTo(getX() + 1, getY());
            return;
        }
    }

    // 5. Line of Sight
    if (hasLineOfSightToPlayer() && getDistanceTo(getWorld()->getPlayer()) > 4.0) {
        Direction d = none;
        if (getWorld()->getPlayer()->getY() == getY())
            d = (getWorld()->getPlayer()->getX() < getX()) ? left : right;
        else
            d = (getWorld()->getPlayer()->getY() < getY()) ? down : up;

        setDirection(d);
        if (d == up) moveTo(getX(), getY() + 1);
        if (d == down) moveTo(getX(), getY() - 1);
        if (d == left) moveTo(getX() - 1, getY());
        if (d == right) moveTo(getX() + 1, getY());

        m_numSquaresToMove = 0;
        return;
    }

    // 6. Regular Movement
    m_numSquaresToMove--;
    if (m_numSquaresToMove <= 0) {
        pickNewDirection();
    }
    else {
        if (m_ticksSincePerpendicularTurn > 200) {
            Direction perp1 = none, perp2 = none;
            if (getDirection() == left || getDirection() == right) {
                perp1 = up; perp2 = down;
            }
            else {
                perp1 = left; perp2 = right;
            }

            bool can1 = (perp1 == up) ? getWorld()->isLocationAccessible(getX(), getY() + 1) :
                (perp1 == down) ? getWorld()->isLocationAccessible(getX(), getY() - 1) :
                (perp1 == left) ? getWorld()->isLocationAccessible(getX() - 1, getY()) :
                getWorld()->isLocationAccessible(getX() + 1, getY());

            bool can2 = (perp2 == up) ? getWorld()->isLocationAccessible(getX(), getY() + 1) :
                (perp2 == down) ? getWorld()->isLocationAccessible(getX(), getY() - 1) :
                (perp2 == left) ? getWorld()->isLocationAccessible(getX() - 1, getY()) :
                getWorld()->isLocationAccessible(getX() + 1, getY());

            if (can1 || can2) {
                if (can1 && can2) setDirection((rand() % 2 == 0) ? perp1 : perp2);
                else if (can1) setDirection(perp1);
                else setDirection(perp2);

                m_numSquaresToMove = rand() % 53 + 8;
                m_ticksSincePerpendicularTurn = 0;
            }
        }
    }

    int tx = getX(), ty = getY();
    if (getDirection() == up) ty++;
    else if (getDirection() == down) ty--;
    else if (getDirection() == left) tx--;
    else if (getDirection() == right) tx++;

    if (getWorld()->isLocationAccessible(tx, ty)) {
        moveTo(tx, ty);
    }
    else {
        m_numSquaresToMove = 0;
    }
}

void Protester::decHP(int amount) {
    if (m_leaving) return;

    Agent::decHP(amount);

    if (getHP() > 0) {
        getWorld()->playSound(SOUND_PROTESTER_ANNOYED);
        int N = std::max(50, 100 - (int)getWorld()->getLevel() * 10);
        m_ticksToWait = N; // STUN
    }
    else {
        if (!m_leaving) {
            m_leaving = true;
            getWorld()->playSound(SOUND_PROTESTER_GIVE_UP);
            m_ticksToWait = 0;
        }
    }
}

bool Protester::isFacingPlayer() const {
    int px = getWorld()->getPlayer()->getX();
    int py = getWorld()->getPlayer()->getY();
    return (getDirection() == left && px < getX()) ||
        (getDirection() == right && px > getX()) ||
        (getDirection() == up && py > getY()) ||
        (getDirection() == down && py < getY());
}

bool Protester::hasLineOfSightToPlayer() const {
    int px = getWorld()->getPlayer()->getX();
    int py = getWorld()->getPlayer()->getY();
    if (px != getX() && py != getY()) return false;

    if (px == getX()) {
        int min = std::min(getY(), py), max = std::max(getY(), py);
        for (int k = min; k <= max; k++) if (!getWorld()->isLocationAccessible(getX(), k)) return false;
    }
    else {
        int min = std::min(getX(), px), max = std::max(getX(), px);
        for (int k = min; k <= max; k++) if (!getWorld()->isLocationAccessible(k, getY())) return false;
    }
    return true;
}

void Protester::pickNewDirection() {
    int dir = rand() % 4;
    Direction d;
    if (dir == 0) d = up; else if (dir == 1) d = down; else if (dir == 2) d = left; else d = right;

    bool blocked = false;
    if (d == up && !getWorld()->isLocationAccessible(getX(), getY() + 1)) blocked = true;
    if (d == down && !getWorld()->isLocationAccessible(getX(), getY() - 1)) blocked = true;
    if (d == left && !getWorld()->isLocationAccessible(getX() - 1, getY())) blocked = true;
    if (d == right && !getWorld()->isLocationAccessible(getX() + 1, getY())) blocked = true;

    if (!blocked) {
        setDirection(d);
        m_numSquaresToMove = rand() % 53 + 8;
    }
    else {
        m_numSquaresToMove = 0;
    }
}

bool Protester::beBribed() {
    getWorld()->playSound(SOUND_PROTESTER_FOUND_GOLD);
    getWorld()->increaseScore(25);
    m_leaving = true;
    return true;
}

RegularProtester::RegularProtester(StudentWorld* world) : Protester(world, TID_PROTESTER, 5) {}

HardcoreProtester::HardcoreProtester(StudentWorld* world) : Protester(world, TID_HARD_CORE_PROTESTER, 20) {}

bool HardcoreProtester::beBribed() {
    getWorld()->playSound(SOUND_PROTESTER_FOUND_GOLD);
    getWorld()->increaseScore(50);
    int N = std::max(50, 100 - (int)getWorld()->getLevel() * 10);
    m_ticksToWait = N; // Freeze hardcore
    return true;
}

// ============================================================================
// BOULDER & ITEMS
// ============================================================================
Boulder::Boulder(int startX, int startY, StudentWorld* world)
    : Actor(TID_BOULDER, startX, startY, world, down, 1.0, 1), m_state(STABLE), m_waitingTicks(0)
{
    world->removeEarth(startX, startY);
}

void Boulder::doSomething()
{
    if (!isAlive()) return;

    if (m_state == STABLE) {
        if (!getWorld()->isEarthBelow(getX(), getY())) m_state = WAITING;
    }
    else if (m_state == WAITING) {
        m_waitingTicks++;
        if (m_waitingTicks >= 30) {
            m_state = FALLING;
            getWorld()->playSound(SOUND_FALLING_ROCK);
        }
    }
    else if (m_state == FALLING) {
        if (getY() > 0 && !getWorld()->isEarthAt(getX(), getY() - 1) &&
            !getWorld()->isBoulderAt(getX(), getY() - 4, 0)) {
            moveTo(getX(), getY() - 1);
            getWorld()->annoyAllNearbyActors(getX(), getY(), 3, 100);
        }
        else {
            setDead();
        }
    }
}

ActivatableObject::ActivatableObject(int imageID, int startX, int startY, StudentWorld* world,
    int score, int sound, bool startsVisible, bool temporary, int ticksToLive)
    : Actor(imageID, startX, startY, world, right, 1.0, 2),
    m_scoreDelta(score), m_soundID(sound), m_temporary(temporary),
    m_ticksToLive(ticksToLive), m_pickupableByPlayer(true), m_pickupableByEnemy(false)
{
    setVisible(startsVisible);
}

void ActivatableObject::doSomething()
{
    if (!isAlive()) return;
    if (m_temporary) {
        m_ticksToLive--;
        if (m_ticksToLive <= 0) { setDead(); return; }
    }
    if (m_pickupableByPlayer) {
        if (getDistanceTo(getWorld()->getPlayer()) <= 3.0) {
            setDead();
            getWorld()->playSound(m_soundID);
            getWorld()->increaseScore(m_scoreDelta);
            doEffect();
            return;
        }
        else if (getDistanceTo(getWorld()->getPlayer()) <= 4.0) {
            setVisible(true);
        }
    }
}

Barrel::Barrel(int startX, int startY, StudentWorld* world)
    : ActivatableObject(TID_BARREL, startX, startY, world, 1000, SOUND_FOUND_OIL, false, false) {
}
void Barrel::doEffect() { getWorld()->decreaseBarrelCount(); }

SonarKit::SonarKit(int startX, int startY, StudentWorld* world)
    : ActivatableObject(TID_SONAR, startX, startY, world, 75, SOUND_GOT_GOODIE, true, true, std::max(100, 300 - 10 * (int)world->getLevel())) {
}
void SonarKit::doEffect() { getWorld()->getPlayer()->addSonar(2); }

WaterPool::WaterPool(int startX, int startY, StudentWorld* world)
    : ActivatableObject(TID_WATER_POOL, startX, startY, world, 100, SOUND_GOT_GOODIE, true, true, std::max(100, 300 - 10 * (int)world->getLevel())) {
}
void WaterPool::doEffect() { getWorld()->getPlayer()->addWater(5); }

GoldNugget::GoldNugget(int startX, int startY, StudentWorld* world, bool visible, bool pickupableByPlayer, bool pickupableByEnemy)
    : ActivatableObject(TID_GOLD, startX, startY, world, 10, SOUND_GOT_GOODIE, visible, visible, 100)
{
    setPickupableByPlayer(pickupableByPlayer);
    setPickupableByEnemy(pickupableByEnemy);
}
void GoldNugget::doEffect() { getWorld()->getPlayer()->addGold(1); }

void GoldNugget::doSomething() {
    ActivatableObject::doSomething();
    if (!isAlive()) return;

    if (isPickupableByEnemy()) {
        if (getWorld()->bribeEnemy(getX(), getY())) setDead();
    }
}

Squirt::Squirt(int startX, int startY, StudentWorld* world, Direction dir)
    : Actor(TID_WATER_SPURT, startX, startY, world, dir, 1.0, 1), m_travelDist(4) {
}

void Squirt::doSomething() {
    if (!isAlive()) return;

    // Annoy Protesters
    if (getWorld()->annoyProtesters(getX(), getY(), 3, 2)) {
        setDead(); // Die on hit
        return;
    }

    if (m_travelDist == 0) { setDead(); return; }
    m_travelDist--;

    int tx = getX(), ty = getY();
    if (getDirection() == up) ty++;
    else if (getDirection() == down) ty--;
    else if (getDirection() == left) tx--;
    else if (getDirection() == right) tx++;

    if (getWorld()->isLocationAccessible(tx, ty)) {
        moveTo(tx, ty);
    }
    else {
        setDead();
    }
}