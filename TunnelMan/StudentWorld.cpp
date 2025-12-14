#include "StudentWorld.h"
#include "GameConstants.h"
#include "Actor.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <queue>

using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
    return new StudentWorld(assetDir);
}

StudentWorld::StudentWorld(std::string assetDir)
    : GameWorld(assetDir), m_player(nullptr), m_barrelsLeft(0),
    m_ticksSinceLastProtester(0), m_targetNumProtesters(0), m_gridDirty(true)
{
    for (int x = 0; x < VIEW_WIDTH; x++)
        for (int y = 0; y < VIEW_HEIGHT; y++)
            m_earth[x][y] = nullptr;

    for (int x = 0; x < VIEW_WIDTH; x++) {
        for (int y = 0; y < VIEW_HEIGHT; y++) {
            m_grid_exit[x][y] = 0;
            m_grid_player[x][y] = 0;
        }
    }
}

StudentWorld::~StudentWorld()
{
    cleanUp();
}

int StudentWorld::init()
{
    m_barrelsLeft = 0;
    m_ticksSinceLastProtester = 0;
    m_targetNumProtesters = min(15, 2 + (int)(getLevel() * 1.5));
    m_gridDirty = true;

    // 1. Create Earth
    for (int x = 0; x < VIEW_WIDTH; x++) {
        for (int y = 0; y < 60; y++) {
            if (x >= 30 && x <= 33 && y >= 4) continue;
            m_earth[x][y] = new Earth(x, y, this);
        }
    }

    // 2. Create Player
    m_player = new Tunnelman(this);

    // 3. Distribute Items
    int current_level = getLevel();
    int B = min(current_level / 2 + 2, 9);
    int G = max(5 - current_level / 2, 2);
    int L = min(2 + current_level, 21);
    distributeItems(B, G, L);

    // 4. Force first spawn
    m_ticksSinceLastProtester = max(25, 200 - (int)getLevel());

    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
    updateDisplayText();

    // 0. Update Pathfinding
    if (m_gridDirty) {
        runBFS(60, 60, m_grid_exit);
        m_gridDirty = false;
    }

    // 1. Player Move
    if (m_player->isAlive()) {
        m_player->doSomething();
    }
    else {
        decLives();
        return GWSTATUS_PLAYER_DIED;
    }

    // 2. Actors Move
    for (size_t i = 0; i < m_actors.size(); i++) {
        if (m_actors[i]->isAlive()) {
            m_actors[i]->doSomething();

            if (!m_player->isAlive()) {
                decLives();
                return GWSTATUS_PLAYER_DIED;
            }
            if (m_barrelsLeft == 0) {
                playSound(SOUND_FINISHED_LEVEL);
                return GWSTATUS_FINISHED_LEVEL;
            }
        }
    }

    // 3. Remove Dead
    for (auto it = m_actors.begin(); it != m_actors.end(); ) {
        if (!(*it)->isAlive()) {
            delete* it;
            it = m_actors.erase(it);
        }
        else {
            ++it;
        }
    }

    // 4. Spawn Protesters
    // Count current protesters dynamically to ensure accuracy
    int currentProtesters = 0;
    for (auto* a : m_actors) {
        if (a->canBeAnnoyed()) currentProtesters++;
    }

    int T = max(25, 200 - (int)getLevel());
    if (currentProtesters < m_targetNumProtesters && m_ticksSinceLastProtester >= T) {
        int probHardcore = min(90, (int)getLevel() * 10 + 30);
        if (rand() % 100 < probHardcore) {
            addActor(new HardcoreProtester(this));
        }
        else {
            addActor(new RegularProtester(this));
        }
        m_ticksSinceLastProtester = 0;
    }
    m_ticksSinceLastProtester++;

    // 5. Spawn Goodies
    int G = getLevel() * 25 + 300;
    if (rand() % G == 0) {
        if (rand() % 5 == 0) {
            addActor(new SonarKit(0, 60, this));
        }
        else {
            for (int k = 0; k < 100; k++) {
                int x = rand() % 61;
                int y = rand() % 61;
                if (!isEarthAt(x, y) && !isEarthBelow(x, y) && !isBoulderAt(x, y)) {
                    addActor(new WaterPool(x, y, this));
                    break;
                }
            }
        }
    }

    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
    if (m_player) { delete m_player; m_player = nullptr; }
    for (auto x : m_actors) delete x;
    m_actors.clear();
    for (int x = 0; x < VIEW_WIDTH; x++)
        for (int y = 0; y < VIEW_HEIGHT; y++)
            if (m_earth[x][y]) { delete m_earth[x][y]; m_earth[x][y] = nullptr; }
}

