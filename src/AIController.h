#pragma once

class GameController;

class AIController {
public:
    AIController(int playerId);

    void update(GameController& game);

    int getPlayerId() const;
private:
    int playerId;
};