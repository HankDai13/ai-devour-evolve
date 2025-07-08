#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QTime>
#include <QDebug>
#include <QMessageBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextCursor>
#include <memory>

#include "GameManager.h"
#include "SimpleAIPlayer.h"
#include "CloneBall.h"

class AICrashDebugger : public QWidget {
    Q_OBJECT

public:
    AICrashDebugger(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupGame();
        connectSignals();
        
        logMessage("=== AI崩溃调试器启动 ===");
    }
    
    ~AICrashDebugger() {
        logMessage("=== 调试器关闭 ===");
        if (m_gameManager) {
            m_gameManager->pauseGame();
        }
    }

private slots:
    void step1_CreateGameManager() {
        logMessage("\n🔵 步骤1: 创建场景和GameManager...");
        
        try {
            if (m_gameManager) {
                logMessage("GameManager已存在，先清理...");
                m_gameManager->pauseGame();
                delete m_gameManager;
                m_gameManager = nullptr;
            }
            
            if (m_scene) {
                delete m_scene;
                m_scene = nullptr;
            }
            
            if (m_view) {
                m_gameLayout->removeWidget(m_view);
                delete m_view;
                m_view = nullptr;
            }
            
            // 先创建场景
            m_scene = new QGraphicsScene(this);
            m_view = new QGraphicsView(m_scene, this);
            m_view->setMinimumSize(400, 300);
            logMessage("✅ 场景和视图创建成功");
            
            // 然后创建GameManager，传入场景
            m_gameManager = new GameManager(m_scene, GameManager::Config(), this);
            logMessage("✅ GameManager创建成功");
            
            // 连接信号
            connect(m_gameManager, &GameManager::playerAdded, this, &AICrashDebugger::onPlayerAdded);
            connect(m_gameManager, &GameManager::gameStarted, this, &AICrashDebugger::onGameStarted);
            
            logMessage("✅ 信号连接完成");
            
            // 添加视图到布局
            m_gameLayout->addWidget(m_view);
            logMessage("✅ 视图添加到界面");
            
            m_step1Btn->setEnabled(false);
            m_step2Btn->setEnabled(true);
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤1失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤1失败: 未知异常");
        }
    }
    
    void step2_CreateScene() {
        logMessage("\n🔵 步骤2: 启动游戏...");
        
        try {
            if (!m_gameManager) {
                logMessage("❌ GameManager不存在");
                return;
            }
            
            m_gameManager->startGame();
            logMessage("✅ 游戏启动成功");
            
            // 等待一会儿让游戏稳定
            QTimer::singleShot(1000, this, [this]() {
                logMessage("✅ 游戏运行稳定，可以添加玩家");
                m_step2Btn->setEnabled(false);
                m_step3Btn->setEnabled(true);
            });
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤2失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤2失败: 未知异常");
        }
    }
    
    void step3_StartGame() {
        logMessage("\n🔵 步骤3: 添加人类玩家（对照组）...");
        
        try {
            if (!m_gameManager) {
                logMessage("❌ GameManager不存在");
                return;
            }
            
            // 添加人类玩家
            CloneBall* humanPlayer = m_gameManager->createPlayer(0, 0);
            if (humanPlayer) {
                logMessage(QString("✅ 人类玩家创建成功，ID: %1, 位置: (%2, %3)")
                          .arg(humanPlayer->ballId())
                          .arg(humanPlayer->pos().x())
                          .arg(humanPlayer->pos().y()));
                
                logMessage(QString("✅ 玩家在场景中: %1").arg(humanPlayer->scene() != nullptr ? "是" : "否"));
                
                m_humanPlayer = humanPlayer;
                
                // 等待一会儿
                QTimer::singleShot(500, this, [this]() {
                    logMessage("✅ 人类玩家运行稳定");
                    m_step3Btn->setEnabled(false);
                    m_step4Btn->setEnabled(true);
                });
                
            } else {
                logMessage("❌ 人类玩家创建失败");
            }
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤3失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤3失败: 未知异常");
        }
    }
    