void StudentWorld::updateDisplayText() {
    stringstream ss;
    ss << "Scr: " << setfill('0') << setw(6) << getScore()
        << " Lvl: " << setw(2) << setfill(' ') << getLevel()
        << " Lives: " << getLives()
        << " Hlth: " << setw(3) << m_player->getHP() * 10 << "%"
        << " Wtr: " << setw(2) << m_player->getWater()
        << " Gld: " << setw(2) << m_player->getGold()
        << " Oil Left: " << setw(2) << m_barrelsLeft
        << " Sonar: " << setw(2) << m_player->getSonar();
    setGameStatText(ss.str());
}

void StudentWorld::addActor(Actor* actor) {
    m_actors.push_back(actor);
}

bool StudentWorld::removeEarth(int x, int y) {
    bool dug = false;
    for (int i = x; i < x + 4; i++) {
        for (int j = y; j < y + 4; j++) {
            if (i >= 0 && i < VIEW_WIDTH && j >= 0 && j < VIEW_HEIGHT) {
                if (m_earth[i][j]) {
                    delete m_earth[i][j];
                    m_earth[i][j] = nullptr;
                    dug = true;
                }
            }
        }
    }
    if (dug) flagGridAsDirty();
    return dug;
}

bool StudentWorld::isEarthBelow(int x, int y) const {
    for (int i = x; i < x + 4; i++) {
        if (y - 1 >= 0 && m_earth[i][y - 1]) return true;
    }
    return false;
}

bool StudentWorld::isEarthAt(int x, int y) const {
    for (int i = x; i < x + 4; i++) {
        for (int j = y; j < y + 4; j++) {
            if (i >= 0 && i < VIEW_WIDTH && j >= 0 && j < VIEW_HEIGHT)
                if (m_earth[i][j]) return true;
        }
    }
    return false;
}

bool StudentWorld::isBoulderAt(int x, int y, int radius) {
    for (auto* actor : m_actors) {
        if (actor->isBoulder() && actor->getDistanceTo(x, y) <= radius) return true;
    }
    return false;
}

bool StudentWorld::isLocationAccessible(int x, int y) {
    if (x < 0 || x > 60 || y < 0 || y > 60) return false;
    if (isEarthAt(x, y)) return false;
    if (isBoulderAt(x, y)) return false;
    return true;
}

void StudentWorld::decreaseBarrelCount() {
    m_barrelsLeft--;
}

void StudentWorld::scanForItems(int x, int y, int radius) {
    for (auto* actor : m_actors) {
        if (!actor->isVisible() && actor->getDistanceTo(x, y) <= radius) {
            actor->setVisible(true);
        }
    }
}

// Used by Squirt: Annoys ONLY protesters
bool StudentWorld::annoyProtesters(int x, int y, int radius, int points) {
    bool hit = false;
    for (auto* a : m_actors) {
        if (a->canBeAnnoyed() && a->getDistanceTo(x, y) <= radius) {
            static_cast<Agent*>(a)->decHP(points);
            hit = true;
        }
    }
    return hit;
}

// Used by Boulder: Annoys Player AND Protesters
void StudentWorld::annoyAllNearbyActors(int x, int y, int radius, int points) {
    if (m_player->getDistanceTo(x, y) <= radius) {
        m_player->decHP(points);
    }
    annoyProtesters(x, y, radius, points);
}

bool StudentWorld::bribeEnemy(int x, int y) {
    for (auto* a : m_actors) {
        if (a->canBeAnnoyed() && a->getDistanceTo(x, y) <= 3.0) {
            return a->beBribed();
        }
    }
    return false;
}

