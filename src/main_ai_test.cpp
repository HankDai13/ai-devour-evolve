#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QWidget>
#include "GameManager.h"
#include "MultiPlayerManager.h"
#include "SimpleAIPlayer.h"

class AITestWindow : public QWidget {
    Q_OBJECT
    
public:
    AITestWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupGame();
        connectSignals();
        
        // 设置默认AI模型路径
        QString defaultModelPath = "assets/ai_models/exported_models/ai_model_traced.pt";
        m_multiPlayerManager->setDefaultAIModel(defaultModelPath);
        m_modelPathLabel->setText(defaultModelPath);
    }
    
private slots:
    void selectAIModel() {
        QString modelPath = QFileDialog::getOpenFileName(
            this, 
            "Select AI Model", 
            "assets/ai_models/exported_models", 
            "PyTorch Models (*.pt);;ONNX Models (*.onnx);;All Files (*)"
        );
        
        if (!modelPath.isEmpty()) {
            m_multiPlayerManager->setDefaultAIModel(modelPath);
            m_modelPathLabel->setText(modelPath);
            qDebug() << "Selected AI model:" << modelPath;
        }
    }
    
    void startAIvsAI() {
        int aiCount = m_aiCountSpinBox->value();
        if (aiCount < 2) {
            QMessageBox::warning(this, "Warning", "Need at least 2 AI players for AI vs AI mode");
            return;
        }
        
        // 清除现有玩家
        m_multiPlayerManager->removeAllPlayers();
        
        // 创建AI vs AI模式
        auto players = GoBigger::Multiplayer::GameModeHelper::createAIvsAIMode(
            aiCount, m_multiPlayerManager->getDefaultAIModel()
        );
        
        // 添加玩家
        for (const auto& player : players) {
            if (!m_multiPlayerManager->addPlayer(player)) {
                QMessageBox::critical(this, "Error", "Failed to add AI player");
                return;
            }
        }
        
        // 启动游戏
        m_multiPlayerManager->startMultiPlayerGame();
        
        updateGameStatus();
        qDebug() << "Started AI vs AI game with" << aiCount << "players";
    }
    
    void stopGame() {
        m_multiPlayerManager->stopMultiPlayerGame();
        updateGameStatus();
        qDebug() << "Game stopped";
    }
    
    void pauseResumeGame() {
        if (m_multiPlayerManager->isGameRunning()) {
            if (m_multiPlayerManager->isGamePaused()) {
                m_multiPlayerManager->resumeMultiPlayerGame();
                m_pauseResumeButton->setText("Pause Game");
            } else {
                m_multiPlayerManager->pauseMultiPlayerGame();
                m_pauseResumeButton->setText("Resume Game");
            }
        }
        updateGameStatus();
    }
    
    void onPlayerCountChanged(int total, int ai, int human) {
        m_playerCountLabel->setText(QString("Players: %1 (AI: %2, Human: %3)")
                                   .arg(total).arg(ai).arg(human));
    }
    
    void onGameStarted() {
        m_gameStatusLabel->setText("Game Status: Running");
        m_gameStatusLabel->setStyleSheet("color: green;");
        m_startAIvsAIButton->setEnabled(false);
        m_stopGameButton->setEnabled(true);
        m_pauseResumeButton->setEnabled(true);
        m_pauseResumeButton->setText("Pause Game");
    }
    
    void onGameStopped() {
        m_gameStatusLabel->setText("Game Status: Stopped");
        m_gameStatusLabel->setStyleSheet("color: red;");
        m_startAIvsAIButton->setEnabled(true);
        m_stopGameButton->setEnabled(false);
        m_pauseResumeButton->setEnabled(false);
    }
    
    void onGamePaused() {
        m_gameStatusLabel->setText("Game Status: Paused");
        m_gameStatusLabel->setStyleSheet("color: orange;");
    }
    
    void onGameResumed() {
        m_gameStatusLabel->setText("Game Status: Running");
        m_gameStatusLabel->setStyleSheet("color: green;");
    }
    