    void step4_AddHumanPlayer() {
        logMessage("\n🔵 步骤4: 创建AI玩家球体...");
        
        try {
            if (!m_gameManager) {
                logMessage("❌ GameManager不存在");
                return;
            }
            
            // 先创建AI玩家球体
            CloneBall* aiPlayerBall = m_gameManager->createPlayer(1, 1);
            if (aiPlayerBall) {
                logMessage(QString("✅ AI玩家球体创建成功，ID: %1, 位置: (%2, %3)")
                          .arg(aiPlayerBall->ballId())
                          .arg(aiPlayerBall->pos().x())
                          .arg(aiPlayerBall->pos().y()));
                
                logMessage(QString("✅ AI球在场景中: %1").arg(aiPlayerBall->scene() != nullptr ? "是" : "否"));
                
                m_aiPlayerBall = aiPlayerBall;
                
                // 等待一会儿
                QTimer::singleShot(500, this, [this]() {
                    logMessage("✅ AI玩家球体稳定运行");
                    m_step4Btn->setEnabled(false);
                    m_step5Btn->setEnabled(true);
                });
                
            } else {
                logMessage("❌ AI玩家球体创建失败");
            }
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤4失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤4失败: 未知异常");
        }
    }
    
    void step5_CreateAIPlayer() {
        logMessage("\n🔵 步骤5: 创建AI控制器...");
        
        try {
            if (!m_aiPlayerBall) {
                logMessage("❌ AI玩家球体不存在");
                return;
            }
            
            logMessage("准备创建SimpleAIPlayer...");
            logMessage(QString("目标球体ID: %1").arg(m_aiPlayerBall->ballId()));
            logMessage(QString("目标球体在场景中: %1").arg(m_aiPlayerBall->scene() != nullptr ? "是" : "否"));
            logMessage(QString("球体是否被移除: %1").arg(m_aiPlayerBall->isRemoved() ? "是" : "否"));
            logMessage(QString("球体位置: (%1, %2)").arg(m_aiPlayerBall->pos().x()).arg(m_aiPlayerBall->pos().y()));
            logMessage(QString("球体半径: %1").arg(m_aiPlayerBall->radius()));
            
            // 强制确保球体在场景中
            if (!m_aiPlayerBall->scene()) {
                logMessage("⚠️ 球体不在场景中，尝试手动添加...");
                if (m_scene) {
                    m_scene->addItem(m_aiPlayerBall);
                    logMessage(QString("✅ 球体已手动添加到场景，在场景中: %1")
                              .arg(m_aiPlayerBall->scene() != nullptr ? "是" : "否"));
                } else {
                    logMessage("❌ 场景为空，无法添加球体");
                    return;
                }
            }
            
            // 等待一下确保球体稳定
            QTimer::singleShot(200, this, [this]() {
                try {
                    logMessage("开始创建AI控制器...");
                    
                    // 再次验证球体状态
                    if (!m_aiPlayerBall || m_aiPlayerBall->isRemoved() || !m_aiPlayerBall->scene()) {
                        logMessage("❌ 球体状态无效，无法创建AI控制器");
                        return;
                    }
                    
                    // 创建AI控制器
                    m_aiController = new GoBigger::AI::SimpleAIPlayer(m_aiPlayerBall, this);
                    
                    if (m_aiController) {
                        logMessage("✅ AI控制器创建成功");
                        
                        // 连接信号
                        connect(m_aiController, &GoBigger::AI::SimpleAIPlayer::actionExecuted, 
                                this, &AICrashDebugger::onAIAction);
                        connect(m_aiController, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
                                this, &AICrashDebugger::onAIDestroyed);
                        
                        logMessage("✅ AI信号连接完成");
                        
                        // 等待一会儿
                        QTimer::singleShot(500, this, [this]() {
                            logMessage("✅ AI控制器稳定运行");
                            m_step5Btn->setEnabled(false);
                            m_step6Btn->setEnabled(true);
                        });
                        
                    } else {
                        logMessage("❌ AI控制器创建失败");
                    }
                    
                } catch (const std::exception& e) {
                    logMessage(QString("❌ AI控制器创建异常: %1").arg(e.what()));
                } catch (...) {
                    logMessage("❌ AI控制器创建异常: 未知错误");
                }
            });
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤5失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤5失败: 未知异常");
        }
    }
    
