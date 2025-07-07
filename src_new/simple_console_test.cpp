#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QGraphicsScene>
#include "SimpleAIPlayer.h"
#include "CloneBall.h"
#include "GameManager.h"
#include <memory>

using namespace GoBigger::AI;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Simple AI Test (Console Mode) ===";
    qDebug() << "Application created";
    
    try {
        qDebug() << "Creating graphics scene...";
        // 创建图形场景（AI需要但不会显示）
        auto scene = std::make_unique<QGraphicsScene>();
        scene->setSceneRect(-1000, -1000, 2000, 2000);
        qDebug() << "Graphics scene created";
        
        qDebug() << "Creating game manager...";
        // 创建游戏管理器
        auto gameManager = std::make_unique<GameManager>(scene.get());
        qDebug() << "Game manager created, starting game...";
        gameManager->startGame();
        qDebug() << "Game started";
        
        qDebug() << "Creating player ball...";
        // 创建测试玩家球
        auto playerBall = gameManager->createPlayer(1, 1, QPointF(400, 300));
        if (!playerBall) {
            qCritical() << "Failed to create player ball";
            return -1;
        }
        qDebug() << "Player ball created successfully with ID:" << playerBall->ballId();
        
        qDebug() << "Creating AI player...";
        // 创建AI玩家
        auto aiPlayer = std::make_unique<SimpleAIPlayer>(playerBall);
        qDebug() << "AI player created";
        
        // 尝试加载AI模型
        QString workspaceDir = QDir::currentPath();
        QString modelPath = QDir(workspaceDir).absoluteFilePath("assets/ai_models/exported_models/ai_model.onnx");
        
        qDebug() << "Current workspace:" << workspaceDir;
        qDebug() << "Model path:" << modelPath;
        qDebug() << "Model exists:" << QFileInfo(modelPath).exists();
        
        qDebug() << "Attempting to load AI model...";
        bool modelLoaded = aiPlayer->loadAIModel(modelPath);
        qDebug() << "Model loaded:" << modelLoaded;
        
        // 设置策略
        if (modelLoaded) {
            qDebug() << "Using MODEL_BASED strategy";
            aiPlayer->setAIStrategy(SimpleAIPlayer::AIStrategy::MODEL_BASED);
        } else {
            qDebug() << "Using FOOD_HUNTER strategy (fallback)";
            aiPlayer->setAIStrategy(SimpleAIPlayer::AIStrategy::FOOD_HUNTER);
        }
        
        // 设置决策间隔
        aiPlayer->setDecisionInterval(1000); // 1秒间隔
        
        // 监听AI动作
        int actionCount = 0;
        QObject::connect(aiPlayer.get(), &SimpleAIPlayer::actionExecuted, [&actionCount](const AIAction& action) {
            actionCount++;
            qDebug() << QString("Action #%1 - dx: %2, dy: %3, type: %4")
                        .arg(actionCount)
                        .arg(action.dx, 0, 'f', 3)
                        .arg(action.dy, 0, 'f', 3)
                        .arg(static_cast<int>(action.type));
        });
        
        // 启动AI
        aiPlayer->startAI();
        qDebug() << "AI started, will run for 5 seconds...";
        
        // 运行5秒后退出
        QTimer::singleShot(5000, [&app, &actionCount]() {
            qDebug() << QString("Test completed. Total actions: %1").arg(actionCount);
            app.quit();
        });
        
        return app.exec();
        
    } catch (const std::exception& e) {
        qCritical() << "Test failed with exception:" << e.what();
        return -1;
    }
}
