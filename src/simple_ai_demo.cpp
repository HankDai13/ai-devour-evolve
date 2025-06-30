#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <memory>

#include "SimpleAIPlayer.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "ThornsBall.h"
#include "BaseBall.h"
#include "GameManager.h"

using namespace GoBigger::AI;

class SimpleAIDemo : public QWidget {
    Q_OBJECT

public:
    SimpleAIDemo(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupGame();
        
        // 设置游戏循环定时器
        m_gameUpdateTimer = new QTimer(this);
        connect(m_gameUpdateTimer, &QTimer::timeout, this, &SimpleAIDemo::updateGame);
        m_gameUpdateTimer->start(16); // 约60 FPS
        
        // 连接AI信号
        if (m_aiPlayer) {
            connect(m_aiPlayer.get(), &SimpleAIPlayer::actionExecuted, 
                    this, &SimpleAIDemo::onAIActionExecuted);
            connect(m_aiPlayer.get(), &SimpleAIPlayer::strategyChanged,
                    this, &SimpleAIDemo::onAIStrategyChanged);
        }
    }
    
    ~SimpleAIDemo() = default;

private slots:
    void startAI() {
        if (m_aiPlayer) {
            m_aiPlayer->startAI();
            m_startButton->setEnabled(false);
            m_stopButton->setEnabled(true);
            m_statusLabel->setText("AI Status: Running");
        }
    }
    
    void stopAI() {
        if (m_aiPlayer) {
            m_aiPlayer->stopAI();
            m_startButton->setEnabled(true);
            m_stopButton->setEnabled(false);
            m_statusLabel->setText("AI Status: Stopped");
        }
    }
    
    void changeStrategy() {
        if (m_aiPlayer) {
            int index = m_strategyCombo->currentIndex();
            SimpleAIPlayer::AIStrategy strategy = static_cast<SimpleAIPlayer::AIStrategy>(index);
            m_aiPlayer->setAIStrategy(strategy);
            qDebug() << "Changed AI strategy to:" << index;
        }
    }
    
    void changeInterval() {
        if (m_aiPlayer) {
            int interval = m_intervalSpinBox->value();
            m_aiPlayer->setDecisionInterval(interval);
            qDebug() << "Changed AI decision interval to:" << interval << "ms";
        }
    }
    
    void loadModel() {
        QString modelPath = QFileDialog::getOpenFileName(
            this, 
            "Load ONNX Model", 
            QDir::currentPath() + "/assets/ai_models/", 
            "ONNX Files (*.onnx);;All Files (*)"
        );
        
        if (!modelPath.isEmpty()) {
            if (m_aiPlayer && m_aiPlayer->loadAIModel(modelPath)) {
                QMessageBox::information(this, "Success", "AI model loaded successfully!");
                m_modelLabel->setText("Model: " + QFileInfo(modelPath).fileName());
                
                // 切换到模型策略
                m_strategyCombo->setCurrentIndex(3); // MODEL_BASED
                changeStrategy();
            } else {
                QMessageBox::warning(this, "Error", "Failed to load AI model!");
            }
        }
    }
    
    void loadDemoModel() {
        QString demoModelPath = QDir::currentPath() + "/assets/ai_models/simple_gobigger_demo.onnx";
        
        if (QFile::exists(demoModelPath)) {
            if (m_aiPlayer && m_aiPlayer->loadAIModel(demoModelPath)) {
                QMessageBox::information(this, "Success", "Demo AI model loaded successfully!");
                m_modelLabel->setText("Model: simple_gobigger_demo.onnx");
                
                // 切换到模型策略
                m_strategyCombo->setCurrentIndex(3); // MODEL_BASED
                changeStrategy();
            } else {
                QMessageBox::warning(this, "Error", "Failed to load demo AI model!");
            }
        } else {
            QMessageBox::warning(this, "Error", 
                               "Demo model not found at: " + demoModelPath + 
                               "\nPlease run scripts/create_demo_model.py first!");
        }
    }
    
    void loadRLModel() {
        QString rlModelPath = QDir::currentPath() + "/assets/ai_models/exported_models/ai_model.onnx";
        
        if (QFile::exists(rlModelPath)) {
            if (m_aiPlayer && m_aiPlayer->loadAIModel(rlModelPath)) {
                QMessageBox::information(this, "Success", 
                                       QString("RL AI model loaded successfully!\n") +
                                       QString("This model was trained using reinforcement learning and should show intelligent behavior."));
                m_modelLabel->setText("Model: ai_model.onnx (RL-trained)");
                
                // 切换到模型策略
                m_strategyCombo->setCurrentIndex(3); // MODEL_BASED
                changeStrategy();
            } else {
                QMessageBox::warning(this, "Error", 
                                   QString("Failed to load RL AI model!\n") +
                                   QString("Please check the console output for detailed error information."));
            }
        } else {
            QMessageBox::warning(this, "Error", 
                               QString("RL model not found at: ") + rlModelPath + 
                               QString("\nPlease make sure the RL model has been exported to the expected location."));
        }
    }
    
