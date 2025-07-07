#pragma once

class GameController;

class HumanController {
public:
    HumanController(int playerId, bool useArrowKeys = false);

    void onKeyPress(GameController& game, int key, float mouseX, float mouseY);
    void onKeyRelease(GameController& game, int key);

    int getPlayerId() const;

private:
    int playerId;
    bool useArrowKeys; // true: �÷���������2����false: �����ƣ����1��
    int moveX = 0, moveY = 0;
};