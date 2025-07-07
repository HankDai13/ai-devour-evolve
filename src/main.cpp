#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>
#include "GameView.h"
#include "GameManager.h"
#include "CloneBall.h"
#include "GameStartScreen.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent), m_gameView(nullptr) {
        setupUI();
    }
    
private slots:
    void onStartGameRequested(const GameConfig& config) {
        // 隐藏启动界面
        if (m_startScreen) {
            m_startScreen->hide();
        }
        
        // 创建或重置游戏视图
        if (!m_gameView) {
            m_gameView = new GameView(this);
            setCentralWidget(m_gameView);
            setupGameMenu();
            setupStatusBar();
        }
        
        // 应用游戏配置
        applyGameConfig(config);
        
        // 显示主窗口
        show();
        raise();
        activateWindow();
        
        // 启动游戏
        QTimer::singleShot(500, m_gameView, &GameView::startGame);
        
        qDebug() << "Game started with mode:" << static_cast<int>(config.mode);
    }
    
    void onExitRequested() {
        QApplication::quit();
    }
    
    void onReturnToStartScreen() {
        // 暂停游戏
        if (m_gameView) {
            m_gameView->pauseGame();
        }
        
        // 隐藏主窗口
        hide();
        
        // 显示启动界面
        if (m_startScreen) {
            m_startScreen->show();
            m_startScreen->raise();
            m_startScreen->activateWindow();
        }
    }
    
