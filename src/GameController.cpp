#include "GameController.h"
#include "Physics.h"
#include "Utils.h"
#include "HumanController.h"
#include "AIController.h"
#include "Ball.h"
#include <algorithm>
#include <cmath>

GameController::GameController(int numPlayers, int numAI, int width, int height)
    : width(width), height(height), gameOver(false), winnerId(-1)
{
    int playerId = 0;
    std::vector<std::pair<float, float>> usedPositions;
    auto getNonOverlappingPos = [&](float radius) {
        while (true) {
            float x = Utils::randomFloat(80, width - 80);
            float y = Utils::randomFloat(80, height - 80);
            bool overlap = false;
            for (const auto& pos : usedPositions) {
                float dx = x - pos.first, dy = y - pos.second;
                if (std::sqrt(dx * dx + dy * dy) < radius * 2 + 10) {
                    overlap = true;
                    break;
                }
            }
            if (!overlap) {
                usedPositions.emplace_back(x, y);
                return std::make_pair(x, y);
            }
        }
        };

    float initRadius = 24.0f;
    if (numPlayers == 1) {
        // 人vsAI，只有一个人类控制器
        auto pos = getNonOverlappingPos(initRadius);
        players.emplace_back(std::make_unique<Player>(playerId, Utils::randomColor(), pos.first, pos.second, initRadius));
        humanControllers.emplace_back(std::make_unique<HumanController>(playerId, false));
        ++playerId;
        for (int i = 0; i < numAI; ++i, ++playerId) {
            auto posAI = getNonOverlappingPos(initRadius);
            players.emplace_back(std::make_unique<Player>(playerId, Utils::randomColor(), posAI.first, posAI.second, initRadius));
            aiControllers.emplace_back(std::make_unique<AIController>(playerId));
        }
    }
    else if (numPlayers == 2) {
        // 人vs人，两个都是人类控制器
        auto pos1 = getNonOverlappingPos(initRadius);
        players.emplace_back(std::make_unique<Player>(playerId, Utils::randomColor(), pos1.first, pos1.second, initRadius));
        humanControllers.emplace_back(std::make_unique<HumanController>(playerId, false));
        ++playerId;
        auto pos2 = getNonOverlappingPos(initRadius);
        players.emplace_back(std::make_unique<Player>(playerId, Utils::randomColor(), pos2.first, pos2.second, initRadius));
        humanControllers.emplace_back(std::make_unique<HumanController>(playerId, true));
    }
    spawnFood();
    spawnThorns();

    // 初始化目标点
    playerTargets.resize(players.size(), { width / 2.0f, height / 2.0f });
}

void GameController::setPlayerTarget(int playerId, float x, float y) {
    if (playerTargets.size() <= playerId) playerTargets.resize(playerId + 1, { 0, 0 });
    playerTargets[playerId] = { x, y };
}

void GameController::update() {
    if (gameOver) return;
    // AI更新
    for (auto& ai : aiControllers)
        ai->update(*this);

    // 玩家更新
    for (auto& player : players)
        if (player->isAlive()) player->update();

    // 鼠标控制：玩家1自动朝目标点移动
    if (!players.empty() && !playerTargets.empty()) {
        Player* player = players[0].get();
        if (player && player->isAlive()) {
            float tx = playerTargets[0].first;
            float ty = playerTargets[0].second;
            auto& balls = player->getBalls();
            if (!balls.empty()) {
                Ball* ball = balls[0].get();
                float px = ball->getX();
                float py = ball->getY();
                float dx = tx - px;
                float dy = ty - py;
                float dist = std::sqrt(dx * dx + dy * dy);
                float speed = 5.0f;
                if (dist > 1.0f) {
                    float nx = px + dx / dist * std::min(speed, dist);
                    float ny = py + dy / dist * std::min(speed, dist);
                    player->moveTo(nx, ny);
                }
            }
        }
    }

    for (auto& spore : spores)
        if (spore->isAlive()) spore->update();

    handleCollisions();

    foods.erase(std::remove_if(foods.begin(), foods.end(),
        [](auto& f) { return !f->isAlive(); }), foods.end());
    spores.erase(std::remove_if(spores.begin(), spores.end(),
        [](auto& s) { return !s->isAlive(); }), spores.end());
    thorns.erase(std::remove_if(thorns.begin(), thorns.end(),
        [](auto& t) { return !t->isAlive(); }), thorns.end());

    scoreManager.updateScores(players);

    checkGameOver();
}

