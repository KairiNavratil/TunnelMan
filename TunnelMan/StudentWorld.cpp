#include "StudentWorld.h"
#include "GameConstants.h"
#include "Actor.h"
#include <string>
using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
    return new StudentWorld(assetDir);
}

int StudentWorld::init()
{
    // 1. Create the Tunnelman
    m_player = new Tunnelman(this);

    // 2. Create the Earth field
    for (int x = 0; x < VIEW_WIDTH; x++)
    {
        for (int y = 0; y < 60; y++) // Earth goes from 0 to 59
        {
            // Leave the mine shaft empty:
            // x = 30-33, y = 4-59
            if (x >= 30 && x <= 33 && y >= 4)
                continue;

            m_earth[x][y] = new Earth(x, y, this);
        }
    }

    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
    // Ask player to do something
    if (m_player->isAlive())
    {
        m_player->doSomething();
    }
    else
    {
        decLives();
        return GWSTATUS_PLAYER_DIED;
    }

    // For Part 1, we don't handle other actors yet.

    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
    // Delete the player
    if (m_player != nullptr)
    {
        delete m_player;
        m_player = nullptr;
    }

    // Delete all earth objects
    for (int x = 0; x < VIEW_WIDTH; x++)
    {
        for (int y = 0; y < VIEW_HEIGHT; y++)
        {
            if (m_earth[x][y] != nullptr)
            {
                delete m_earth[x][y];
                m_earth[x][y] = nullptr;
            }
        }
    }
}

bool StudentWorld::removeEarth(int x, int y)
{
    bool dug = false;

    // Check the 4x4 area occupied by the player
    for (int i = x; i < x + 4; i++)
    {
        for (int j = y; j < y + 4; j++)
        {
            // Bounds check
            if (i >= 0 && i < VIEW_WIDTH && j >= 0 && j < VIEW_HEIGHT)
            {
                if (m_earth[i][j] != nullptr)
                {
                    delete m_earth[i][j];
                    m_earth[i][j] = nullptr;
                    dug = true;
                }
            }
        }
    }
    return dug;
}