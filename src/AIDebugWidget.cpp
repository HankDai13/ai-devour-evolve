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
    
    setWindowTitle("🤖 AI调试控制台 - 智能分析面板");
    setMinimumSize(900, 700);
    resize(1000, 800);
    
    // 设置窗口样式
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e1e;
            color: #ffffff;
            font-family: 'Microsoft YaHei', 'Segoe UI', Arial, sans-serif;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #555;
            border-radius: 8px;
            margin: 5px;
            padding-top: 10px;
            background-color: #2a2a2a;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #4fc3f7;
        }
        
        QListWidget {
            background-color: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            alternate-background-color: #333;
            selection-background-color: #0078d4;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 9pt;
        }
        
        QTableWidget {
            background-color: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            alternate-background-color: #333;
            selection-background-color: #0078d4;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 9pt;
        }
        
        QTableWidget::item {
            padding: 3px;
            border-bottom: 1px solid #444;
        }
        
        QTextEdit {
            background-color: #1a1a1a;
            border: 1px solid #555;
            border-radius: 4px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 9pt;
            color: #f0f0f0;
        }
        
        QProgressBar {
            border: 1px solid #555;
            border-radius: 4px;
            background-color: #2a2a2a;
            text-align: center;
            font-weight: bold;
        }
        
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                      stop:0 #4fc3f7, stop:1 #29b6f6);
            border-radius: 3px;
        }
        
        QLabel {
            color: #ffffff;
            font-size: 10pt;
        }
        
        QSplitter::handle {
            background-color: #555;
        }
        
        QSplitter::handle:horizontal {
            width: 3px;
        }
        
        QSplitter::handle:vertical {
            height: 3px;
        }
    )");
    
    addLogEntry("🚀 AI调试控制台已初始化", "SYSTEM");
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
    m_aiStatusGroup = new QGroupBox("🤖 AI玩家状态监控", this);
    QVBoxLayout* layout = new QVBoxLayout(m_aiStatusGroup);
    
    // AI计数标签
    m_aiCountLabel = new QLabel("🤖 总AI玩家: 0 | 存活: 0", m_aiStatusGroup);
    m_activeAILabel = new QLabel("⚡ 活跃AI: 0", m_aiStatusGroup);
    
    // 设置标签样式
    m_aiCountLabel->setStyleSheet("font-weight: bold; color: #4fc3f7;");
    m_activeAILabel->setStyleSheet("font-weight: bold; color: #81c784;");
    
    // AI玩家列表
    m_aiPlayersList = new QListWidget(m_aiStatusGroup);
    m_aiPlayersList->setAlternatingRowColors(true);
    connect(m_aiPlayersList, &QListWidget::itemSelectionChanged, 
            this, &AIDebugWidget::onAIPlayerSelected);
    
    layout->addWidget(m_aiCountLabel);
    layout->addWidget(m_activeAILabel);
    layout->addWidget(m_aiPlayersList);
}

void AIDebugWidget::setupPerformancePanel() {
    m_performanceGroup = new QGroupBox("📊 性能监控面板", this);
    QVBoxLayout* layout = new QVBoxLayout(m_performanceGroup);
    
    // FPS监控
    QHBoxLayout* fpsLayout = new QHBoxLayout();
    m_fpsLabel = new QLabel("🎮 FPS: 0", m_performanceGroup);
    m_fpsLabel->setStyleSheet("font-weight: bold; color: #ffb74d;");
    m_fpsBar = new QProgressBar(m_performanceGroup);
    m_fpsBar->setRange(0, 120);
    m_fpsBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffb74d, stop:1 #ff8a65); }");
    fpsLayout->addWidget(m_fpsLabel);
    fpsLayout->addWidget(m_fpsBar);
    
    // CPU使用率
    QHBoxLayout* cpuLayout = new QHBoxLayout();
    m_cpuLabel = new QLabel("⚙️ CPU: 0%", m_performanceGroup);
    m_cpuLabel->setStyleSheet("font-weight: bold; color: #e57373;");
    m_cpuUsageBar = new QProgressBar(m_performanceGroup);
    m_cpuUsageBar->setRange(0, 100);
    m_cpuUsageBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #e57373, stop:1 #ef5350); }");
    cpuLayout->addWidget(m_cpuLabel);
    cpuLayout->addWidget(m_cpuUsageBar);
    
    // 内存使用率
    QHBoxLayout* memLayout = new QHBoxLayout();
    m_memoryLabel = new QLabel("💾 内存: 0MB", m_performanceGroup);
    m_memoryLabel->setStyleSheet("font-weight: bold; color: #ba68c8;");
    m_memoryUsageBar = new QProgressBar(m_performanceGroup);
    m_memoryUsageBar->setRange(0, 1024); // 1GB
    m_memoryUsageBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ba68c8, stop:1 #ab47bc); }");
    memLayout->addWidget(m_memoryLabel);
    memLayout->addWidget(m_memoryUsageBar);
    
    layout->addLayout(fpsLayout);
    layout->addLayout(cpuLayout);
    layout->addLayout(memLayout);
}

