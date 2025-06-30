#include "AIDebugWidget.h"
#include "GameManager.h"
#include <QSplitter>
#include <QHeaderView>
#include <QDateTime>
#include <QApplication>
#include <QProcess>
#include <QDebug>

AIDebugWidget::AIDebugWidget(GameManager* gameManager, QWidget* parent)
    : QWidget(parent)
    , m_gameManager(gameManager)
    , m_selectedAI(nullptr)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    
    // 设置刷新定时器
    m_refreshTimer->setInterval(100); // 10 FPS更新频率
    connect(m_refreshTimer, &QTimer::timeout, this, &AIDebugWidget::onRefreshTimer);
    m_refreshTimer->start();
    
    setWindowTitle("AI Debug Console");
    resize(800, 600);
    
    addLogEntry("AI Debug Console initialized", "SYSTEM");
}

AIDebugWidget::~AIDebugWidget() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

void AIDebugWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // 创建分割器
    QSplitter* mainSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter* topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
    
    // 设置各个面板
    setupAIStatusPanel();
    setupPerformancePanel();
    setupDecisionPanel();
    setupLogPanel();
    
    // 添加到分割器
    topSplitter->addWidget(m_aiStatusGroup);
    topSplitter->addWidget(m_performanceGroup);
    topSplitter->addWidget(m_decisionGroup);
    
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_logGroup);
    
    // 设置分割器比例
    mainSplitter->setStretchFactor(0, 7);
    mainSplitter->setStretchFactor(1, 3);
    
    m_mainLayout->addWidget(mainSplitter);
}

void AIDebugWidget::setupAIStatusPanel() {
    m_aiStatusGroup = new QGroupBox("AI Players Status", this);
    QVBoxLayout* layout = new QVBoxLayout(m_aiStatusGroup);
    
    // AI计数标签
    m_aiCountLabel = new QLabel("Total AI Players: 0", m_aiStatusGroup);
    m_activeAILabel = new QLabel("Active AI: 0", m_aiStatusGroup);
    
    // AI玩家列表
    m_aiPlayersList = new QListWidget(m_aiStatusGroup);
    connect(m_aiPlayersList, &QListWidget::itemSelectionChanged, 
            this, &AIDebugWidget::onAIPlayerSelected);
    
    layout->addWidget(m_aiCountLabel);
    layout->addWidget(m_activeAILabel);
    layout->addWidget(m_aiPlayersList);
}

void AIDebugWidget::setupPerformancePanel() {
    m_performanceGroup = new QGroupBox("Performance Monitor", this);
    QVBoxLayout* layout = new QVBoxLayout(m_performanceGroup);
    
    // FPS监控
    QHBoxLayout* fpsLayout = new QHBoxLayout();
    m_fpsLabel = new QLabel("FPS: 0", m_performanceGroup);
    m_fpsBar = new QProgressBar(m_performanceGroup);
    m_fpsBar->setRange(0, 120);
    fpsLayout->addWidget(m_fpsLabel);
    fpsLayout->addWidget(m_fpsBar);
    
    // CPU使用率
    QHBoxLayout* cpuLayout = new QHBoxLayout();
    m_cpuLabel = new QLabel("CPU: 0%", m_performanceGroup);
    m_cpuUsageBar = new QProgressBar(m_performanceGroup);
    m_cpuUsageBar->setRange(0, 100);
    cpuLayout->addWidget(m_cpuLabel);
    cpuLayout->addWidget(m_cpuUsageBar);
    
    // 内存使用率
    QHBoxLayout* memLayout = new QHBoxLayout();
    m_memoryLabel = new QLabel("Memory: 0MB", m_performanceGroup);
    m_memoryUsageBar = new QProgressBar(m_performanceGroup);
    m_memoryUsageBar->setRange(0, 1024); // 1GB
    memLayout->addWidget(m_memoryLabel);
    memLayout->addWidget(m_memoryUsageBar);
    
    layout->addLayout(fpsLayout);
    layout->addLayout(cpuLayout);
    layout->addLayout(memLayout);
}