    void step6_CreateAIController() {
        logMessage("\n🔵 步骤6: 启动AI控制...");
        
        try {
            if (!m_aiController) {
                logMessage("❌ AI控制器不存在");
                return;
            }
            
            if (!m_aiPlayerBall) {
                logMessage("❌ AI玩家球体不存在");
                return;
            }
            
            // 详细检查AI控制器和球体状态
            logMessage("📋 AI启动前状态检查:");
            logMessage(QString("  - AI控制器: %1").arg(m_aiController ? "存在" : "不存在"));
            logMessage(QString("  - 球体: %1").arg(m_aiPlayerBall ? "存在" : "不存在"));
            logMessage(QString("  - 球体ID: %1").arg(m_aiPlayerBall->ballId()));
            logMessage(QString("  - 球体在场景中: %1").arg(m_aiPlayerBall->scene() != nullptr ? "是" : "否"));
            logMessage(QString("  - 球体被移除: %1").arg(m_aiPlayerBall->isRemoved() ? "是" : "否"));
            logMessage(QString("  - 球体位置: (%1, %2)").arg(m_aiPlayerBall->pos().x()).arg(m_aiPlayerBall->pos().y()));
            logMessage(QString("  - AI已激活: %1").arg(m_aiController->isAIActive() ? "是" : "否"));
            
            // 设置更长的决策间隔，降低崩溃风险
            logMessage("设置AI决策间隔为1000ms (安全模式)...");
            m_aiController->setDecisionInterval(1000);
            
            logMessage("准备启动AI...");
            
            // 使用延迟启动，避免立即执行决策
            QTimer::singleShot(500, this, [this]() {
                try {
                    if (!m_aiController || !m_aiPlayerBall) {
                        logMessage("❌ AI启动延迟检查失败");
                        return;
                    }
                    
                    logMessage("🚀 执行AI启动...");
                    m_aiController->startAI();
                    logMessage("✅ AI启动命令已发送");
                    
                    // 等待一下再开始监控
                    QTimer::singleShot(2000, this, [this]() {
                        if (m_aiController && m_aiController->isAIActive()) {
                            logMessage("✅ AI启动成功，开始监控");
                            
                            // 开始监控
                            m_monitorTimer = new QTimer(this);
                            connect(m_monitorTimer, &QTimer::timeout, this, &AICrashDebugger::monitorAI);
                            m_monitorTimer->start(1000); // 每秒检查一次
                            
                            logMessage("✅ AI监控已启动");
                            
                            m_step6Btn->setEnabled(false);
                            m_step7Btn->setEnabled(true);
                        } else {
                            logMessage("❌ AI启动失败或已停止");
                        }
                    });
                    
                } catch (const std::exception& e) {
                    logMessage(QString("❌ AI启动异常: %1").arg(e.what()));
                } catch (...) {
                    logMessage("❌ AI启动异常: 未知错误");
                }
            });
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤6失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤6失败: 未知异常");
        }
    }
    
    void step7_StartAI() {
        logMessage("\n🔵 步骤7: 扩展监控...");
        
        try {
            if (!m_monitorTimer) {
                logMessage("❌ 监控定时器不存在");
                return;
            }
            
            logMessage("✅ 继续AI监控，观察长期运行状态");
            m_step7Btn->setEnabled(false);
            m_stopBtn->setEnabled(true);
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 步骤7失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 步骤7失败: 未知异常");
        }
    }
    
    void stopAll() {
        logMessage("\n🔴 停止所有操作...");
        
        try {
            if (m_monitorTimer) {
                m_monitorTimer->stop();
                logMessage("✅ 监控已停止");
            }
            
            if (m_aiController) {
                m_aiController->stopAI();
                logMessage("✅ AI已停止");
            }
            
            if (m_gameManager) {
                m_gameManager->pauseGame();
                logMessage("✅ 游戏已停止");
            }
            
            // 重置按钮状态
            resetButtons();
            
        } catch (const std::exception& e) {
            logMessage(QString("❌ 停止操作失败: %1").arg(e.what()));
        } catch (...) {
            logMessage("❌ 停止操作失败: 未知异常");
        }
    }
    
