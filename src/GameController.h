#pragma once

#include <vector>
#include <memory>
#include <QPainter>
#include "Player.h"
#include "Food.h"
#include "Spore.h"
#include "Thorn.h"
#include "ScoreManager.h"
#include "HumanController.h"
#include "AIController.h"

class GameController {
public:
    GameController(int numPlayers, int numAI, int width, int height);

    void update();
    void render(QPainter* painter);

    void handleInput(int playerId, int key, bool pressed, float mouseX, float mouseY);

    std::vector<std::unique_ptr<Player>>& getPlayers();
    std::vector<std::unique_ptr<Food>>& getFoods();
    std::vector<std::unique_ptr<Spore>>& getSpores();
    std::vector<std::unique_ptr<Thorn>>& getThorns();
    ScoreManager& getScoreManager();
    int getWidth() const;
    int getHeight() const;
    bool isGameOver() const;
    int getWinnerId() const;

    // 新增：设置玩家目标点（用于鼠标控制）
    void setPlayerTarget(int playerId, float x, float y);

private:
    void handleCollisions();
    void spawnFood();
    void spawnThorns();
    void checkGameOver();

    int width, height;
    bool gameOver;
    int winnerId;

    std::vector<std::unique_ptr<Player>> players;
    std::vector<std::unique_ptr<Food>> foods;
    std::vector<std::unique_ptr<Spore>> spores;
    std::vector<std::unique_ptr<Thorn>> thorns;
    ScoreManager scoreManager;
    std::vector<std::unique_ptr<HumanController>> humanControllers;
    std::vector<std::unique_ptr<AIController>> aiControllers;

    // 新增：每个玩家的目标点
    std::vector<std::pair<float, float>> playerTargets;
};

