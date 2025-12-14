#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include "GraphObject.h"
#include <string>
#include <vector>
#include <queue>

class Actor;
class Tunnelman;
class Earth;

class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetDir);
    virtual ~StudentWorld();

    virtual int init();
    virtual int move();
    virtual void cleanUp();

    // -- Accessors --
    Tunnelman* getPlayer() const { return m_player; }

    // -- Earth/Physics Helpers --
    bool removeEarth(int x, int y);
    bool isEarthAt(int x, int y) const;
    bool isEarthBelow(int x, int y) const;
    bool isBoulderAt(int x, int y, int radius = 3);
    bool isLocationAccessible(int x, int y);

    // -- Game Logic Helpers --
    void addActor(Actor* actor);
    void decreaseBarrelCount();
    void scanForItems(int x, int y, int radius);

    // Protester/Squirt Interaction
    // Returns true if at least one protester was annoyed (used to kill Squirt)
    bool annoyProtesters(int x, int y, int radius, int points);

    // Boulder Interaction (Annoys Player + Protesters)
    void annoyAllNearbyActors(int x, int y, int radius, int points);

    bool bribeEnemy(int x, int y);
    bool canSpawnProtester();

    // -- Pathfinding --
    GraphObject::Direction getDirectionToExit(int x, int y);
    GraphObject::Direction getDirectionToPlayer(int x, int y, int maxMoves);

    void flagGridAsDirty() { m_gridDirty = true; }

private:
    Tunnelman* m_player;
    Earth* m_earth[VIEW_WIDTH][VIEW_HEIGHT];
    std::vector<Actor*> m_actors;

    int m_barrelsLeft;
    int m_ticksSinceLastProtester;
    int m_targetNumProtesters;
    int m_protesterCount;

    // Pathfinding Grids
    int m_grid_exit[VIEW_WIDTH][VIEW_HEIGHT];
    int m_grid_player[VIEW_WIDTH][VIEW_HEIGHT];
    bool m_gridDirty;

    void updateDisplayText();
    void distributeItems(int numBoulders, int numGold, int numBarrels);
    bool isPositionValid(int x, int y);

    void runBFS(int targetX, int targetY, int outputGrid[VIEW_WIDTH][VIEW_HEIGHT]);
};

#endif // STUDENTWORLD_H_