    void onAIActionExecuted(const AIAction& action) {
        m_actionLabel->setText(QString("Last Action: dx=%.2f, dy=%.2f, type=%1")
                              .arg(action.dx)
                              .arg(action.dy)
                              .arg(static_cast<int>(action.type)));
    }
    
    void onAIStrategyChanged(SimpleAIPlayer::AIStrategy strategy) {
        m_strategyCombo->setCurrentIndex(static_cast<int>(strategy));
    }
    
    void updateGame() {
        // 更新所有球的物理状态
        auto items = m_scene->items();
        QList<BaseBall*> ballsToRemove;
        
        for (auto item : items) {
            BaseBall* ball = dynamic_cast<BaseBall*>(item);
            if (ball && !ball->isRemoved()) {
                ball->updatePhysics(0.016f); // 16ms ≈ 60 FPS
                
                // 检查碰撞
                checkCollisions(ball, ballsToRemove);
            }
        }
        
        // 移除被吃掉的球
        for (BaseBall* ball : ballsToRemove) {
            m_scene->removeItem(ball);
            ball->deleteLater();
        }
        
        // 随机补充食物
        static int foodCounter = 0;
        if (++foodCounter % 60 == 0) { // 每秒补充一次
            generateSingleFood();
        }
    }
    
    void checkCollisions(BaseBall* ball, QList<BaseBall*>& ballsToRemove) {
        if (!ball || ball->isRemoved()) return;
        
        auto items = m_scene->items();
        for (auto item : items) {
            BaseBall* other = dynamic_cast<BaseBall*>(item);
            if (other && other != ball && !other->isRemoved()) {
                if (ball->collidesWith(other)) {
                    if (ball->canEat(other)) {
                        qDebug() << "Collision detected: Ball" << ball->ballId() 
                                << "eating Ball" << other->ballId();
                        ball->eat(other);
                        if (!ballsToRemove.contains(other)) {
                            ballsToRemove.append(other);
                        }
                    } else if (other->canEat(ball)) {
                        qDebug() << "Collision detected: Ball" << other->ballId() 
                                << "eating Ball" << ball->ballId();
                        other->eat(ball);
                        if (!ballsToRemove.contains(ball)) {
                            ballsToRemove.append(ball);
                        }
                        break; // 当前球被吃掉，退出循环
                    }
                }
            }
        }
    }

private:
    void setupUI() {
        setWindowTitle("Simple AI Demo - GoBigger");
        setMinimumSize(1000, 700);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        
        // 控制面板
        QWidget* controlPanel = new QWidget;
        QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
        
        // AI控制按钮
        m_startButton = new QPushButton("Start AI");
        m_stopButton = new QPushButton("Stop AI");
        m_stopButton->setEnabled(false);
        
        connect(m_startButton, &QPushButton::clicked, this, &SimpleAIDemo::startAI);
        connect(m_stopButton, &QPushButton::clicked, this, &SimpleAIDemo::stopAI);
        
        // 策略选择
        QLabel* strategyLabel = new QLabel("Strategy:");
        m_strategyCombo = new QComboBox;
        m_strategyCombo->addItems({"Random", "Food Hunter", "Aggressive", "Model Based"});
        m_strategyCombo->setCurrentIndex(1); // 默认Food Hunter
        
        connect(m_strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SimpleAIDemo::changeStrategy);
        
        // 决策间隔
        QLabel* intervalLabel = new QLabel("Interval (ms):");
        m_intervalSpinBox = new QSpinBox;
        m_intervalSpinBox->setRange(50, 2000);
        m_intervalSpinBox->setValue(200);
        
        connect(m_intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &SimpleAIDemo::changeInterval);
        
        // 模型加载
        QPushButton* loadModelButton = new QPushButton("Load Model");
        connect(loadModelButton, &QPushButton::clicked, this, &SimpleAIDemo::loadModel);
        
        QPushButton* loadDemoModelButton = new QPushButton("Load Demo Model");
        connect(loadDemoModelButton, &QPushButton::clicked, this, &SimpleAIDemo::loadDemoModel);
        
        QPushButton* loadRLModelButton = new QPushButton("Load RL Model");
        connect(loadRLModelButton, &QPushButton::clicked, this, &SimpleAIDemo::loadRLModel);
        
        controlLayout->addWidget(m_startButton);
        controlLayout->addWidget(m_stopButton);
        controlLayout->addWidget(strategyLabel);
        controlLayout->addWidget(m_strategyCombo);
        controlLayout->addWidget(intervalLabel);
        controlLayout->addWidget(m_intervalSpinBox);
        controlLayout->addWidget(loadModelButton);
        controlLayout->addWidget(loadDemoModelButton);
        controlLayout->addWidget(loadRLModelButton);
        controlLayout->addStretch();
        
        // 状态面板
        QWidget* statusPanel = new QWidget;
        QVBoxLayout* statusLayout = new QVBoxLayout(statusPanel);
        
        m_statusLabel = new QLabel("AI Status: Stopped");
        m_actionLabel = new QLabel("Last Action: None");
        m_modelLabel = new QLabel("Model: None loaded");
        
        statusLayout->addWidget(m_statusLabel);
        statusLayout->addWidget(m_actionLabel);
        statusLayout->addWidget(m_modelLabel);
        
        // 游戏视图
        m_graphicsView = new QGraphicsView;
        m_graphicsView->setRenderHint(QPainter::Antialiasing);
        m_graphicsView->setMinimumSize(800, 500);
        
        mainLayout->addWidget(controlPanel);
        mainLayout->addWidget(statusPanel);
        mainLayout->addWidget(m_graphicsView);
    }
    