private:
    void setupUI() {
        setWindowTitle("AI Integration Test - GoBigger");
        setMinimumSize(1200, 800);
        
        // 主布局
        auto mainLayout = new QHBoxLayout(this);
        
        // 游戏视图
        m_gameView = new QGraphicsView();
        m_gameView->setMinimumSize(800, 600);
        mainLayout->addWidget(m_gameView);
        
        // 控制面板
        auto controlPanel = new QWidget();
        controlPanel->setMaximumWidth(350);
        controlPanel->setMinimumWidth(300);
        auto controlLayout = new QVBoxLayout(controlPanel);
        
        // AI模型选择
        controlLayout->addWidget(new QLabel("AI Model Configuration:"));
        m_modelPathLabel = new QLabel("No model selected");
        m_modelPathLabel->setWordWrap(true);
        m_modelPathLabel->setStyleSheet("border: 1px solid gray; padding: 5px;");
        controlLayout->addWidget(m_modelPathLabel);
        
        auto selectModelButton = new QPushButton("Select AI Model");
        connect(selectModelButton, &QPushButton::clicked, this, &AITestWindow::selectAIModel);
        controlLayout->addWidget(selectModelButton);
        
        controlLayout->addWidget(new QLabel(""));
        
        // 游戏配置
        controlLayout->addWidget(new QLabel("Game Configuration:"));
        
        auto aiCountLayout = new QHBoxLayout();
        aiCountLayout->addWidget(new QLabel("AI Players:"));
        m_aiCountSpinBox = new QSpinBox();
        m_aiCountSpinBox->setRange(2, 8);
        m_aiCountSpinBox->setValue(4);
        aiCountLayout->addWidget(m_aiCountSpinBox);
        controlLayout->addLayout(aiCountLayout);
        
        controlLayout->addWidget(new QLabel(""));
        
        // 游戏控制
        controlLayout->addWidget(new QLabel("Game Control:"));
        
        m_startAIvsAIButton = new QPushButton("Start AI vs AI");
        connect(m_startAIvsAIButton, &QPushButton::clicked, this, &AITestWindow::startAIvsAI);
        controlLayout->addWidget(m_startAIvsAIButton);
        
        m_pauseResumeButton = new QPushButton("Pause Game");
        m_pauseResumeButton->setEnabled(false);
        connect(m_pauseResumeButton, &QPushButton::clicked, this, &AITestWindow::pauseResumeGame);
        controlLayout->addWidget(m_pauseResumeButton);
        
        m_stopGameButton = new QPushButton("Stop Game");
        m_stopGameButton->setEnabled(false);
        connect(m_stopGameButton, &QPushButton::clicked, this, &AITestWindow::stopGame);
        controlLayout->addWidget(m_stopGameButton);
        
        controlLayout->addWidget(new QLabel(""));
        
        // 状态显示
        controlLayout->addWidget(new QLabel("Game Status:"));
        
        m_gameStatusLabel = new QLabel("Game Status: Stopped");
        m_gameStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        controlLayout->addWidget(m_gameStatusLabel);
        
        m_playerCountLabel = new QLabel("Players: 0 (AI: 0, Human: 0)");
        controlLayout->addWidget(m_playerCountLabel);
        
        controlLayout->addStretch();
        
        // 调试信息
        controlLayout->addWidget(new QLabel("Debug Info:"));
        m_debugLabel = new QLabel("Ready");
        m_debugLabel->setStyleSheet("font-family: monospace; font-size: 10px;");
        m_debugLabel->setWordWrap(true);
        controlLayout->addWidget(m_debugLabel);
        
        mainLayout->addWidget(controlPanel);
    }
    
    void setupGame() {
        // 创建游戏场景
        m_gameScene = new QGraphicsScene(this);
        m_gameScene->setSceneRect(-400, -400, 800, 800);
        m_gameView->setScene(m_gameScene);
        
        // 创建游戏管理器
        GameManager::Config config;
        m_gameManager = new GameManager(m_gameScene, config, this);
        
        // 创建多玩家管理器
        m_multiPlayerManager = new GoBigger::Multiplayer::MultiPlayerManager(m_gameManager, this);
    }
    
    void connectSignals() {
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::playerCountChanged,
                this, &AITestWindow::onPlayerCountChanged);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameStarted,
                this, &AITestWindow::onGameStarted);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameStopped,
                this, &AITestWindow::onGameStopped);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gamePaused,
                this, &AITestWindow::onGamePaused);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameResumed,
                this, &AITestWindow::onGameResumed);
        
        // 定时更新调试信息
        auto debugTimer = new QTimer(this);
        connect(debugTimer, &QTimer::timeout, this, [this]() {
            if (m_gameManager) {
                QString debugInfo = QString("Food: %1, Thorns: %2, Players: %3")
                                  .arg(m_gameManager->getFoodCount())
                                  .arg(m_gameManager->getThornsCount())
                                  .arg(m_gameManager->getPlayerCount());
                m_debugLabel->setText(debugInfo);
            }
        });
        debugTimer->start(1000); // 每秒更新一次
    }
    
    void updateGameStatus() {
        // 状态已通过信号更新
    }
    
private:
    QGraphicsView* m_gameView;
    QGraphicsScene* m_gameScene;
    GameManager* m_gameManager;
    GoBigger::Multiplayer::MultiPlayerManager* m_multiPlayerManager;
    
    // UI控件
    QLabel* m_modelPathLabel;
    QSpinBox* m_aiCountSpinBox;
    QPushButton* m_startAIvsAIButton;
    QPushButton* m_pauseResumeButton;
    QPushButton* m_stopGameButton;
    QLabel* m_gameStatusLabel;
    QLabel* m_playerCountLabel;
    QLabel* m_debugLabel;
};

#include "main_ai_test.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    AITestWindow window;
    window.show();
    
    return app.exec();
}
