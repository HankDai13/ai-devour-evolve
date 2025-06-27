#pragma once

#include <vector>
#include <utility>
#include <memory>

class Player;

class ScoreManager {
public:
    void updateScores(const std::vector<std::unique_ptr<Player>>& players);
    std::vector<std::pair<int, float>> getRankedList() const;
private:
    std::vector<std::pair<int, float>> scores; // <playerId, score>
};