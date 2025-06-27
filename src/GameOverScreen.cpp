#include "GameOverScreen.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QPushButton> // Ensure QPushButton is included
#include <QObject>     // Ensure QObject is included for connect
#include "StartScreen.h"
#include "HumanController.h"
#include "GameController.h"
GameOverScreen::GameOverScreen(int winnerId, QWidget* parent) : QWidget(parent) {
    setFixedSize(400, 200);
    setWindowTitle("Game Over");

    QVBoxLayout* layout = new QVBoxLayout(this);

    QString winnerText = (winnerId >= 0) ? QString("Player %1 wins!").arg(winnerId) : "Game Over";
    QLabel* label = new QLabel(winnerText, this);
    label->setAlignment(Qt::AlignCenter);

    restartBtn = new QPushButton("Restart", this); // Ensure button has parent
    exitBtn = new QPushButton("Exit", this);       // Ensure button has parent

    // Connect signals and slots
    connect(restartBtn, &QPushButton::clicked, this, &GameOverScreen::onRestart);
    connect(exitBtn, &QPushButton::clicked, this, &GameOverScreen::onExit);

    layout->addWidget(label);
    layout->addWidget(restartBtn);
    layout->addWidget(exitBtn);
    setLayout(layout);
}

void GameOverScreen::onRestart() {
    StartScreen* start = new StartScreen();
    start->show();
    this->close();
}

void GameOverScreen::onExit() {
    QApplication::quit();
}