void StudentWorld::distributeItems(int numBoulders, int numGold, int numBarrels)
{
    for (int i = 0; i < numBoulders; i++) {
        int x, y;
        do { x = rand() % 61; y = rand() % 37 + 20; } while (!isPositionValid(x, y));
        addActor(new Boulder(x, y, this));
        flagGridAsDirty();
    }
    for (int i = 0; i < numBarrels; i++) {
        int x, y;
        do { x = rand() % 61; y = rand() % 57; } while (!isPositionValid(x, y));
        addActor(new Barrel(x, y, this));
        m_barrelsLeft++;
    }
    for (int i = 0; i < numGold; i++) {
        int x, y;
        do { x = rand() % 61; y = rand() % 57; } while (!isPositionValid(x, y));
        addActor(new GoldNugget(x, y, this, false, true, false));
    }
}

bool StudentWorld::isPositionValid(int x, int y)
{
    if (x > 26 && x < 34 && y > 4) return false;
    for (auto* actor : m_actors) {
        if (actor->getDistanceTo(x, y) <= 6.0) return false;
    }
    return true;
}

void StudentWorld::runBFS(int targetX, int targetY, int outputGrid[VIEW_WIDTH][VIEW_HEIGHT]) {
    for (int x = 0; x < VIEW_WIDTH; x++)
        for (int y = 0; y < VIEW_HEIGHT; y++)
            outputGrid[x][y] = -1;

    queue<pair<int, int>> q;
    q.push({ targetX, targetY });
    outputGrid[targetX][targetY] = 0;

    int dirs[4][2] = { {0,1}, {0,-1}, {1,0}, {-1,0} };

    while (!q.empty()) {
        auto curr = q.front(); q.pop();
        int cx = curr.first;
        int cy = curr.second;
        int dist = outputGrid[cx][cy];

        for (auto& d : dirs) {
            int nx = cx + d[0];
            int ny = cy + d[1];

            if (isLocationAccessible(nx, ny) && outputGrid[nx][ny] == -1) {
                outputGrid[nx][ny] = dist + 1;
                q.push({ nx, ny });
            }
        }
    }
}

GraphObject::Direction StudentWorld::getDirectionToExit(int x, int y) {
    int currentDist = m_grid_exit[x][y];
    if (currentDist == -1) return GraphObject::none;

    if (x + 1 <= 60 && m_grid_exit[x + 1][y] != -1 && m_grid_exit[x + 1][y] < currentDist) return GraphObject::right;
    if (x - 1 >= 0 && m_grid_exit[x - 1][y] != -1 && m_grid_exit[x - 1][y] < currentDist) return GraphObject::left;
    if (y + 1 <= 60 && m_grid_exit[x][y + 1] != -1 && m_grid_exit[x][y + 1] < currentDist) return GraphObject::up;
    if (y - 1 >= 0 && m_grid_exit[x][y - 1] != -1 && m_grid_exit[x][y - 1] < currentDist) return GraphObject::down;

    return GraphObject::none;
}

GraphObject::Direction StudentWorld::getDirectionToPlayer(int x, int y, int maxMoves) {
    runBFS(m_player->getX(), m_player->getY(), m_grid_player);

    int dist = m_grid_player[x][y];
    if (dist == -1 || dist > maxMoves) return GraphObject::none;

    if (x + 1 <= 60 && m_grid_player[x + 1][y] != -1 && m_grid_player[x + 1][y] < dist) return GraphObject::right;
    if (x - 1 >= 0 && m_grid_player[x - 1][y] != -1 && m_grid_player[x - 1][y] < dist) return GraphObject::left;
    if (y + 1 <= 60 && m_grid_player[x][y + 1] != -1 && m_grid_player[x][y + 1] < dist) return GraphObject::up;
    if (y - 1 >= 0 && m_grid_player[x][y - 1] != -1 && m_grid_player[x][y - 1] < dist) return GraphObject::down;

    return GraphObject::none;
}