void AIDebugWidget::setupDecisionPanel() {
    m_decisionGroup = new QGroupBox("AI Decision Details", this);
    QVBoxLayout* layout = new QVBoxLayout(m_decisionGroup);
    
    // 选中的AI信息
    m_selectedAILabel = new QLabel("Selected AI: None", m_decisionGroup);
    m_currentStrategyLabel = new QLabel("Strategy: N/A", m_decisionGroup);
    m_lastActionLabel = new QLabel("Last Action: N/A", m_decisionGroup);
    
    // 决策表格
    m_decisionTable = new QTableWidget(0, 3, m_decisionGroup);
    m_decisionTable->setHorizontalHeaderLabels({"Time", "Action Type", "Details"});
    m_decisionTable->horizontalHeader()->setStretchLastSection(true);
    m_decisionTable->setAlternatingRowColors(true);
    
    layout->addWidget(m_selectedAILabel);
    layout->addWidget(m_currentStrategyLabel);
    layout->addWidget(m_lastActionLabel);
    layout->addWidget(m_decisionTable);
}

void AIDebugWidget::setupLogPanel() {
    m_logGroup = new QGroupBox("Debug Log", this);
    QVBoxLayout* layout = new QVBoxLayout(m_logGroup);
    
    m_logTextEdit = new QTextEdit(m_logGroup);
    m_logTextEdit->setReadOnly(true);
    
    layout->addWidget(m_logTextEdit);
}

void AIDebugWidget::addAIPlayer(GoBigger::AI::SimpleAIPlayer* aiPlayer) {
    if (!aiPlayer || m_monitoredAI.contains(aiPlayer)) {
        return;
    }
    
    m_monitoredAI.append(aiPlayer);
    
    // 连接信号
    connect(aiPlayer, &GoBigger::AI::SimpleAIPlayer::actionExecuted,
            this, &AIDebugWidget::onAIActionExecuted);
    connect(aiPlayer, &GoBigger::AI::SimpleAIPlayer::strategyChanged,
            this, &AIDebugWidget::onAIStrategyChanged);
    
    // 更新UI
    updateAIStatus();
    
    QString aiName = QString("AI-%1").arg(m_monitoredAI.size());
    if (aiPlayer->getPlayerBall()) {
        aiName = QString("AI-T%1P%2").arg(aiPlayer->getPlayerBall()->teamId()).arg(aiPlayer->getPlayerBall()->playerId());
    }
    
    addLogEntry(QString("Added AI player: %1").arg(aiName), "AI");
}

void AIDebugWidget::removeAIPlayer(GoBigger::AI::SimpleAIPlayer* aiPlayer) {
    if (!aiPlayer) return;
    
    // 断开信号连接
    disconnect(aiPlayer, nullptr, this, nullptr);
    
    m_monitoredAI.removeAll(aiPlayer);
    
    if (m_selectedAI == aiPlayer) {
        m_selectedAI = nullptr;
        m_selectedAILabel->setText("Selected AI: None");
        m_currentStrategyLabel->setText("Strategy: N/A");
        m_lastActionLabel->setText("Last Action: N/A");
    }
    
    updateAIStatus();
    addLogEntry("Removed AI player", "AI");
}

void AIDebugWidget::clearAllAI() {
    for (auto ai : m_monitoredAI) {
        if (ai) {
            disconnect(ai, nullptr, this, nullptr);
        }
    }
    
    m_monitoredAI.clear();
    m_selectedAI = nullptr;
    updateAIStatus();
    
    m_decisionTable->setRowCount(0);
    addLogEntry("Cleared all AI players", "AI");
}

