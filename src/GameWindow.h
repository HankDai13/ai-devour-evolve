#pragma once

#include <QWidget>
#include <QMenuBar>
#include <QTimer>
#include "GameController.h"

class GameWindow : public QWidget {
    Q_OBJECT
public:
    GameWindow(int numPlayers, int numAI, QWidget* parent = nullptr);
    // 新增：获取玩家1鼠标目标点
    float getMouseX() const { return mouseX[0]; }
    float getMouseY() const { return mouseY[0]; }

    ~GameWindow();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private slots:
    void onFrame();

private:
    int width, height;
    std::vector<float> mouseX, mouseY;
    QMenuBar* menuBar;
    QTimer* timer;
    GameController* controller;
    int numPlayers;
    int numAI;
};