void AIDebugWidget::setupDecisionPanel() {
    m_decisionGroup = new QGroupBox("🧠 AI决策分析面板", this);
    QVBoxLayout* layout = new QVBoxLayout(m_decisionGroup);
    
    // 选中的AI信息
    m_selectedAILabel = new QLabel("📍 选中AI: 无", m_decisionGroup);
    m_selectedAILabel->setStyleSheet("font-weight: bold; color: #4fc3f7;");
    
    m_currentStrategyLabel = new QLabel("🎯 策略: 无", m_decisionGroup);
    m_currentStrategyLabel->setStyleSheet("font-weight: bold; color: #81c784;");
    
    m_lastActionLabel = new QLabel("⚡ 最后动作: 无", m_decisionGroup);
    m_lastActionLabel->setStyleSheet("font-weight: bold; color: #ffb74d;");
    
    // 决策表格
    m_decisionTable = new QTableWidget(0, 3, m_decisionGroup);
    m_decisionTable->setHorizontalHeaderLabels({"⏰ 时间", "🎯 动作类型", "📋 详细信息"});
    m_decisionTable->horizontalHeader()->setStretchLastSection(true);
    m_decisionTable->setAlternatingRowColors(true);
    m_decisionTable->setSortingEnabled(false);
    
    // 设置表格样式
    m_decisionTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #3a3a3a; "
        "color: #ffffff; "
        "font-weight: bold; "
        "border: 1px solid #555; "
        "padding: 4px; "
        "}"
    );
    
    layout->addWidget(m_selectedAILabel);
    layout->addWidget(m_currentStrategyLabel);
    layout->addWidget(m_lastActionLabel);
    layout->addWidget(m_decisionTable);
}

