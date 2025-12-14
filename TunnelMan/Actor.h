#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include <algorithm>

class StudentWorld;

// ============================================================================
// BASE ACTOR
// ============================================================================
class Actor : public GraphObject
{
public:
    Actor(int imageID, int startX, int startY, StudentWorld* world, Direction dir = right, double size = 1.0, unsigned int depth = 0)
        : GraphObject(imageID, startX, startY, dir, size, depth), m_world(world), m_isAlive(true) {
        setVisible(true);
    }

    virtual void doSomething() = 0;

    bool isAlive() const { return m_isAlive; }
    void setDead() { m_isAlive = false; }
    StudentWorld* getWorld() const { return m_world; }

    virtual bool canBeAnnoyed() const { return false; }
    virtual bool beBribed() { return false; }
    virtual bool isBoulder() const { return false; }

    double getDistanceTo(int x, int y) const;
    double getDistanceTo(Actor* other) const;

private:
    StudentWorld* m_world;
    bool m_isAlive;
};

// Earth
class Earth : public Actor {
public:
    Earth(int startX, int startY, StudentWorld* world) : Actor(TID_EARTH, startX, startY, world, right, 0.25, 3) {}
    virtual void doSomething() {}
};

// Agent
class Agent : public Actor {
public:
    Agent(int imageID, int startX, int startY, StudentWorld* world, int hp, Direction dir)
        : Actor(imageID, startX, startY, world, dir, 1.0, 0), m_hp(hp) {
    }

    int getHP() const { return m_hp; }
    virtual void decHP(int amount);
    virtual bool canBeAnnoyed() const { return true; }

protected:
    int m_hp;
};

// Tunnelman
class Tunnelman : public Agent {
public:
    Tunnelman(StudentWorld* world);
    virtual void doSomething();
    virtual void decHP(int amount);

    void addGold(int n) { m_gold += n; }
    int getGold() const { return m_gold; }
    void addSonar(int n) { m_sonar += n; }
    int getSonar() const { return m_sonar; }
    void addWater(int n) { m_water += n; }
    int getWater() const { return m_water; }
private:
    int m_gold; int m_sonar; int m_water;
};

// Protesters
class Protester : public Agent {
public:
    Protester(StudentWorld* world, int imageID, int hp);
    virtual void doSomething();
    virtual void decHP(int amount);
    virtual bool beBribed();
    virtual bool isHardcore() const { return false; }
    virtual bool canBeAnnoyed() const;

protected:
    int m_ticksToWait;
    int m_stunTicks;

private:
    int m_numSquaresToMove;
    bool m_leaving;
    int m_ticksSincePerpendicularTurn;
    int m_ticksSinceLastShout; // <--- ADDED

    bool isFacingPlayer() const;
    bool hasLineOfSightToPlayer() const;
    void pickNewDirection();
};

class RegularProtester : public Protester {
public:
    RegularProtester(StudentWorld* world);
};

class HardcoreProtester : public Protester {
public:
    HardcoreProtester(StudentWorld* world);
    virtual bool beBribed();
    virtual bool isHardcore() const { return true; }
};

// Boulder
class Boulder : public Actor {
public:
    Boulder(int startX, int startY, StudentWorld* world);
    virtual void doSomething();
    virtual bool isBoulder() const { return true; }
private:
    enum State { STABLE, WAITING, FALLING };
    State m_state; int m_waitingTicks;
};

// ActivatableObject
class ActivatableObject : public Actor {
public:
    ActivatableObject(int imageID, int startX, int startY, StudentWorld* world, int score, int sound, bool startsVisible, bool temporary, int ticksToLive = 0);
    virtual void doSomething();
    virtual void doEffect() = 0;
    bool isPickupableByPlayer() const { return m_pickupableByPlayer; }
    void setPickupableByPlayer(bool val) { m_pickupableByPlayer = val; }
    bool isPickupableByEnemy() const { return m_pickupableByEnemy; }
    void setPickupableByEnemy(bool val) { m_pickupableByEnemy = val; }
private:
    int m_scoreDelta; int m_soundID; bool m_temporary; int m_ticksToLive; bool m_pickupableByPlayer; bool m_pickupableByEnemy;
};

class Barrel : public ActivatableObject { public: Barrel(int x, int y, StudentWorld* w); virtual void doEffect(); };
class GoldNugget : public ActivatableObject { public: GoldNugget(int x, int y, StudentWorld* w, bool v, bool p, bool e); virtual void doEffect(); virtual void doSomething(); };
class SonarKit : public ActivatableObject { public: SonarKit(int x, int y, StudentWorld* w); virtual void doEffect(); };
class WaterPool : public ActivatableObject { public: WaterPool(int x, int y, StudentWorld* w); virtual void doEffect(); };

// Squirt
class Squirt : public Actor {
public:
    Squirt(int startX, int startY, StudentWorld* world, Direction dir);
    virtual void doSomething();
private:
    int m_travelDist;
};

#endif // ACTOR_H_