private:
    void setupUI() {
        // 创建启动界面
        m_startScreen = new GameStartScreen();
        
        // 连接信号
        connect(m_startScreen, &GameStartScreen::startGameRequested,
                this, &MainWindow::onStartGameRequested);
        connect(m_startScreen, &GameStartScreen::exitRequested,
                this, &MainWindow::onExitRequested);
        
        // 设置主窗口属性
        setWindowTitle("智能吞噬进化 - AI Devour Evolve");
        resize(1200, 800);
        
        // 显示启动界面
        m_startScreen->show();
    }
    
    void setupGameMenu() {
        if (!m_gameView) return;
        
        // 清除现有菜单
        menuBar()->clear();
        
        // 创建菜单栏
        QMenu *gameMenu = menuBar()->addMenu("游戏");
        
        // 添加游戏控制菜单项
        QAction *startAction = gameMenu->addAction("开始游戏 (P)");
        QAction *pauseAction = gameMenu->addAction("暂停游戏 (P)");
        QAction *resetAction = gameMenu->addAction("重置游戏 (Esc)");
        gameMenu->addSeparator();
        QAction *returnToStartAction = gameMenu->addAction("返回开始界面");
        QAction *exitAction = gameMenu->addAction("退出");
        
        // 创建AI菜单
        QMenu *aiMenu = menuBar()->addMenu("AI");
        QAction *addAIAction = aiMenu->addAction("快速添加AI玩家 (食物猎手)");
        QAction *addRLAIAction = aiMenu->addAction("快速添加RL-AI玩家");
        QAction *addCustomAIAction = aiMenu->addAction("自定义添加AI玩家...");
        aiMenu->addSeparator();
        QAction *startAllAIAction = aiMenu->addAction("启动所有AI");
        QAction *stopAllAIAction = aiMenu->addAction("停止所有AI");
        QAction *removeAllAIAction = aiMenu->addAction("移除所有AI");
        aiMenu->addSeparator();
        QAction *showDebugAction = aiMenu->addAction("显示AI调试控制台");
        showDebugAction->setShortcut(QKeySequence("F12"));
        
        // 连接游戏菜单动作
        connect(startAction, &QAction::triggered, m_gameView, &GameView::startGame);
        connect(pauseAction, &QAction::triggered, m_gameView, &GameView::pauseGame);
        connect(resetAction, &QAction::triggered, m_gameView, &GameView::resetGame);
        connect(returnToStartAction, &QAction::triggered, this, &MainWindow::onReturnToStartScreen);
        connect(exitAction, &QAction::triggered, &QApplication::quit);
        
        // 连接AI菜单动作
        connect(addAIAction, &QAction::triggered, m_gameView, &GameView::addAIPlayer);
        connect(addRLAIAction, &QAction::triggered, m_gameView, &GameView::addRLAIPlayer);
        connect(addCustomAIAction, &QAction::triggered, m_gameView, &GameView::addAIPlayerWithDialog);
        connect(startAllAIAction, &QAction::triggered, m_gameView, &GameView::startAllAI);
        connect(stopAllAIAction, &QAction::triggered, m_gameView, &GameView::stopAllAI);
        connect(removeAllAIAction, &QAction::triggered, m_gameView, &GameView::removeAllAI);
        connect(showDebugAction, &QAction::triggered, m_gameView, &GameView::showAIDebugConsole);
        
        // 创建帮助菜单
        QMenu *helpMenu = menuBar()->addMenu("帮助");
        QAction *controlsAction = helpMenu->addAction("控制说明");
        
        connect(controlsAction, &QAction::triggered, [this]() {
            QMessageBox::information(this, "控制说明", 
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
    }
    
    void setupStatusBar() {
        if (!m_gameView) return;
        
        // 创建状态栏
        QStatusBar *statusBar = this->statusBar();
        if (m_statusLabel) {
            statusBar->removeWidget(m_statusLabel);
            delete m_statusLabel;
        }
        
        m_statusLabel = new QLabel();
        m_statusLabel->setText("按P键开始游戏");
        statusBar->addWidget(m_statusLabel);
        
        // 定时更新状态栏
        if (m_statusTimer) {
            m_statusTimer->stop();
            delete m_statusTimer;
        }
        
        m_statusTimer = new QTimer(this);
        connect(m_statusTimer, &QTimer::timeout, [this]() {
            if (!m_gameView) return;
            
            if (m_gameView->isGameRunning()) {
                CloneBall* player = m_gameView->getMainPlayer();
                if (player && !player->isRemoved()) {
                    float totalScore = m_gameView->getTotalPlayerScore();
                    m_statusLabel->setText(QString("游戏进行中 - 总分数: %1 | 主球大小: %2")
                        .arg(QString::number(totalScore, 'f', 1))
                        .arg(QString::number(player->radius(), 'f', 1)));
                } else {
                    m_statusLabel->setText("游戏进行中 - 玩家已被淘汰");
                }
            } else {
                m_statusLabel->setText("游戏已暂停 - 按P键继续");
            }
        });
        m_statusTimer->start(100);
    }
    
    void applyGameConfig(const GameConfig& config) {
        if (!m_gameView) return;
        
        // 重置游戏
        m_gameView->resetGame();
        
        // 应用世界配置
        GameManager* gameManager = m_gameView->getGameManager();
        if (gameManager) {
            // 这里可以设置世界大小、食物密度、荆棘密度等参数
            // 注意：需要GameManager支持这些配置方法
            qDebug() << "Applying world config - Size:" << config.worldSize 
                     << "Food density:" << config.foodDensity 
                     << "Thorn density:" << config.thornDensity;
        }
        
        // 根据配置模式设置游戏
        switch (config.mode) {
        case GameMode::DEBUG_SINGLE_PLAYER:
            // 调试模式：添加少量AI作为测试对象
            addDebugModeAI(config);
            qDebug() << "Applied DEBUG_SINGLE_PLAYER configuration";
            break;
            
        case GameMode::SURVIVAL_BATTLE:
            // 生存模式：添加多支AI队伍
            addSurvivalAITeams(config);
            qDebug() << "Applied SURVIVAL_BATTLE configuration with" << config.totalTeams << "teams";
            break;
            
        case GameMode::BOSS_CHALLENGE:
            // BOSS模式：添加BOSS和挑战者
            addBossChallenge(config);
            qDebug() << "Applied BOSS_CHALLENGE configuration";
            break;
        }
        
        // 应用时间限制
        if (config.gameTimeLimit > 0) {
            QTimer::singleShot(config.gameTimeLimit * 1000, [this, config]() {
                if (m_gameView && m_gameView->isGameRunning()) {
                    m_gameView->pauseGame();
                    QMessageBox::information(this, "游戏结束", 
                        QString("时间到！游戏结束。\n时间限制：%1秒").arg(config.gameTimeLimit));
                }
            });
            qDebug() << "Set game time limit to" << config.gameTimeLimit << "seconds";
        }
    }
    
    void addSurvivalAITeams(const GameConfig& config) {
        if (!m_gameView || !m_gameView->getGameManager()) {
            qWarning() << "GameView or GameManager not available";
            return;
        }
        
        GameManager* gameManager = m_gameView->getGameManager();
        
        // 为每支队伍添加AI
        for (auto it = config.teamAIConfigs.begin(); it != config.teamAIConfigs.end(); ++it) {
            int teamId = it.key();
            const AIConfig& aiConfig = it.value();
            int playerIdCounter = 0;
            
            qDebug() << "Setting up team" << teamId << "with AI config:" << aiConfig.name;
            
            // 添加食物猎人AI
            for (int i = 0; i < aiConfig.foodHunterCount; ++i) {
                bool success = gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::FOOD_HUNTER
                );
                if (success) {
                    qDebug() << "Added FOOD_HUNTER AI to team" << teamId;
                } else {
                    qWarning() << "Failed to add FOOD_HUNTER AI to team" << teamId;
                }
            }
            
            // 添加攻击性AI
            for (int i = 0; i < aiConfig.aggressiveCount; ++i) {
                bool success = gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::AGGRESSIVE
                );
                if (success) {
                    qDebug() << "Added AGGRESSIVE AI to team" << teamId;
                } else {
                    qWarning() << "Failed to add AGGRESSIVE AI to team" << teamId;
                }
            }
            
            // 添加模型AI
            for (int i = 0; i < aiConfig.modelBasedCount; ++i) {
                bool success = gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::MODEL_BASED
                );
                if (success) {
                    qDebug() << "Added MODEL_BASED AI to team" << teamId;
                } else {
                    qWarning() << "Failed to add MODEL_BASED AI to team" << teamId;
                }
            }
            
            // 添加随机AI
            for (int i = 0; i < aiConfig.randomCount; ++i) {
                bool success = gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::RANDOM
                );
                if (success) {
                    qDebug() << "Added RANDOM AI to team" << teamId;
                } else {
                    qWarning() << "Failed to add RANDOM AI to team" << teamId;
                }
            }
        }
        
        // 启动所有AI
        QTimer::singleShot(1000, gameManager, &GameManager::startAllAI);
    }
    
    void addBossChallenge(const GameConfig& config) {
        if (!m_gameView || !m_gameView->getGameManager()) {
            qWarning() << "GameView or GameManager not available for BOSS challenge";
            return;
        }
        
        GameManager* gameManager = m_gameView->getGameManager();
        qDebug() << "Setting up BOSS challenge mode";
        
        // 设置BOSS队伍
        int bossTeamId = config.bossTeamId;
        const AIConfig& bossConfig = config.teamAIConfigs[bossTeamId];
        
        qDebug() << "Setting up BOSS team" << bossTeamId << "with enhanced AI";
        
        // BOSS队伍使用更强的AI配置
        int bossPlayerIdCounter = 0;
        
        // 添加攻击性BOSS AI
        for (int i = 0; i < bossConfig.aggressiveCount; ++i) {
            bool success = gameManager->addAIPlayerWithStrategy(
                bossTeamId, bossPlayerIdCounter++, 
                GoBigger::AI::AIStrategy::AGGRESSIVE
            );
            if (success) {
                qDebug() << "Added AGGRESSIVE BOSS AI to team" << bossTeamId;
                
                // 为BOSS设置更高的初始分数
                CloneBall* bossPlayer = gameManager->getPlayer(bossTeamId, bossPlayerIdCounter - 1);
                if (bossPlayer) {
                    bossPlayer->setScore(config.bossInitialScore);
                    qDebug() << "Set BOSS initial score to" << config.bossInitialScore;
                }
            }
        }
        
        // 添加模型BOSS AI
        for (int i = 0; i < bossConfig.modelBasedCount; ++i) {
            bool success = gameManager->addAIPlayerWithStrategy(
                bossTeamId, bossPlayerIdCounter++, 
                GoBigger::AI::AIStrategy::MODEL_BASED
            );
            if (success) {
                qDebug() << "Added MODEL_BASED BOSS AI to team" << bossTeamId;
                
                // 为BOSS设置更高的初始分数
                CloneBall* bossPlayer = gameManager->getPlayer(bossTeamId, bossPlayerIdCounter - 1);
                if (bossPlayer) {
                    bossPlayer->setScore(config.bossInitialScore);
                    qDebug() << "Set BOSS initial score to" << config.bossInitialScore;
                }
            }
        }
        
        // 设置挑战者队伍
        for (auto it = config.teamAIConfigs.begin(); it != config.teamAIConfigs.end(); ++it) {
            int teamId = it.key();
            if (teamId == bossTeamId) continue; // 跳过BOSS队伍
            
            const AIConfig& aiConfig = it.value();
            int playerIdCounter = 0;
            
            qDebug() << "Setting up challenger team" << teamId;
            
            // 添加挑战者AI
            for (int i = 0; i < aiConfig.foodHunterCount; ++i) {
                gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::FOOD_HUNTER
                );
            }
            
            for (int i = 0; i < aiConfig.aggressiveCount; ++i) {
                gameManager->addAIPlayerWithStrategy(
                    teamId, playerIdCounter++, 
                    GoBigger::AI::AIStrategy::AGGRESSIVE
                );
            }
        }
        
        // 启动所有AI
        QTimer::singleShot(1000, gameManager, &GameManager::startAllAI);
        
        // 如果启用了BOSS特殊能力，可以在这里设置额外的游戏规则
        if (config.enableBossSpecialAbilities) {
            qDebug() << "BOSS special abilities enabled";
            // 这里可以添加特殊能力的实现，比如：
            // - BOSS移动速度加成
            // - BOSS分裂能力增强
            // - BOSS免疫荆棘伤害等
        }
    }
    
    void addDebugModeAI(const GameConfig& config) {
        if (!m_gameView || !m_gameView->getGameManager()) {
            qWarning() << "GameView or GameManager not available for debug mode";
            return;
        }
        
        GameManager* gameManager = m_gameView->getGameManager();
        qDebug() << "Setting up debug mode with test AI";
        
        // 调试模式：添加少量不同策略的AI作为测试对象
        // 队伍1：食物猎手AI
        gameManager->addAIPlayerWithStrategy(1, 0, GoBigger::AI::AIStrategy::FOOD_HUNTER);
        gameManager->addAIPlayerWithStrategy(1, 1, GoBigger::AI::AIStrategy::FOOD_HUNTER);
        
        // 队伍2：攻击性AI
        gameManager->addAIPlayerWithStrategy(2, 0, GoBigger::AI::AIStrategy::AGGRESSIVE);
        
        // 队伍3：随机AI
        gameManager->addAIPlayerWithStrategy(3, 0, GoBigger::AI::AIStrategy::RANDOM);
        
        // 如果有模型可用，添加一个模型AI
        gameManager->addAIPlayerWithStrategy(4, 0, GoBigger::AI::AIStrategy::MODEL_BASED);
        
        qDebug() << "Added test AI players for debug mode";
        
        // 稍后启动AI
        QTimer::singleShot(1000, gameManager, &GameManager::startAllAI);
    }
    
private:
    GameStartScreen* m_startScreen = nullptr;
    GameView* m_gameView = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTimer* m_statusTimer = nullptr;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("AI Devour Evolve");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("AI Development Team");
    
    // 创建并显示主窗口（它会自动显示启动界面）
    MainWindow mainWindow;
    
    return app.exec();
}