#include "HumanController.h"
#include "GameController.h"
#include "Player.h"
#include "Ball.h" // 添加此行以包含 Ball 的定义
#include <QKeyEvent>

HumanController::HumanController(int pid, bool arrow)
    : playerId(pid), useArrowKeys(arrow) {
}

// 其余代码保持不变


void HumanController::onKeyPress(GameController& game, int key, float mouseX, float mouseY) {
    auto& players = game.getPlayers();
    if (playerId >= players.size()) return;
    Player* player = players[playerId].get();
    if (!player || !player->isAlive()) return;

    if (!useArrowKeys) {
        // 玩家1：鼠标+Space/W
        if (key == Qt::Key_Space) {
            player->split(mouseX, mouseY);
        }
        else if (key == Qt::Key_W) {
            player->ejectSpore();
        }
    }
    else {
        // 玩家2：方向键+右Ctrl(分裂)+右Shift(孢子)
        switch (key) {
        case Qt::Key_Up:    moveY = -1; break;
        case Qt::Key_Down:  moveY = 1; break;
        case Qt::Key_Left:  moveX = -1; break;
        case Qt::Key_Right: moveX = 1; break;
        case Qt::Key_Control: {
            float dirX = moveX, dirY = moveY;
            if (dirX == 0 && dirY == 0) dirY = -1;
            float px = player->getBalls()[0]->getX();
            float py = player->getBalls()[0]->getY();
            player->split(px + dirX * 100, py + dirY * 100);
            break;
        }
        case Qt::Key_Shift:
            player->ejectSpore();
            break;
        }
        // 持续移动
        player->setMoveDelta(moveX, moveY);
    }
}

void HumanController::onKeyRelease(GameController& game, int key) {
    if (useArrowKeys) {
        switch (key) {
        case Qt::Key_Up:
        case Qt::Key_Down: moveY = 0; break;
        case Qt::Key_Left:
        case Qt::Key_Right: moveX = 0; break;
        }
        // 停止移动
        auto& players = game.getPlayers();
        if (playerId < players.size()) {
            players[playerId]->setMoveDelta(moveX, moveY);
        }
    }
}

int HumanController::getPlayerId() const {
    return playerId;
}