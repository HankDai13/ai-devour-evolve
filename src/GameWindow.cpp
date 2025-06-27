#include "GameWindow.h"
#include <QKeyEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QMessageBox>
#include <QApplication>
#include "StartScreen.h"
#include "GameOverScreen.h"

GameWindow::GameWindow(int numPlayers_, int numAI_, QWidget* parent)
    : QWidget(parent), width(800), height(600), numPlayers(numPlayers_), numAI(numAI_) {
    setFixedSize(width, height + 21); // 21 for menu bar
    setFocusPolicy(Qt::StrongFocus);

    menuBar = new QMenuBar(this);
    menuBar->setGeometry(0, 0, width, 21);

    QMenu* menu = menuBar->addMenu("Menu");
    QAction* restartAction = menu->addAction("Restart");
    QAction* exitAction = menu->addAction("Exit");

    connect(restartAction, &QAction::triggered, [this]() {
        StartScreen* start = new StartScreen();
        start->show();
        this->close();
        });
    connect(exitAction, &QAction::triggered, this, []() {
        QApplication::quit();
        });

    controller = new GameController(numPlayers, numAI, width, height);

    mouseX = std::vector<float>(numPlayers, width / 2.f);
    mouseY = std::vector<float>(numPlayers, height / 2.f);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameWindow::onFrame);
    timer->start(16); // ~60 FPS
}

GameWindow::~GameWindow() {
    delete controller;
}

void GameWindow::onFrame() {
    // 鼠标控制：每帧更新玩家1目标点
    if (numPlayers > 0) {
        controller->setPlayerTarget(0, mouseX[0], mouseY[0]);
    }
    controller->update();
    update();
    if (controller->isGameOver()) {
        timer->stop();
        int winner = controller->getWinnerId();
        GameOverScreen* over = new GameOverScreen(winner);
        over->show();
        this->close();
    }
}

void GameWindow::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.translate(0, 21);
    controller->render(&painter);

    // 排行榜
    auto scores = controller->getScoreManager().getRankedList();
    int x = width - 170, y = 30;
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(x, y, "Leaderboard");
    y += 20;
    int cnt = 0;
    for (const auto& p : scores) {
        painter.setPen(Qt::black);
        painter.drawText(x, y, QString("Player %1: %2").arg(p.first).arg(int(p.second)));
        y += 18; if (++cnt > 6) break;
    }
}

void GameWindow::keyPressEvent(QKeyEvent* event) {
    if (numPlayers == 1) {
        controller->handleInput(0, event->key(), true, mouseX[0], mouseY[0]);
    }
    if (numPlayers == 2) {
        // 玩家1（playerId=0）：鼠标+Space/W
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_W)
            controller->handleInput(0, event->key(), true, mouseX[0], mouseY[0]);
        // 玩家2（playerId=1）：方向键+右Ctrl(分裂)+右Shift(孢子)
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
            event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
            event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift)
        {
            controller->handleInput(1, event->key(), true, mouseX[1], mouseY[1]);
        }
    }
}

void GameWindow::keyReleaseEvent(QKeyEvent* event) {
    if (numPlayers == 1) {
        controller->handleInput(0, event->key(), false, mouseX[0], mouseY[0]);
    }
    if (numPlayers == 2) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_W)
            controller->handleInput(0, event->key(), false, mouseX[0], mouseY[0]);
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
            event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
            event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift)
        {
            controller->handleInput(1, event->key(), false, mouseX[1], mouseY[1]);
        }
    }
}

void GameWindow::mouseMoveEvent(QMouseEvent* event) {
    if (numPlayers > 0) {
        mouseX[0] = event->position().x();
        mouseY[0] = event->pos().y() - 21;
    }
}

void GameWindow::resizeEvent(QResizeEvent* event) {
    menuBar->setGeometry(0, 0, width, 21);
    QWidget::resizeEvent(event);
}
