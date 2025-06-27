#include "GameWorld.h"
#include "Physics.h"
#include <algorithm>
#include <cmath>
#include "HumanController.h"
#include "AIController.h"
GameWorld::GameWorld(int mode, QObject *parent)
    : QObject(parent), mode(mode), quadTree(0, 0, 6000, 4000), tick(0)
{
    players.emplace_back(std::make_unique<Player>(1, QColor(70, 130, 255)));
    players.emplace_back(std::make_unique<Player>(2, QColor(220, 20, 60)));
    for (int i = 0; i < 3000; ++i)
        foods.emplace_back(std::make_unique<Food>());
    for (int i = 0; i < 15; ++i)
        thorns.emplace_back(std::make_unique<Thorn>());
}

void GameWorld::update()
{
    ++tick;
    spawnEntities();
    for (auto &player : players) player->update(*this);
    for (auto &food : foods) food->update(*this);
    for (auto &spore : spores) spore->update(*this);
    for (auto &thorn : thorns) thorn->update(*this);
    handleCollisions();
    foods.erase(std::remove_if(foods.begin(), foods.end(),
        [](const std::unique_ptr<Food>& f){return !f->isAlive();}), foods.end());
    spores.erase(std::remove_if(spores.begin(), spores.end(),
        [](const std::unique_ptr<Spore>& s){return !s->isAlive();}), spores.end());
}

void GameWorld::render(QPainter *painter, const QSize &size)
{
    painter->fillRect(0, 0, size.width(), size.height(), Qt::white);
    for (auto &food : foods) food->render(painter);
    for (auto &spore : spores) spore->render(painter);
    for (auto &thorn : thorns) thorn->render(painter);
    for (auto &player : players) player->render(painter);
}

Player* GameWorld::getPlayer(int id)
{
    return id == 1 ? players[0].get() : players[1].get();
}

int GameWorld::getPlayerScore(int id) const
{
    return players[id-1]->getTotalScore();
}

bool GameWorld::isGameOver() const
{
    return tick > 60 * 60 * 3; // 3 minutes for demo
}

void GameWorld::spawnEntities()
{
    while (foods.size() < 3000)
        foods.emplace_back(std::make_unique<Food>());
    while (thorns.size() < 15)
        thorns.emplace_back(std::make_unique<Thorn>());
}

void GameWorld::handleCollisions()
{
    // Player eats food, spores, splits at thorns, etc.
    for (auto& player : players) {
        for (auto& ball : player->getBalls()) {
            for (auto& food : foods) {
                if (food->isAlive() && Physics::checkCollision(*ball, *food)) {
                    ball->setRadius(std::sqrt(ball->getRadius()*ball->getRadius() + 8*8));
                    food->kill();
                }
            }
            for (auto& spore : spores) {
                if (spore->isAlive() && Physics::checkCollision(*ball, *spore)) {
                    ball->setRadius(std::sqrt(ball->getRadius()*ball->getRadius() + 12*12));
                    spore->kill();
                }
            }
            for (auto& thorn : thorns) {
                if (thorn->isAlive() && Physics::checkCollision(*ball, *thorn)) {
                    if (ball->getRadius() > 37) {
                        float angle = 0;
                        for (int i=0; i<6; ++i, angle+=3.14159/3) {
                            float nx = ball->getX() + cos(angle) * ball->getRadius();
                            float ny = ball->getY() + sin(angle) * ball->getRadius();
                            float new_r = ball->getRadius()/2.0f;
                            player->getBalls().push_back(new PlayerBall(nx, ny, new_r, ball->getColor(), player->getId()));
                        }
                        ball->setRadius(ball->getRadius()/2.0f);
                    }
                }
            }
        }
    }
}

void GameWorld::addFood(std::unique_ptr<Food> food) { foods.push_back(std::move(food)); }
void GameWorld::addSpore(std::unique_ptr<Spore> spore) { spores.push_back(std::move(spore)); }
void GameWorld::addThorn(std::unique_ptr<Thorn> thorn) { thorns.push_back(std::move(thorn)); }

std::vector<Food*>& GameWorld::getFoods()
{
    static std::vector<Food*> ptrs;
    ptrs.clear();
    for (auto& f : foods) if(f->isAlive()) ptrs.push_back(f.get());
    return ptrs;
}
std::vector<Spore*>& GameWorld::getSpores()
{
    static std::vector<Spore*> ptrs;
    ptrs.clear();
    for (auto& s : spores) if(s->isAlive()) ptrs.push_back(s.get());
    return ptrs;
}
std::vector<Thorn*>& GameWorld::getThorns()
{
    static std::vector<Thorn*> ptrs;
    ptrs.clear();
    for (auto& t : thorns) if(t->isAlive()) ptrs.push_back(t.get());
    return ptrs;
}
std::vector<Player*>& GameWorld::getPlayers()
{
    static std::vector<Player*> ptrs;
    ptrs.clear();
    for (auto& p : players) ptrs.push_back(p.get());
    return ptrs;
}