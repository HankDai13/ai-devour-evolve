#pragma once

#include <QWidget>
#include <QPushButton>

class StartScreen : public QWidget {
    Q_OBJECT
public:
    explicit StartScreen(QWidget* parent = nullptr);

private slots:
    void onStartVsAI();
    void onStartVsHuman();
    void onExitClicked();

private:
    QPushButton* vsAIBtn;
    QPushButton* vsHumanBtn;
    QPushButton* exitBtn;
};