    void onPlayerAdded(CloneBall* player) {
        logMessage(QString("🎯 玩家添加事件: ID=%1, 团队=%2, 玩家=%3")
                  .arg(player->ballId())
                  .arg(player->teamId())
                  .arg(player->playerId()));
    }
    
    void onGameStarted() {
        logMessage("🎯 游戏启动事件触发");
    }
    
    void onAIAction(const GoBigger::AI::AIAction& action) {
        m_aiActionCount++;
        if (m_aiActionCount % 10 == 0) { // 每10个动作记录一次
            logMessage(QString("🤖 AI动作 #%1: dx=%2, dy=%3, type=%4")
                      .arg(m_aiActionCount)
                      .arg(action.dx, 0, 'f', 2)
                      .arg(action.dy, 0, 'f', 2)
                      .arg(static_cast<int>(action.type)));
        }
    }
    
    void onAIDestroyed(GoBigger::AI::SimpleAIPlayer* ai) {
        logMessage("💀 AI被销毁事件触发");
    }
    
    void monitorAI() {
        try {
            if (!m_aiController || !m_aiPlayerBall) {
                logMessage("⚠️ 监控: AI控制器或球体已失效");
                if (m_monitorTimer) {
                    m_monitorTimer->stop();
                    logMessage("🛑 监控已自动停止");
                }
                return;
            }
            
            // 详细状态检查
            bool aiActive = false;
            bool ballRemoved = true;
            bool ballInScene = false;
            
            // 安全地检查AI状态
            try {
                aiActive = m_aiController->isAIActive();
            } catch (...) {
                logMessage("❌ 监控: 无法获取AI活跃状态");
                return;
            }
            
            // 安全地检查球体状态
            try {
                ballRemoved = m_aiPlayerBall->isRemoved();
                ballInScene = (m_aiPlayerBall->scene() != nullptr);
            } catch (...) {
                logMessage("❌ 监控: 无法获取球体状态");
                return;
            }
            
            logMessage(QString("📊 监控: AI活跃=%1, 球体移除=%2, 在场景=%3, 动作计数=%4")
                      .arg(aiActive ? "是" : "否")
                      .arg(ballRemoved ? "是" : "否")
                      .arg(ballInScene ? "是" : "否")
                      .arg(m_aiActionCount));
            
            // 如果球体被移除或不在场景中，停止AI
            if (ballRemoved || !ballInScene) {
                logMessage("⚠️ 球体状态异常，停止AI");
                if (m_aiController) {
                    m_aiController->stopAI();
                }
                if (m_monitorTimer) {
                    m_monitorTimer->stop();
                }
            }
                      
        } catch (const std::exception& e) {
            logMessage(QString("❌ 监控异常: %1").arg(e.what()));
            // 异常时停止监控
            if (m_monitorTimer) {
                m_monitorTimer->stop();
            }
        } catch (...) {
            logMessage("❌ 监控异常: 未知错误");
            // 异常时停止监控
            if (m_monitorTimer) {
                m_monitorTimer->stop();
            }
        }
    }

private:
    void setupUI() {
        setWindowTitle("AI崩溃调试器");
        setMinimumSize(800, 600);
        
        QHBoxLayout* mainLayout = new QHBoxLayout(this);
        
        // 左侧控制面板
        QWidget* controlPanel = new QWidget;
        controlPanel->setMaximumWidth(300);
        QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
        
        // 步骤按钮
        m_step1Btn = new QPushButton("步骤1: 创建场景和GameManager");
        m_step2Btn = new QPushButton("步骤2: 启动游戏");
        m_step3Btn = new QPushButton("步骤3: 添加人类玩家");
        m_step4Btn = new QPushButton("步骤4: 创建AI球体");
        m_step5Btn = new QPushButton("步骤5: 创建AI控制器");
        m_step6Btn = new QPushButton("步骤6: 启动AI");
        m_step7Btn = new QPushButton("步骤7: 扩展监控");
        m_stopBtn = new QPushButton("停止所有");
        
        controlLayout->addWidget(m_step1Btn);
        controlLayout->addWidget(m_step2Btn);
        controlLayout->addWidget(m_step3Btn);
        controlLayout->addWidget(m_step4Btn);
        controlLayout->addWidget(m_step5Btn);
        controlLayout->addWidget(m_step6Btn);
        controlLayout->addWidget(m_step7Btn);
        controlLayout->addWidget(m_stopBtn);
        controlLayout->addStretch();
        
        // 初始状态
        resetButtons();
        
        // 右侧游戏和日志区域
        QWidget* gameArea = new QWidget;
        QVBoxLayout* gameLayout = new QVBoxLayout(gameArea);
        
        m_gameLayout = gameLayout; // 保存引用用于添加游戏视图
        
        // 日志区域
        m_logArea = new QTextEdit;
        m_logArea->setMaximumHeight(200);
        m_logArea->setReadOnly(true);
        gameLayout->addWidget(m_logArea);
        
        mainLayout->addWidget(controlPanel);
        mainLayout->addWidget(gameArea, 1);
    }
    
