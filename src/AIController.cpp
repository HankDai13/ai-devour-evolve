#include "AIController.h"
#include "GameController.h"
#include "Player.h"
#include <cstdlib>
#include <cmath>

AIController::AIController(int pid)
    : playerId(pid)
{
}

void AIController::update(GameController& game) {
    auto& players = game.getPlayers();
    if (playerId >= players.size()) return;
    Player* player = players[playerId].get();
    if (!player || !player->isAlive()) return;

    // 降低AI移动速度：每次只移动一小步
    static float tx = rand() % game.getWidth();
    static float ty = rand() % game.getHeight();
    float px = player->getBalls()[0]->getX();
    float py = player->getBalls()[0]->getY();
    float dx = tx - px;
    float dy = ty - py;
    float dist = std::sqrt(dx * dx + dy * dy);
    float speed = 2.0f; // AI速度降低
    if (dist < speed) {
        tx = rand() % game.getWidth();
        ty = rand() % game.getHeight();
    }
    else {
        float nx = px + dx / dist * speed;
        float ny = py + dy / dist * speed;
        player->moveTo(nx, ny);
    }
}

int AIController::getPlayerId() const {
    return playerId;
}
