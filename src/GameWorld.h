#pragma once
#include <QObject>
#include <vector>
#include <memory>
#include "Player.h"
#include "Food.h"
#include "Spore.h"
#include "Thorn.h"
#include "QuadTree.h"

class GameWorld : public QObject
{
    Q_OBJECT
public:
    GameWorld(int mode, QObject *parent = nullptr);
    void update();
    void render(QPainter *painter, const QSize &size);
    Player* getPlayer(int id);
    int getPlayerScore(int id) const;
    bool isGameOver() const;

    void addFood(std::unique_ptr<Food> food);
    void addSpore(std::unique_ptr<Spore> spore);
    void addThorn(std::unique_ptr<Thorn> thorn);

    std::vector<Food*>& getFoods();
    std::vector<Spore*>& getSpores();
    std::vector<Thorn*>& getThorns();
    std::vector<Player*>& getPlayers();

private:
    int mode;
    std::vector<std::unique_ptr<Player>> players;
    std::vector<std::unique_ptr<Food>> foods;
    std::vector<std::unique_ptr<Spore>> spores;
    std::vector<std::unique_ptr<Thorn>> thorns;
    QuadTree quadTree;
    int tick;
    void spawnEntities();
    void handleCollisions();
};