void AIDebugWidget::updateAIStatus() {
    // 更新计数
    int totalAI = m_monitoredAI.size();
    int activeAI = 0;
    
    for (auto ai : m_monitoredAI) {
        if (ai && ai->isAIActive()) {
            activeAI++;
        }
    }
    
    m_aiCountLabel->setText(QString("Total AI Players: %1").arg(totalAI));
    m_activeAILabel->setText(QString("Active AI: %1").arg(activeAI));
    
    // 更新AI列表
    m_aiPlayersList->clear();
    for (int i = 0; i < m_monitoredAI.size(); ++i) {
        auto ai = m_monitoredAI[i];
        if (!ai) continue;
        
        QString status = ai->isAIActive() ? "Active" : "Inactive";
        QString strategy = "Unknown";
        
        switch (ai->getAIStrategy()) {
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM:
            strategy = "Random";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER:
            strategy = "Food Hunter";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE:
            strategy = "Aggressive";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED:
            strategy = "Model-Based";
            break;
        }
        
        QString aiName = QString("AI-%1").arg(i + 1);
        QString scoreInfo = "N/A";
        QString ballColor = "N/A";
        
        if (ai->getPlayerBall() && !ai->getPlayerBall()->isRemoved()) {
            CloneBall* ball = ai->getPlayerBall();
            aiName = QString("AI-T%1P%2").arg(ball->teamId()).arg(ball->playerId());
            scoreInfo = QString("Score: %1").arg(static_cast<int>(ball->score()));
            
            // 根据teamId生成颜色信息
            ballColor = QString("Team %1").arg(ball->teamId());
            
            // 简单的颜色映射
            switch (ball->teamId() % 8) {
                case 0: ballColor += " (Blue)"; break;
                case 1: ballColor += " (Red)"; break;
                case 2: ballColor += " (Green)"; break;
                case 3: ballColor += " (Yellow)"; break;
                case 4: ballColor += " (Purple)"; break;
                case 5: ballColor += " (Orange)"; break;
                case 6: ballColor += " (Cyan)"; break;
                default: ballColor += " (Pink)"; break;
            }
        } else {
            scoreInfo = "Ball Removed";
            ballColor = "N/A";
            status = "Destroyed"; // AI球被吃掉后的状态
        }
        
        // 格式：AI名称 [状态] (策略) | 分数 | 颜色
        QString itemText = QString("%1 [%2] (%3) | %4 | Color: %5")
                           .arg(aiName, status, strategy, scoreInfo, ballColor);
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, QVariant::fromValue(ai));
        
        // 根据状态设置背景色
        if (ai->getPlayerBall() && ai->getPlayerBall()->isRemoved()) {
            item->setBackground(QBrush(QColor(128, 128, 128))); // 灰色：已被吃掉
        } else if (ai->isAIActive()) {
            item->setBackground(QBrush(QColor(200, 255, 200))); // 浅绿色：活跃
        } else {
            item->setBackground(QBrush(QColor(255, 200, 200))); // 浅红色：非活跃
        }
        
        m_aiPlayersList->addItem(item);
    }
}

void AIDebugWidget::onAIActionExecuted(const GoBigger::AI::AIAction& action) {
    GoBigger::AI::SimpleAIPlayer* sender = qobject_cast<GoBigger::AI::SimpleAIPlayer*>(QObject::sender());
    if (!sender) return;
    
    // 更新性能统计
    m_perfStats.actionCount++;
    
    // 如果是选中的AI，更新详细信息
    if (sender == m_selectedAI) {
        QString actionType;
        switch (action.type) {
        case GoBigger::AI::ActionType::MOVE:
            actionType = "MOVE";
            break;
        case GoBigger::AI::ActionType::SPLIT:
            actionType = "SPLIT";
            break;
        case GoBigger::AI::ActionType::EJECT:
            actionType = "EJECT";
            break;
        }
        
        QString actionDetails = QString("dx: %1, dy: %2")
                                    .arg(action.dx, 0, 'f', 2)
                                    .arg(action.dy, 0, 'f', 2);
        
        m_lastActionLabel->setText(QString("Last Action: %1 (%2)")
                                       .arg(actionType, actionDetails));
        
        // 添加到决策表格
        int row = m_decisionTable->rowCount();
        m_decisionTable->insertRow(row);
        
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        m_decisionTable->setItem(row, 0, new QTableWidgetItem(timestamp));
        m_decisionTable->setItem(row, 1, new QTableWidgetItem(actionType));
        m_decisionTable->setItem(row, 2, new QTableWidgetItem(actionDetails));
        
        // 限制表格行数
        if (m_decisionTable->rowCount() > 100) {
            m_decisionTable->removeRow(0);
        }
        
        // 滚动到最新条目
        m_decisionTable->scrollToBottom();
    }
    
    // 记录日志
    QString aiName = sender->getPlayerBall() ? 
                     QString("AI-T%1P%2").arg(sender->getPlayerBall()->teamId()).arg(sender->getPlayerBall()->playerId()) : 
                     "Unknown AI";
    addLogEntry(QString("%1 executed action: %2").arg(aiName).arg(static_cast<int>(action.type)), "ACTION");
}

void AIDebugWidget::onAIStrategyChanged(GoBigger::AI::SimpleAIPlayer::AIStrategy strategy) {
    GoBigger::AI::SimpleAIPlayer* sender = qobject_cast<GoBigger::AI::SimpleAIPlayer*>(QObject::sender());
    if (!sender) return;
    
    QString strategyName;
    switch (strategy) {
    case GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM:
        strategyName = "Random";
        break;
    case GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER:
        strategyName = "Food Hunter";
        break;
    case GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE:
        strategyName = "Aggressive";
        break;
    case GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED:
        strategyName = "Model-Based";
        break;
    }
    
    if (sender == m_selectedAI) {
        m_currentStrategyLabel->setText(QString("Strategy: %1").arg(strategyName));
    }
    
    updateAIStatus();
    
    QString aiName = sender->getPlayerBall() ? 
                     QString("AI-T%1P%2").arg(sender->getPlayerBall()->teamId()).arg(sender->getPlayerBall()->playerId()) : 
                     "Unknown AI";
    addLogEntry(QString("%1 changed strategy to: %2").arg(aiName, strategyName), "STRATEGY");
}