void GameController::render(QPainter* painter) {
    for (const auto& food : foods)
        if (food->isAlive()) food->render(painter);
    for (const auto& spore : spores)
        if (spore->isAlive()) spore->render(painter);
    for (const auto& thorn : thorns)
        if (thorn->isAlive()) thorn->render(painter);
    for (const auto& player : players)
        if (player->isAlive()) player->render(painter);
}

void GameController::handleInput(int playerId, int key, bool pressed, float mouseX, float mouseY) {
    for (auto& ctrl : humanControllers) {
        if (ctrl->getPlayerId() == playerId) {
            if (pressed)
                ctrl->onKeyPress(*this, key, mouseX, mouseY);
            else
                ctrl->onKeyRelease(*this, key);
        }
    }
}

std::vector<std::unique_ptr<Player>>& GameController::getPlayers() { return players; }
std::vector<std::unique_ptr<Food>>& GameController::getFoods() { return foods; }
std::vector<std::unique_ptr<Spore>>& GameController::getSpores() { return spores; }
std::vector<std::unique_ptr<Thorn>>& GameController::getThorns() { return thorns; }
ScoreManager& GameController::getScoreManager() { return scoreManager; }
int GameController::getWidth() const { return width; }
int GameController::getHeight() const { return height; }
bool GameController::isGameOver() const { return gameOver; }
int GameController::getWinnerId() const { return winnerId; }

void GameController::handleCollisions() {
    // 玩家吃食物
    for (auto& player : players) {
        for (auto& ball : player->getBalls()) {
            for (auto& food : foods) {
                if (food->isAlive() && Physics::canEat(*ball, *food)) {
                    food->kill();
                    ball->grow(food->getRadius() * 0.4f);
                }
            }
        }
    }
    // 玩家互吃
    for (size_t i = 0; i < players.size(); ++i) {
        for (size_t j = 0; j < players.size(); ++j) {
            if (i == j) continue;
            for (auto& ballA : players[i]->getBalls()) {
                for (auto& ballB : players[j]->getBalls()) {
                    if (!ballA->isAlive() || !ballB->isAlive()) continue;
                    if (Physics::canEat(*ballA, *ballB, 1.1f)) {
                        ballB->kill();
                        ballA->grow(ballB->getRadius() * 0.8f);
                    }
                }
            }
        }
    }
    // 玩家吃孢子
    for (auto& player : players) {
        for (auto& ball : player->getBalls()) {
            for (auto& spore : spores) {
                if (spore->isAlive() && Physics::canEat(*ball, *spore)) {
                    spore->kill();
                    ball->grow(spore->getRadius() * 0.3f);
                }
            }
        }
    }
    // 玩家碰荆棘球
    for (auto& player : players) {
        for (auto& ball : player->getBalls()) {
            for (auto& thorn : thorns) {
                if (thorn->isAlive() && Physics::canEat(*ball, *thorn, 1.0f)) {
                    if (ball->getRadius() > thorn->getRadius() * 1.15f) {
                        // 分裂并生成孢子
                        float bx = ball->getX(), by = ball->getY();
                        for (int i = 0; i < 8; ++i) {
                            float angle = i * 2 * M_PI / 8;
                            float sx = bx + std::cos(angle) * (thorn->getRadius() + 10);
                            float sy = by + std::sin(angle) * (thorn->getRadius() + 10);
                            spores.emplace_back(std::make_unique<Spore>(sx, sy, 8.0f, QColor(120, 255, 120)));
                        }
                        ball->splitRandom();
                        thorn->kill();
                    }
                }
            }
        }
    }
}

void GameController::spawnFood() {
    for (int i = 0; i < 80; ++i) {
        float fx = Utils::randomFloat(30, width - 30);
        float fy = Utils::randomFloat(30, height - 30);
        float fr = Utils::randomFloat(6.0f, 12.0f);
        QColor color = Utils::randomColor();
        foods.emplace_back(std::make_unique<Food>(fx, fy, fr, color));
    }
}

void GameController::spawnThorns() {
    for (int i = 0; i < 8; ++i) {
        float tx = Utils::randomFloat(60, width - 60);
        float ty = Utils::randomFloat(60, height - 60);
        float tr = Utils::randomFloat(16.0f, 26.0f);
        QColor color(0, 180, 0);
        thorns.emplace_back(std::make_unique<Thorn>(tx, ty, tr, color));
    }
}

void GameController::checkGameOver() {
    int aliveCount = 0;
    int lastAlive = -1;
    for (const auto& player : players) {
        if (player->isAlive()) {
            ++aliveCount;
            lastAlive = player->getId();
        }
    }
    if (aliveCount <= 1 && !gameOver) {
        gameOver = true;
        winnerId = lastAlive;
    }
}