    void setupGame() {
        m_gameManager = nullptr;
        m_scene = nullptr;
        m_view = nullptr;
        m_humanPlayer = nullptr;
        m_aiPlayerBall = nullptr;
        m_aiController = nullptr;
        m_monitorTimer = nullptr;
        m_aiActionCount = 0;
    }
    
    void connectSignals() {
        connect(m_step1Btn, &QPushButton::clicked, this, &AICrashDebugger::step1_CreateGameManager);
        connect(m_step2Btn, &QPushButton::clicked, this, &AICrashDebugger::step2_CreateScene);
        connect(m_step3Btn, &QPushButton::clicked, this, &AICrashDebugger::step3_StartGame);
        connect(m_step4Btn, &QPushButton::clicked, this, &AICrashDebugger::step4_AddHumanPlayer);
        connect(m_step5Btn, &QPushButton::clicked, this, &AICrashDebugger::step5_CreateAIPlayer);
        connect(m_step6Btn, &QPushButton::clicked, this, &AICrashDebugger::step6_CreateAIController);
        connect(m_step7Btn, &QPushButton::clicked, this, &AICrashDebugger::step7_StartAI);
        connect(m_stopBtn, &QPushButton::clicked, this, &AICrashDebugger::stopAll);
    }
    
    void resetButtons() {
        m_step1Btn->setEnabled(true);
        m_step2Btn->setEnabled(false);
        m_step3Btn->setEnabled(false);
        m_step4Btn->setEnabled(false);
        m_step5Btn->setEnabled(false);
        m_step6Btn->setEnabled(false);
        m_step7Btn->setEnabled(false);
        m_stopBtn->setEnabled(false);
    }
    
    void logMessage(const QString& message) {
        QString timestamp = QTime::currentTime().toString("hh:mm:ss.zzz");
        QString logEntry = QString("[%1] %2").arg(timestamp, message);
        m_logArea->append(logEntry);
        qDebug() << logEntry;
        
        // 自动滚动到底部
        QTextCursor cursor = m_logArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logArea->setTextCursor(cursor);
    }

private:
    // UI组件
    QPushButton* m_step1Btn;
    QPushButton* m_step2Btn;
    QPushButton* m_step3Btn;
    QPushButton* m_step4Btn;
    QPushButton* m_step5Btn;
    QPushButton* m_step6Btn;
    QPushButton* m_step7Btn;
    QPushButton* m_stopBtn;
    QTextEdit* m_logArea;
    QVBoxLayout* m_gameLayout;
    
    // 游戏组件
    GameManager* m_gameManager;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    CloneBall* m_humanPlayer;
    CloneBall* m_aiPlayerBall;
    GoBigger::AI::SimpleAIPlayer* m_aiController;
    QTimer* m_monitorTimer;
    
    // 统计
    int m_aiActionCount;
};

#include "ai_crash_debug.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    AICrashDebugger debugger;
    debugger.show();
    
    return app.exec();
}
