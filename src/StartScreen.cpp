#include "StartScreen.h"
#include <QVBoxLayout>
#include <QApplication>
#include "GameWindow.h"

StartScreen::StartScreen(QWidget* parent) : QWidget(parent) {
    setFixedSize(400, 300);
    setWindowTitle("Agar.io - Start");

    QVBoxLayout* layout = new QVBoxLayout(this);

    vsAIBtn = new QPushButton("Human vs AI");
    vsHumanBtn = new QPushButton("Human vs Human");
    exitBtn = new QPushButton("Exit");

    connect(vsAIBtn, &QPushButton::clicked, this, &StartScreen::onStartVsAI);
    connect(vsHumanBtn, &QPushButton::clicked, this, &StartScreen::onStartVsHuman);
    connect(exitBtn, &QPushButton::clicked, this, &StartScreen::onExitClicked);

    layout->addStretch();
    layout->addWidget(vsAIBtn);
    layout->addWidget(vsHumanBtn);
    layout->addWidget(exitBtn);
    layout->addStretch();
    setLayout(layout);
}

void StartScreen::onStartVsAI() {
    GameWindow* game = new GameWindow(1, 1); // 1 human + 1 AI
    game->show();
    this->close();
}

void StartScreen::onStartVsHuman() {
    GameWindow* game = new GameWindow(2, 0); // 2 humans
    game->show();
    this->close();
}

void StartScreen::onExitClicked() {
    QApplication::quit();
}
