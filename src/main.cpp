#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include "GameView.h"
#include "CloneBall.h"  // 添加CloneBall头文件

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow mainWindow;
    
    // 创建GameView实例
    GameView *gameView = new GameView(&mainWindow);
    
    // 设置主窗口
    mainWindow.setCentralWidget(gameView);
    mainWindow.setWindowTitle("智能吞噬进化 - AI Devour Evolve");
    mainWindow.resize(1200, 800);
    
    // 创建菜单栏
    QMenuBar *menuBar = mainWindow.menuBar();
    QMenu *gameMenu = menuBar->addMenu("游戏");
    
    // 添加游戏控制菜单项
    QAction *startAction = gameMenu->addAction("开始游戏 (P)");
    QAction *pauseAction = gameMenu->addAction("暂停游戏 (P)");
    QAction *resetAction = gameMenu->addAction("重置游戏 (Esc)");
    gameMenu->addSeparator();
    QAction *exitAction = gameMenu->addAction("退出");
    
    // 创建AI菜单
    QMenu *aiMenu = menuBar->addMenu("AI");
    QAction *addAIAction = aiMenu->addAction("添加AI玩家");
    QAction *addRLAIAction = aiMenu->addAction("添加RL-AI玩家");
    aiMenu->addSeparator();
    QAction *startAllAIAction = aiMenu->addAction("启动所有AI");
    QAction *stopAllAIAction = aiMenu->addAction("停止所有AI");
    QAction *removeAllAIAction = aiMenu->addAction("移除所有AI");
    aiMenu->addSeparator();
    QAction *showDebugAction = aiMenu->addAction("显示AI调试控制台");
    showDebugAction->setShortcut(QKeySequence("F12"));
    
    // 连接游戏菜单动作
    QObject::connect(startAction, &QAction::triggered, gameView, &GameView::startGame);
    QObject::connect(pauseAction, &QAction::triggered, gameView, &GameView::pauseGame);
    QObject::connect(resetAction, &QAction::triggered, gameView, &GameView::resetGame);
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    
    // 连接AI菜单动作
    QObject::connect(addAIAction, &QAction::triggered, gameView, &GameView::addAIPlayer);
    QObject::connect(addRLAIAction, &QAction::triggered, gameView, &GameView::addRLAIPlayer);
    QObject::connect(startAllAIAction, &QAction::triggered, gameView, &GameView::startAllAI);
    QObject::connect(stopAllAIAction, &QAction::triggered, gameView, &GameView::stopAllAI);
    QObject::connect(removeAllAIAction, &QAction::triggered, gameView, &GameView::removeAllAI);
    QObject::connect(showDebugAction, &QAction::triggered, gameView, &GameView::showAIDebugConsole);
    
    // 创建帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助");
    QAction *controlsAction = helpMenu->addAction("控制说明");
    
    QObject::connect(controlsAction, &QAction::triggered, [&mainWindow]() {
        QMessageBox::information(&mainWindow, "控制说明", 
            "游戏控制:\n"
            "WASD 或 方向键 - 移动\n"
            "空格键 或 左键 - 分裂\n"
            "R键 或 右键 - 喷射孢子\n"
            "P键 - 暂停/继续游戏\n"
            "Esc键 - 重置游戏\n"
            "鼠标滚轮 - 缩放视野\n\n"
            "游戏规则:\n"
            "- 吞噬比自己小的球来成长\n"
            "- 避免被更大的球吞噬\n"
            "- 分裂可以增加机动性，但会减小单个球的大小\n"
            "- 荆棘球会对玩家造成伤害并可能导致分裂\n"
            "- 团队合作是获胜的关键"
        );
    });
    
    // 创建状态栏
    QStatusBar *statusBar = mainWindow.statusBar();
    QLabel *statusLabel = new QLabel();
    statusLabel->setText("按P键开始游戏");
    statusBar->addWidget(statusLabel);
    
    // 定时更新状态栏
    QTimer *statusTimer = new QTimer(&mainWindow);
    QObject::connect(statusTimer, &QTimer::timeout, [gameView, statusLabel]() {
        if (gameView->isGameRunning()) {
            CloneBall* player = gameView->getMainPlayer();
            if (player && !player->isRemoved()) {
                float totalScore = gameView->getTotalPlayerScore();
                statusLabel->setText(QString("游戏进行中 - 总分数: %1 | 主球大小: %2")
                    .arg(QString::number(totalScore, 'f', 1))
                    .arg(QString::number(player->radius(), 'f', 1)));
            } else {
                statusLabel->setText("游戏进行中 - 玩家已被淘汰");
            }
        } else {
            statusLabel->setText("游戏已暂停 - 按P键继续");
        }
    });
    statusTimer->start(100);
    
    // 显示窗口
    mainWindow.show();
    
    // 自动开始游戏
    QTimer::singleShot(500, gameView, &GameView::startGame);
    
    return app.exec();
}