    void setupGame() {
        // 创建游戏场景
        m_scene = new QGraphicsScene(0, 0, 800, 600, this);
        m_graphicsView->setScene(m_scene);
        
        // 定义游戏边界
        Border gameBorder(0, 800, 0, 600);
        
        // 创建AI玩家球
        m_playerBall = new CloneBall(1, QPointF(400, 300), gameBorder, 1, 1);
        m_scene->addItem(m_playerBall);
        
        // 创建AI控制器
        m_aiPlayer = std::make_unique<SimpleAIPlayer>(m_playerBall, this);
        
        // 添加一些食物
        generateFood();
        
        // 添加一些其他玩家（NPC）
        generateNPCs();
        
        qDebug() << "Game setup completed";
    }
    
    void generateFood() {
        // 定义游戏边界
        Border gameBorder(0, 800, 0, 600);
        
        // 随机生成食物
        for (int i = 0; i < 50; ++i) {
            FoodBall* food = new FoodBall(i + 1000, QPointF(rand() % 800, rand() % 600), gameBorder);
            m_scene->addItem(food);
        }
    }
    
    void generateNPCs() {
        // 定义游戏边界
        Border gameBorder(0, 800, 0, 600);
        
        // 生成一些NPC玩家
        for (int i = 0; i < 3; ++i) { // 减少NPC数量，避免过于拥挤
            CloneBall* npc = new CloneBall(i + 100, QPointF(rand() % 800, rand() % 600), gameBorder, i + 2, i + 100);
            m_scene->addItem(npc);
        }
        
        // 添加荆棘球
        generateThorns();
    }
    
    void generateThorns() {
        // 定义游戏边界
        Border gameBorder(0, 800, 0, 600);
        
        // 生成一些荆棘球
        for (int i = 0; i < 8; ++i) {
            ThornsBall* thorns = new ThornsBall(i + 500, QPointF(rand() % 800, rand() % 600), gameBorder);
            m_scene->addItem(thorns);
        }
    }
    
    void generateSingleFood() {
        // 定义游戏边界
        Border gameBorder(0, 800, 0, 600);
        
        // 随机生成一个食物
        static int foodIdCounter = 2000;
        FoodBall* food = new FoodBall(foodIdCounter++, 
                                    QPointF(rand() % 800, rand() % 600), 
                                    gameBorder);
        m_scene->addItem(food);
    }

private:
    QGraphicsView* m_graphicsView;
    QGraphicsScene* m_scene;
    CloneBall* m_playerBall;
    std::unique_ptr<SimpleAIPlayer> m_aiPlayer;
    QTimer* m_gameUpdateTimer; // 游戏循环定时器
    
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QComboBox* m_strategyCombo;
    QSpinBox* m_intervalSpinBox;
    QLabel* m_statusLabel;
    QLabel* m_actionLabel;
    QLabel* m_modelLabel;
};

#include "simple_ai_demo.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    SimpleAIDemo demo;
    demo.show();
    
    qDebug() << "Starting Simple AI Demo";
    
    return app.exec();
}
