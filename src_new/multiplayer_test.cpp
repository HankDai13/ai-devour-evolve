#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QWidget>
#include "GameManager.h"
#include "MultiPlayerManager.h"
#include "SimpleAIPlayer.h"

class MultiPlayerTestWindow : public QWidget {
    Q_OBJECT
    
public:
    MultiPlayerTestWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupGame();
        connectSignals();
        
        // 启动状态更新定时器
        m_statusTimer = new QTimer(this);
        connect(m_statusTimer, &QTimer::timeout, this, &MultiPlayerTestWindow::updateStatus);
        m_statusTimer->start(1000); // 每秒更新一次
    }
    
private slots:
    void startAIBattle() {
        int aiCount = m_aiCountSpinBox->value();
        if (aiCount < 2) {
            QMessageBox::warning(this, "Warning", "Need at least 2 AI players for battle");
            return;
        }
        
        // 停止现有游戏
        m_multiPlayerManager->stopMultiPlayerGame();
        m_multiPlayerManager->removeAllPlayers();
        
        // 创建AI vs AI模式
        auto players = GoBigger::Multiplayer::GameModeHelper::createAIvsAIMode(aiCount, "");
        
        // 设置AI策略
        for (int i = 0; i < players.size(); ++i) {
            // 为不同AI设置不同策略
            // 这个将在添加玩家后设置
        }
        
        // 添加玩家
        for (const auto& player : players) {
            if (!m_multiPlayerManager->addPlayer(player)) {
                QMessageBox::critical(this, "Error", "Failed to add AI player");
                return;
            }
        }
        
        // 设置AI策略
        auto aiPlayers = m_gameManager->getAIPlayers();
        for (int i = 0; i < aiPlayers.size() && i < aiCount; ++i) {
            GoBigger::AI::SimpleAIPlayer::AIStrategy strategy;
            switch (i % 3) {
                case 0: strategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER; break;
                case 1: strategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE; break;
                case 2: strategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM; break;
            }
            aiPlayers[i]->setAIStrategy(strategy);
            aiPlayers[i]->setDecisionInterval(m_speedSlider->value());
        }
        
        // 启动游戏
        m_multiPlayerManager->startMultiPlayerGame();
        
        updateGameControls();
        qDebug() << "Started AI battle with" << aiCount << "players";
    }
    
    void stopGame() {
        m_multiPlayerManager->stopMultiPlayerGame();
        updateGameControls();
        qDebug() << "Game stopped";
    }
    
    void pauseResumeGame() {
        if (m_multiPlayerManager->isGameRunning()) {
            if (m_multiPlayerManager->isGamePaused()) {
                m_multiPlayerManager->resumeMultiPlayerGame();
            } else {
                m_multiPlayerManager->pauseMultiPlayerGame();
            }
        }
        updateGameControls();
    }
    
    void adjustAISpeed() {
        int speed = m_speedSlider->value();
        auto aiPlayers = m_gameManager->getAIPlayers();
        for (auto aiPlayer : aiPlayers) {
            if (aiPlayer) {
                aiPlayer->setDecisionInterval(speed);
            }
        }
        m_speedLabel->setText(QString("AI Speed: %1ms").arg(speed));
    }
    
    void onPlayerCountChanged(int total, int ai, int human) {
        m_playerCountLabel->setText(QString("Players: %1 (AI: %2)").arg(total).arg(ai));
    }
    
    void onGameStarted() {
        m_gameStatusLabel->setText("Status: Running");
        m_gameStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        updateGameControls();
    }
    
    void onGameStopped() {
        m_gameStatusLabel->setText("Status: Stopped");
        m_gameStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        updateGameControls();
    }
    
    void onGamePaused() {
        m_gameStatusLabel->setText("Status: Paused");
        m_gameStatusLabel->setStyleSheet("color: orange; font-weight: bold;");
        updateGameControls();
    }
    
    void onGameResumed() {
        m_gameStatusLabel->setText("Status: Running");
        m_gameStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        updateGameControls();
    }
    
    void updateStatus() {
        if (m_gameManager) {
            QString statusText = QString("Food: %1 | Thorns: %2 | Players: %3")
                               .arg(m_gameManager->getFoodCount())
                               .arg(m_gameManager->getThornsCount())
                               .arg(m_gameManager->getPlayerCount());
            m_statusLabel->setText(statusText);
        }
    }
    