void AIDebugWidget::onRefreshTimer() {
    if (!isVisible()) return;
    
    // 更新性能统计
    static int frameCount = 0;
    static qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
    
    frameCount++;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - lastTime;
    
    if (elapsed >= 1000) { // 每秒更新一次
        m_perfStats.fps = (frameCount * 1000.0) / elapsed;
        frameCount = 0;
        lastTime = currentTime;
        
        // 更新UI
        m_fpsLabel->setText(QString("FPS: %1").arg(m_perfStats.fps, 0, 'f', 1));
        m_fpsBar->setValue(static_cast<int>(m_perfStats.fps));
        
        // 简单的CPU和内存统计（这里只是示例）
        m_perfStats.cpuUsage = qMin(100.0, m_perfStats.actionCount * 0.1);
        m_perfStats.memoryUsage = QApplication::applicationDisplayName().length() * 10; // 示例值
        
        m_cpuLabel->setText(QString("CPU: %1%").arg(m_perfStats.cpuUsage, 0, 'f', 1));
        m_cpuUsageBar->setValue(static_cast<int>(m_perfStats.cpuUsage));
        
        m_memoryLabel->setText(QString("Memory: %1MB").arg(m_perfStats.memoryUsage, 0, 'f', 1));
        m_memoryUsageBar->setValue(static_cast<int>(m_perfStats.memoryUsage));
        
        // 重置计数器
        m_perfStats.actionCount = 0;
    }
}

void AIDebugWidget::onAIPlayerSelected() {
    QListWidgetItem* currentItem = m_aiPlayersList->currentItem();
    if (!currentItem) {
        m_selectedAI = nullptr;
        m_selectedAILabel->setText("Selected AI: None");
        m_currentStrategyLabel->setText("Strategy: N/A");
        m_lastActionLabel->setText("Last Action: N/A");
        m_decisionTable->setRowCount(0);
        return;
    }
    
    m_selectedAI = currentItem->data(Qt::UserRole).value<GoBigger::AI::SimpleAIPlayer*>();
    
    if (m_selectedAI) {
        QString aiName = m_selectedAI->getPlayerBall() ? 
                         QString("AI-T%1P%2").arg(m_selectedAI->getPlayerBall()->teamId()).arg(m_selectedAI->getPlayerBall()->playerId()) : 
                         "Unknown AI";
        m_selectedAILabel->setText(QString("Selected AI: %1").arg(aiName));
        
        QString strategy;
        switch (m_selectedAI->getAIStrategy()) {
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM:
            strategy = "Random";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER:
            strategy = "Food Hunter";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE:
            strategy = "Aggressive";
            break;
        case GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED:
            strategy = "Model-Based";
            break;
        }
        m_currentStrategyLabel->setText(QString("Strategy: %1").arg(strategy));
        
        // 清空决策表格，准备显示新的AI信息
        m_decisionTable->setRowCount(0);
        
        addLogEntry(QString("Selected AI: %1").arg(aiName), "UI");
    }
}

void AIDebugWidget::addLogEntry(const QString& message, const QString& level) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString colorCode;
    
    if (level == "ERROR") colorCode = "red";
    else if (level == "WARNING") colorCode = "orange";
    else if (level == "AI") colorCode = "blue";
    else if (level == "ACTION") colorCode = "green";
    else if (level == "STRATEGY") colorCode = "purple";
    else if (level == "SYSTEM") colorCode = "gray";
    else colorCode = "black";
    
    QString logEntry = QString("<span style='color: %1'>[%2] [%3] %4</span>")
                           .arg(colorCode, timestamp, level, message);
    
    m_logTextEdit->append(logEntry);
    
    // 自动滚动到底部
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);
}

void AIDebugWidget::showDebugInfo(bool show) {
    setVisible(show);
    if (show) {
        updateAIStatus();
        addLogEntry("Debug window opened", "UI");
    }
}

void AIDebugWidget::refreshDebugInfo() {
    updateAIStatus();
    addLogEntry("Debug info refreshed", "UI");
}
