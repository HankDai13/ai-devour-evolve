// 简单测试程序验证荆棘球机制
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include "src/GameManager.h"
#include "src/ThornsBall.h"
#include "src/CloneBall.h"
#include "src/GoBiggerConfig.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "Testing Thorns Ball Mechanisms...";
    
    // 创建GameManager
    GameManager gameManager;
    
    // 开始游戏
    gameManager.startGame();
    
    qDebug() << "Initial thorns count:" << gameManager.getThornsCount();
    qDebug() << "Initial food count:" << gameManager.getFoodCount();
    qDebug() << "Initial player count:" << gameManager.getPlayerCount();
    
    // 创建一个定时器来定期打印状态
    QTimer* statusTimer = new QTimer();
    QObject::connect(statusTimer, &QTimer::timeout, [&gameManager]() {
        qDebug() << "Status - Thorns:" << gameManager.getThornsCount() 
                 << "Food:" << gameManager.getFoodCount()
                 << "Players:" << gameManager.getPlayerCount();
    });
    statusTimer->start(2000); // 每2秒打印一次状态
    
    // 3秒后退出测试
    QTimer::singleShot(3000, &app, &QApplication::quit);
    
    return app.exec();
}
