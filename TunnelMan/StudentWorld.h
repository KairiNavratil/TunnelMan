#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include <string>

class Actor;
class Tunnelman;
class Earth;

class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetDir)
        : GameWorld(assetDir), m_player(nullptr)
    {
        // Initialize earth array to nullptr
        for (int x = 0; x < VIEW_WIDTH; x++)
            for (int y = 0; y < VIEW_HEIGHT; y++)
                m_earth[x][y] = nullptr;
    }

    virtual ~StudentWorld()
    {
        cleanUp();
    }

    virtual int init();
    virtual int move();
    virtual void cleanUp();

    // Helper to remove earth at location x,y (and the 4x4 sprite area)
    // Returns true if any earth was removed
    bool removeEarth(int x, int y);

private:
    Tunnelman* m_player;
    Earth* m_earth[VIEW_WIDTH][VIEW_HEIGHT];
};

#endif // STUDENTWORLD_H_