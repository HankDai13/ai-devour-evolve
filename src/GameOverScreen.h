#pragma once

#include <QWidget>
#include <QPushButton>

class GameOverScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameOverScreen(int winnerId, QWidget* parent = nullptr);

private slots:
    void onRestart();
    void onExit();

private:
    QPushButton* restartBtn;
    QPushButton* exitBtn;
};