private:
    void setupUI() {
        setWindowTitle("Multi-Player AI Test - GoBigger");
        setMinimumSize(1200, 800);
        
        // 主布局
        auto mainLayout = new QHBoxLayout(this);
        
        // 游戏视图
        m_gameView = new QGraphicsView();
        m_gameView->setMinimumSize(800, 600);
        m_gameView->setDragMode(QGraphicsView::ScrollHandDrag);
        mainLayout->addWidget(m_gameView);
        
        // 控制面板
        auto controlPanel = new QWidget();
        controlPanel->setMaximumWidth(350);
        controlPanel->setMinimumWidth(300);
        auto controlLayout = new QVBoxLayout(controlPanel);
        
        // 游戏配置
        controlLayout->addWidget(new QLabel("<b>Game Configuration:</b>"));
        
        auto aiCountLayout = new QHBoxLayout();
        aiCountLayout->addWidget(new QLabel("AI Players:"));
        m_aiCountSpinBox = new QSpinBox();
        m_aiCountSpinBox->setRange(2, 8);
        m_aiCountSpinBox->setValue(4);
        aiCountLayout->addWidget(m_aiCountSpinBox);
        controlLayout->addLayout(aiCountLayout);
        
        // AI速度控制
        auto speedLayout = new QVBoxLayout();
        m_speedLabel = new QLabel("AI Speed: 200ms");
        speedLayout->addWidget(m_speedLabel);
        m_speedSlider = new QSlider(Qt::Horizontal);
        m_speedSlider->setRange(50, 1000);
        m_speedSlider->setValue(200);
        m_speedSlider->setTickPosition(QSlider::TicksBelow);
        m_speedSlider->setTickInterval(100);
        connect(m_speedSlider, &QSlider::valueChanged, this, &MultiPlayerTestWindow::adjustAISpeed);
        speedLayout->addWidget(m_speedSlider);
        controlLayout->addLayout(speedLayout);
        
        controlLayout->addWidget(new QLabel(""));
        
        // 游戏控制
        controlLayout->addWidget(new QLabel("<b>Game Control:</b>"));
        
        m_startBattleButton = new QPushButton("Start AI Battle");
        connect(m_startBattleButton, &QPushButton::clicked, this, &MultiPlayerTestWindow::startAIBattle);
        controlLayout->addWidget(m_startBattleButton);
        
        m_pauseResumeButton = new QPushButton("Pause Game");
        m_pauseResumeButton->setEnabled(false);
        connect(m_pauseResumeButton, &QPushButton::clicked, this, &MultiPlayerTestWindow::pauseResumeGame);
        controlLayout->addWidget(m_pauseResumeButton);
        
        m_stopGameButton = new QPushButton("Stop Game");
        m_stopGameButton->setEnabled(false);
        connect(m_stopGameButton, &QPushButton::clicked, this, &MultiPlayerTestWindow::stopGame);
        controlLayout->addWidget(m_stopGameButton);
        
        controlLayout->addWidget(new QLabel(""));
        
        // 状态显示
        controlLayout->addWidget(new QLabel("<b>Game Status:</b>"));
        
        m_gameStatusLabel = new QLabel("Status: Stopped");
        m_gameStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        controlLayout->addWidget(m_gameStatusLabel);
        
        m_playerCountLabel = new QLabel("Players: 0 (AI: 0)");
        controlLayout->addWidget(m_playerCountLabel);
        
        m_statusLabel = new QLabel("Food: 0 | Thorns: 0 | Players: 0");
        m_statusLabel->setStyleSheet("font-family: monospace; font-size: 10px;");
        controlLayout->addWidget(m_statusLabel);
        
        controlLayout->addStretch();
        
        // 帮助信息
        controlLayout->addWidget(new QLabel("<b>Instructions:</b>"));
        auto helpText = new QLabel("• Green circles = AI players\n"
                                   "• Small dots = Food\n"
                                   "• Dark circles = Thorns\n"
                                   "• Use mouse wheel to zoom\n"
                                   "• Drag to pan the view");
        helpText->setStyleSheet("font-size: 10px;");
        helpText->setWordWrap(true);
        controlLayout->addWidget(helpText);
        
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
                this, &MultiPlayerTestWindow::onPlayerCountChanged);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameStarted,
                this, &MultiPlayerTestWindow::onGameStarted);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameStopped,
                this, &MultiPlayerTestWindow::onGameStopped);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gamePaused,
                this, &MultiPlayerTestWindow::onGamePaused);
        connect(m_multiPlayerManager, &GoBigger::Multiplayer::MultiPlayerManager::gameResumed,
                this, &MultiPlayerTestWindow::onGameResumed);
    }
    
    void updateGameControls() {
        bool isRunning = m_multiPlayerManager->isGameRunning();
        bool isPaused = m_multiPlayerManager->isGamePaused();
        
        m_startBattleButton->setEnabled(!isRunning);
        m_stopGameButton->setEnabled(isRunning);
        m_pauseResumeButton->setEnabled(isRunning);
        
        if (isRunning) {
            m_pauseResumeButton->setText(isPaused ? "Resume Game" : "Pause Game");
        }
    }
    
private:
    QGraphicsView* m_gameView;
    QGraphicsScene* m_gameScene;
    GameManager* m_gameManager;
    GoBigger::Multiplayer::MultiPlayerManager* m_multiPlayerManager;
    QTimer* m_statusTimer;
    
    // UI控件
    QSpinBox* m_aiCountSpinBox;
    QSlider* m_speedSlider;
    QLabel* m_speedLabel;
    QPushButton* m_startBattleButton;
    QPushButton* m_pauseResumeButton;
    QPushButton* m_stopGameButton;
    QLabel* m_gameStatusLabel;
    QLabel* m_playerCountLabel;
    QLabel* m_statusLabel;
};

#include "multiplayer_test.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MultiPlayerTestWindow window;
    window.show();
    
    return app.exec();
}
