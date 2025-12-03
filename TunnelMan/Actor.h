#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"

// Forward declaration so we can use a pointer to StudentWorld
class StudentWorld;

// Base Class for all game objects
class Actor : public GraphObject
{
public:
    Actor(int imageID, int startX, int startY, StudentWorld* world, Direction dir = right, double size = 1.0, unsigned int depth = 0)
        : GraphObject(imageID, startX, startY, dir, size, depth), m_world(world), m_isAlive(true)
    {
        setVisible(true);
    }

    virtual void doSomething() = 0;

    bool isAlive() const { return m_isAlive; }
    void setDead() { m_isAlive = false; }

    StudentWorld* getWorld() const { return m_world; }

private:
    StudentWorld* m_world;
    bool m_isAlive;
};

// Earth Class
class Earth : public Actor
{
public:
    Earth(int startX, int startY, StudentWorld* world)
        : Actor(TID_EARTH, startX, startY, world, right, 0.25, 3)
    {
        // Earth doesn't need to do anything, but it must be visible and have the right depth/size
    }

    virtual void doSomething() {} // Earth does nothing
};

// Tunnelman Class
class Tunnelman : public Actor
{
public:
    Tunnelman(StudentWorld* world)
        : Actor(TID_PLAYER, 30, 60, world, right, 1.0, 0)
    {
        // Tunnelman starts at 30, 60, depth 0
    }

    virtual void doSomething();
};

#endif // ACTOR_H_