void AIDebugWidget::setupLogPanel() {
    m_logGroup = new QGroupBox("📜 调试日志面板", this);
    QVBoxLayout* layout = new QVBoxLayout(m_logGroup);
    
    m_logTextEdit = new QTextEdit(m_logGroup);
    m_logTextEdit->setReadOnly(true);
    // 注意：setMaximumBlockCount在某些Qt版本中可能不可用
    // m_logTextEdit->setMaximumBlockCount(1000); // 限制日志行数
    
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
    int aliveAI = 0;
    
    for (auto ai : m_monitoredAI) {
        if (ai) {
            if (ai->isAIActive()) {
                activeAI++;
            }
            if (ai->getPlayerBall() && !ai->getPlayerBall()->isRemoved()) {
                aliveAI++;
            }
        }
    }
    
    m_aiCountLabel->setText(QString("🤖 总AI玩家: %1 | 存活: %2").arg(totalAI).arg(aliveAI));
    m_activeAILabel->setText(QString("⚡ 活跃AI: %1").arg(activeAI));
    
    // 更新AI列表
    m_aiPlayersList->clear();
    
    // 按队伍分组显示
    QMap<int, QVector<GoBigger::AI::SimpleAIPlayer*>> teamGroups;
    QVector<GoBigger::AI::SimpleAIPlayer*> unknownTeamAI;
    
    for (auto ai : m_monitoredAI) {
        if (!ai) continue;
        
        if (ai->getPlayerBall() && !ai->getPlayerBall()->isRemoved()) {
            int teamId = ai->getPlayerBall()->teamId();
            teamGroups[teamId].append(ai);
        } else {
            unknownTeamAI.append(ai);
        }
    }
    
    // 定义更丰富的队伍信息
    struct TeamInfo {
        QString name;
        QString colorName;
        QColor bgColor;
        QColor textColor;
    };
    
    QMap<int, TeamInfo> teamInfos = {
        {0, {"🔵 蓝色战队", "蔚蓝", QColor(173, 216, 230), QColor(0, 0, 139)}},
        {1, {"🔴 红色战队", "烈焰", QColor(255, 182, 193), QColor(139, 0, 0)}},
        {2, {"🟢 绿色战队", "翡翠", QColor(144, 238, 144), QColor(0, 100, 0)}},
        {3, {"🟡 黄色战队", "金辉", QColor(255, 255, 224), QColor(184, 134, 11)}},
        {4, {"🟣 紫色战队", "紫罗兰", QColor(221, 160, 221), QColor(75, 0, 130)}},
        {5, {"🟠 橙色战队", "夕阳", QColor(255, 218, 185), QColor(255, 69, 0)}},
        {6, {"🩵 青色战队", "碧空", QColor(175, 238, 238), QColor(0, 139, 139)}},
        {7, {"🩷 粉色战队", "樱花", QColor(255, 192, 203), QColor(199, 21, 133)}}
    };
    
    // 按队伍显示
    for (auto it = teamGroups.begin(); it != teamGroups.end(); ++it) {
        int teamId = it.key();
        const auto& teamAI = it.value();
        
        TeamInfo teamInfo = teamInfos.value(teamId, {"❓ 未知战队", "未知", QColor(200, 200, 200), QColor(64, 64, 64)});
        
        // 计算队伍总分
        float teamTotalScore = 0;
        for (auto ai : teamAI) {
            if (ai->getPlayerBall() && !ai->getPlayerBall()->isRemoved()) {
                teamTotalScore += ai->getPlayerBall()->score();
            }
        }
        
        // 添加队伍标题
        QString teamHeader = QString("═══ %1 (队伍 %2) - 总分: %3 ═══")
                            .arg(teamInfo.name)
                            .arg(teamId)
                            .arg(QString::number(teamTotalScore, 'f', 0));
        
        QListWidgetItem* headerItem = new QListWidgetItem(teamHeader);
        QFont headerFont;
        headerFont.setBold(true);
        headerFont.setPointSize(10);
        headerItem->setFont(headerFont);
        headerItem->setBackground(QBrush(teamInfo.bgColor.darker(120)));
        headerItem->setForeground(QBrush(teamInfo.textColor));
        headerItem->setFlags(Qt::ItemIsEnabled); // 不可选择
        m_aiPlayersList->addItem(headerItem);
        
        // 添加该队伍的AI
        for (int i = 0; i < teamAI.size(); ++i) {
            auto ai = teamAI[i];
            
            QString status = ai->isAIActive() ? "🟢 运行中" : "🔴 已停止";
            QString strategy = "❓ 未知";
            
            switch (ai->getAIStrategy()) {
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM:
                strategy = "🎲 随机模式";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER:
                strategy = "🍎 食物猎人";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE:
                strategy = "⚔️ 攻击策略";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED:
                strategy = "🧠 AI模型";
                break;
            }
            
            CloneBall* ball = ai->getPlayerBall();
            QString aiName = QString("  ├─ AI-P%1").arg(ball->playerId());
            QString scoreInfo = QString("分数: %1").arg(QString::number(ball->score(), 'f', 0));
            QString radiusInfo = QString("半径: %1").arg(QString::number(ball->radius(), 'f', 1));
            QString posInfo = QString("位置: (%1,%2)")
                             .arg(QString::number(ball->pos().x(), 'f', 0))
                             .arg(QString::number(ball->pos().y(), 'f', 0));
            
            QString itemText = QString("%1 【%2】 (%3) | %4 | %5 | %6")
                              .arg(aiName, status, strategy, scoreInfo, radiusInfo, posInfo);
            
            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, QVariant::fromValue(ai));
            
            // 设置样式
            if (ai->isAIActive()) {
                item->setBackground(QBrush(teamInfo.bgColor.lighter(110)));
                item->setForeground(QBrush(teamInfo.textColor));
            } else {
                item->setBackground(QBrush(teamInfo.bgColor.darker(130)));
                item->setForeground(QBrush(teamInfo.textColor.lighter(150)));
            }
            
            m_aiPlayersList->addItem(item);
        }
        
        // 添加空行分隔
        QListWidgetItem* separator = new QListWidgetItem("");
        separator->setFlags(Qt::ItemIsEnabled);
        separator->setSizeHint(QSize(0, 5));
        m_aiPlayersList->addItem(separator);
    }
    
    // 显示已被淘汰的AI
    if (!unknownTeamAI.isEmpty()) {
        QListWidgetItem* destroyedHeader = new QListWidgetItem("═══ 💀 已淘汰的AI ═══");
        QFont headerFont;
        headerFont.setBold(true);
        headerFont.setPointSize(10);
        destroyedHeader->setFont(headerFont);
        destroyedHeader->setBackground(QBrush(QColor(128, 128, 128)));
        destroyedHeader->setForeground(QBrush(QColor(255, 255, 255)));
        destroyedHeader->setFlags(Qt::ItemIsEnabled);
        m_aiPlayersList->addItem(destroyedHeader);
        
        for (auto ai : unknownTeamAI) {
            QString strategy = "❓ 未知";
            switch (ai->getAIStrategy()) {
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM:
                strategy = "🎲 随机模式";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER:
                strategy = "🍎 食物猎人";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE:
                strategy = "⚔️ 攻击策略";
                break;
            case GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED:
                strategy = "🧠 AI模型";
                break;
            }
            
            QString itemText = QString("  💀 AI已被淘汰 (%1) - 状态: %2")
                              .arg(strategy)
                              .arg(ai->isAIActive() ? "仍在运行" : "已停止");
            
            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, QVariant::fromValue(ai));
            item->setBackground(QBrush(QColor(64, 64, 64)));
            item->setForeground(QBrush(QColor(192, 192, 192)));
            
            m_aiPlayersList->addItem(item);
        }
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
        m_fpsLabel->setText(QString("🎮 FPS: %1").arg(m_perfStats.fps, 0, 'f', 1));
        m_fpsBar->setValue(static_cast<int>(qMin(120.0, m_perfStats.fps)));
        
        // 基于AI数量和动作频率计算更真实的CPU使用率
        int totalAI = m_monitoredAI.size();
        int activeAI = 0;
        for (auto ai : m_monitoredAI) {
            if (ai && ai->isAIActive()) {
                activeAI++;
            }
        }
        
        // CPU使用率模拟：基于活跃AI数量和动作频率
        m_perfStats.cpuUsage = qMin(100.0, activeAI * 2.5 + m_perfStats.actionCount * 0.8);
        
        // 内存使用率模拟：基于AI数量和游戏时间
        static int gameSeconds = 0;
        gameSeconds++;
        m_perfStats.memoryUsage = qMin(1024.0, 128.0 + totalAI * 8.0 + gameSeconds * 0.1);
        
        m_cpuLabel->setText(QString("⚙️ CPU: %1%").arg(m_perfStats.cpuUsage, 0, 'f', 1));
        m_cpuUsageBar->setValue(static_cast<int>(m_perfStats.cpuUsage));
        
        m_memoryLabel->setText(QString("💾 内存: %1MB").arg(m_perfStats.memoryUsage, 0, 'f', 1));
        m_memoryUsageBar->setValue(static_cast<int>(m_perfStats.memoryUsage));
        
        // 重置计数器
        m_perfStats.actionCount = 0;
    }
    
    // 每10帧更新一次AI状态（降低更新频率）
    static int updateCounter = 0;
    updateCounter++;
    if (updateCounter >= 10) {
        updateCounter = 0;
        updateAIStatus();
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
