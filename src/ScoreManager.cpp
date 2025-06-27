#include "ScoreManager.h"
#include "Player.h"
#include <algorithm>
#include "Ball.h"
void ScoreManager::updateScores(const std::vector<std::unique_ptr<Player>>& players) {
    scores.clear();
    for (const auto& player : players) {
        float total = 0;
        for (const auto& ball : player->getBalls())
            if (ball->isAlive()) total += ball->getRadius();
        scores.emplace_back(player->getId(), total);
    }
    std::sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
        });
}

std::vector<std::pair<int, float>> ScoreManager::getRankedList() const {
    return scores;
}