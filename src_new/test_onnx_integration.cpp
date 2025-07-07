#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QStandardPaths>
#include "SimpleAIPlayer.h"
#include "CloneBall.h"
#include "GameManager.h"
#include <memory>

using namespace GoBigger::AI;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "=== AI Model Integration Test ===";
    
    // 创建游戏场景
    auto gameManager = std::make_unique<GameManager>();
    gameManager->initializeGame();
    
    // 创建测试玩家球
    auto playerBall = gameManager->createPlayerBall(1, QPointF(400, 300), 20.0f);
    if (!playerBall) {
        qCritical() << "Failed to create player ball";
        return -1;
    }
    
    // 创建AI玩家
    auto aiPlayer = std::make_unique<SimpleAIPlayer>(playerBall);
    
    // 测试ONNX模型加载
    QString workspaceDir = QDir::currentPath();
    QString modelPath = QDir(workspaceDir).absoluteFilePath("assets/ai_models/exported_models/ai_model.onnx");
    
    qDebug() << "Current workspace:" << workspaceDir;
    qDebug() << "Model path:" << modelPath;
    qDebug() << "Model exists:" << QFileInfo(modelPath).exists();
    
    // 尝试加载模型
    bool modelLoaded = aiPlayer->loadAIModel(modelPath);
    qDebug() << "Model loaded:" << modelLoaded;
    
    if (modelLoaded) {
        qDebug() << "Testing MODEL_BASED strategy...";
        aiPlayer->setAIStrategy(SimpleAIPlayer::AIStrategy::MODEL_BASED);
    } else {
        qDebug() << "Model not loaded, testing FOOD_HUNTER strategy...";
        aiPlayer->setAIStrategy(SimpleAIPlayer::AIStrategy::FOOD_HUNTER);
    }
    
    // 设置AI决策间隔
    aiPlayer->setDecisionInterval(500); // 500ms间隔
    
    // 监听AI动作
    QObject::connect(aiPlayer.get(), &SimpleAIPlayer::actionExecuted, [](const AIAction& action) {
        qDebug() << QString("AI Action - dx: %1, dy: %2, type: %3")
                    .arg(action.dx, 0, 'f', 3)
                    .arg(action.dy, 0, 'f', 3)
                    .arg(static_cast<int>(action.type));
    });
    
    // 启动AI
    aiPlayer->startAI();
    qDebug() << "AI started, running for 10 seconds...";
    
    // 运行测试10秒
    QTimer::singleShot(10000, [&app]() {
        qDebug() << "Test completed, exiting...";
        app.quit();
    });
    
    // 每秒输出状态
    auto statusTimer = new QTimer(&app);
    QObject::connect(statusTimer, &QTimer::timeout, [&aiPlayer, &playerBall]() {
        if (playerBall) {
            QPointF pos = playerBall->pos();
            qDebug() << QString("Player position: (%.1f, %.1f), radius: %.1f")
                        .arg(pos.x()).arg(pos.y()).arg(playerBall->getRadius());
        }
        qDebug() << "AI active:" << aiPlayer->isAIActive() 
                 << "Strategy:" << static_cast<int>(aiPlayer->getAIStrategy())
                 << "Model loaded:" << aiPlayer->isModelLoaded();
    });
    statusTimer->start(2000); // 每2秒输出一次状态
    
    return app.exec();
}
