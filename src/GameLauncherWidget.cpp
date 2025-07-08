#include "GameLauncherWidget.h"
#include "GoBiggerConfig.h"
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QEasingCurve>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

GameLauncherWidget::GameLauncherWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentMode(GameMode::SINGLE_DEBUG)
    , m_titleAnimation(nullptr)
    , m_animationTimer(new QTimer(this))
{
    // 设置窗口属性
    setWindowTitle("GoBigger AI - Game Launcher");
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    // 设置主题颜色
    m_primaryColor = "#2C3E50";      // 深蓝灰
    m_secondaryColor = "#34495E";    // 中等蓝灰 
    m_accentColor = "#3498DB";       // 明亮蓝色
    m_backgroundColor = "#ECF0F1";   // 浅灰色
    
    setupUI();
    applyTheme();
    
    // 启动标题动画
    connect(m_animationTimer, &QTimer::timeout, this, &GameLauncherWidget::animateTitle);
    m_animationTimer->start(3000); // 每3秒变换一次
}

GameLauncherWidget::~GameLauncherWidget()
{
    if (m_titleAnimation) {
        m_titleAnimation->stop();
        delete m_titleAnimation;
    }
}

void GameLauncherWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(30);
    m_mainLayout->setContentsMargins(50, 30, 50, 30);
    
    setupTitleArea();
    setupModeButtons();
    setupConfigPanel();
    
    // 默认隐藏配置面板
    hideConfigPanel();
}

void GameLauncherWidget::setupTitleArea()
{
    m_titleWidget = new QWidget(this);
    QVBoxLayout* titleLayout = new QVBoxLayout(m_titleWidget);
    titleLayout->setAlignment(Qt::AlignCenter);
    titleLayout->setSpacing(15);
    
    // 主标题
    m_titleLabel = new QLabel("🎮 GoBigger AI Evolution", m_titleWidget);
    QFont titleFont;
    titleFont.setFamily("Arial");
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: #2C3E50; text-shadow: 2px 2px 4px rgba(0,0,0,0.3);");
    
    // 副标题
    m_subtitleLabel = new QLabel("Choose your game mode and start the AI battle!", m_titleWidget);
    QFont subtitleFont;
    subtitleFont.setFamily("Arial");
    subtitleFont.setPointSize(16);
    subtitleFont.setItalic(true);
    m_subtitleLabel->setFont(subtitleFont);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("color: #7F8C8D; margin-bottom: 20px;");
    
    // 添加阴影效果
    QGraphicsDropShadowEffect* titleShadow = new QGraphicsDropShadowEffect();
    titleShadow->setBlurRadius(10);
    titleShadow->setColor(QColor(0, 0, 0, 100));
    titleShadow->setOffset(3, 3);
    m_titleLabel->setGraphicsEffect(titleShadow);
    
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_subtitleLabel);
    
    m_mainLayout->addWidget(m_titleWidget);
}

void GameLauncherWidget::setupModeButtons()
{
    m_modeButtonsWidget = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(m_modeButtonsWidget);
    buttonLayout->setSpacing(40);
    buttonLayout->setAlignment(Qt::AlignCenter);
    
    // 单人调试模式按钮
    m_singleDebugButton = new QPushButton("🔧 Single Debug\\nMode", m_modeButtonsWidget);
    m_singleDebugButton->setMinimumSize(180, 120);
    m_singleDebugButton->setToolTip("Enter the game in single player mode for debugging and testing");
    connect(m_singleDebugButton, &QPushButton::clicked, this, &GameLauncherWidget::onSingleDebugClicked);
    
    // 生存游戏模式按钮
    m_survivalButton = new QPushButton("⚔️ Survival\\nGame", m_modeButtonsWidget);
    m_survivalButton->setMinimumSize(180, 120);
    m_survivalButton->setToolTip("8 teams battle for survival with AI players");
    connect(m_survivalButton, &QPushButton::clicked, this, &GameLauncherWidget::onSurvivalClicked);
    
    // BOSS模式按钮
    m_bossButton = new QPushButton("👑 BOSS\\nMode", m_modeButtonsWidget);
    m_bossButton->setMinimumSize(180, 120);
    m_bossButton->setToolTip("Challenge powerful AI bosses in epic battles");
    connect(m_bossButton, &QPushButton::clicked, this, &GameLauncherWidget::onBossClicked);
    
    buttonLayout->addWidget(m_singleDebugButton);
    buttonLayout->addWidget(m_survivalButton);
    buttonLayout->addWidget(m_bossButton);
    
    m_mainLayout->addWidget(m_modeButtonsWidget);
}

void GameLauncherWidget::setupConfigPanel()
{
    // 创建可滚动的配置面板
    m_configScrollArea = new QScrollArea(this);
    m_configScrollArea->setWidgetResizable(true);
    m_configScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_configScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_configWidget = new QWidget();
    QVBoxLayout* configLayout = new QVBoxLayout(m_configWidget);
    configLayout->setSpacing(20);
    
    // 配置预览
    m_configPreviewLabel = new QLabel("Configuration Preview", m_configWidget);
    m_configPreviewLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2C3E50; padding: 10px; background-color: #D5DBDB; border-radius: 5px;");
    configLayout->addWidget(m_configPreviewLabel);
    
    setupSurvivalConfig();
    setupBossConfig();
    
    // 控制按钮
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    m_backButton = new QPushButton("⬅️ Back to Menu", m_configWidget);
    m_backButton->setMinimumSize(120, 40);
    connect(m_backButton, &QPushButton::clicked, this, &GameLauncherWidget::onBackToMenuClicked);
    
    m_startGameButton = new QPushButton("🚀 Start Game", m_configWidget);
    m_startGameButton->setMinimumSize(150, 40);
    connect(m_startGameButton, &QPushButton::clicked, this, &GameLauncherWidget::onStartGameClicked);
    
    controlLayout->addWidget(m_backButton);
    controlLayout->addStretch();
    controlLayout->addWidget(m_startGameButton);
    
    configLayout->addLayout(controlLayout);
    
    m_configScrollArea->setWidget(m_configWidget);
    m_mainLayout->addWidget(m_configScrollArea);
}

void GameLauncherWidget::setupSurvivalConfig()
{
    m_survivalConfigGroup = new QGroupBox("⚔️ Survival Game Configuration", m_configWidget);
    QGridLayout* survivalLayout = new QGridLayout(m_survivalConfigGroup);
    
    // 队伍数量
    survivalLayout->addWidget(new QLabel("Number of Teams:"), 0, 0);
    m_teamCountSpin = new QSpinBox();
    m_teamCountSpin->setRange(2, 8);
    m_teamCountSpin->setValue(8);
    m_teamCountSpin->setToolTip("Number of teams in the survival game (2-8)");
    connect(m_teamCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    survivalLayout->addWidget(m_teamCountSpin, 0, 1);
    
    // 初始分数范围
    survivalLayout->addWidget(new QLabel("Initial Score Range:"), 1, 0);
    QHBoxLayout* scoreLayout = new QHBoxLayout();
    m_initialScoreMinSpin = new QSpinBox();
    m_initialScoreMinSpin->setRange(1000, 50000);
    m_initialScoreMinSpin->setValue(5000);
    m_initialScoreMinSpin->setSuffix(" Min");
    connect(m_initialScoreMinSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    
    m_initialScoreMaxSpin = new QSpinBox();
    m_initialScoreMaxSpin->setRange(1000, 50000);
    m_initialScoreMaxSpin->setValue(10000);
    m_initialScoreMaxSpin->setSuffix(" Max");
    connect(m_initialScoreMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    
    scoreLayout->addWidget(m_initialScoreMinSpin);
    scoreLayout->addWidget(m_initialScoreMaxSpin);
    survivalLayout->addLayout(scoreLayout, 1, 1);
    
    // AI策略配置
    survivalLayout->addWidget(new QLabel("AI Strategy 1:"), 2, 0);
    m_aiStrategy1Combo = new QComboBox();
    m_aiStrategy1Combo->addItem("🍎 Food Hunter", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER));
    m_aiStrategy1Combo->addItem("⚔️ Aggressive", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE));
    m_aiStrategy1Combo->addItem("🎲 Random", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM));
    m_aiStrategy1Combo->addItem("🤖 Model-Based", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED));
    m_aiStrategy1Combo->setCurrentIndex(0); // Default: Food Hunter
    connect(m_aiStrategy1Combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameLauncherWidget::updateConfigPreview);
    survivalLayout->addWidget(m_aiStrategy1Combo, 2, 1);
    
    survivalLayout->addWidget(new QLabel("AI Strategy 2:"), 3, 0);
    m_aiStrategy2Combo = new QComboBox();
    m_aiStrategy2Combo->addItem("🍎 Food Hunter", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER));
    m_aiStrategy2Combo->addItem("⚔️ Aggressive", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE));
    m_aiStrategy2Combo->addItem("🎲 Random", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM));
    m_aiStrategy2Combo->addItem("🤖 Model-Based", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED));
    m_aiStrategy2Combo->setCurrentIndex(1); // Default: Aggressive
    connect(m_aiStrategy2Combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameLauncherWidget::updateConfigPreview);
    survivalLayout->addWidget(m_aiStrategy2Combo, 3, 1);
    
    // RL模型支持
    m_enableRLCheckBox = new QCheckBox("Enable RL Model Support");
    m_enableRLCheckBox->setToolTip("Enable reinforcement learning model support for AI players");
    connect(m_enableRLCheckBox, &QCheckBox::toggled, this, &GameLauncherWidget::updateConfigPreview);
    survivalLayout->addWidget(m_enableRLCheckBox, 4, 0, 1, 2);
    
    m_configWidget->layout()->addWidget(m_survivalConfigGroup);
}

void GameLauncherWidget::setupBossConfig()
{
    m_bossConfigGroup = new QGroupBox("👑 BOSS Mode Configuration", m_configWidget);
    QGridLayout* bossLayout = new QGridLayout(m_bossConfigGroup);
    
    // BOSS分数
    bossLayout->addWidget(new QLabel("BOSS Initial Score:"), 0, 0);
    m_bossScoreSpin = new QSpinBox();
    m_bossScoreSpin->setRange(10000, 100000);
    m_bossScoreSpin->setValue(50000);
    m_bossScoreSpin->setSuffix(" points");
    m_bossScoreSpin->setToolTip("Initial score for BOSS AI players");
    connect(m_bossScoreSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    bossLayout->addWidget(m_bossScoreSpin, 0, 1);
    
    // 玩家分数
    bossLayout->addWidget(new QLabel("Player Initial Score:"), 1, 0);
    m_playerScoreSpin = new QSpinBox();
    m_playerScoreSpin->setRange(1000, 20000);
    m_playerScoreSpin->setValue(5000);
    m_playerScoreSpin->setSuffix(" points");
    m_playerScoreSpin->setToolTip("Initial score for human player");
    connect(m_playerScoreSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    bossLayout->addWidget(m_playerScoreSpin, 1, 1);
    
    // BOSS数量
    bossLayout->addWidget(new QLabel("Number of BOSSes:"), 2, 0);
    m_bossCountSpin = new QSpinBox();
    m_bossCountSpin->setRange(1, 4);
    m_bossCountSpin->setValue(2);
    m_bossCountSpin->setToolTip("Number of BOSS AI players to spawn");
    connect(m_bossCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameLauncherWidget::updateConfigPreview);
    bossLayout->addWidget(m_bossCountSpin, 2, 1);
    
    // BOSS策略
    bossLayout->addWidget(new QLabel("BOSS Strategy:"), 3, 0);
    m_bossStrategyCombo = new QComboBox();
    m_bossStrategyCombo->addItem("🤖 Model-Based", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED));
    m_bossStrategyCombo->addItem("⚔️ Aggressive", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE));
    m_bossStrategyCombo->addItem("🍎 Food Hunter", static_cast<int>(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER));
    m_bossStrategyCombo->setCurrentIndex(0); // Default: Model-Based
    connect(m_bossStrategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameLauncherWidget::updateConfigPreview);
    bossLayout->addWidget(m_bossStrategyCombo, 3, 1);
    
    m_configWidget->layout()->addWidget(m_bossConfigGroup);
}

void GameLauncherWidget::applyTheme()
{
    // 主窗口背景
    setStyleSheet(QString("QWidget { background-color: %1; }").arg(m_backgroundColor));
    
    // 按钮样式
    QString buttonStyle = QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 10px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "   transform: scale(1.05);"
        "}"
        "QPushButton:pressed {"
        "   background-color: %3;"
        "}"
    ).arg(m_accentColor, "#2980B9", m_primaryColor);
    
    m_singleDebugButton->setStyleSheet(buttonStyle);
    m_survivalButton->setStyleSheet(buttonStyle);
    m_bossButton->setStyleSheet(buttonStyle);
    m_startGameButton->setStyleSheet(buttonStyle + "QPushButton { background-color: #27AE60; }" +
                                                    "QPushButton:hover { background-color: #229954; }");
    m_backButton->setStyleSheet(buttonStyle + "QPushButton { background-color: #E74C3C; }" +
                                             "QPushButton:hover { background-color: #C0392B; }");
    
    // 配置组样式
    QString groupBoxStyle = QString(
        "QGroupBox {"
        "   font-weight: bold;"
        "   font-size: 16px;"
        "   color: %1;"
        "   border: 2px solid %2;"
        "   border-radius: 8px;"
        "   margin-top: 10px;"
        "   padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   left: 10px;"
        "   padding: 0 8px 0 8px;"
        "   background-color: %3;"
        "   border-radius: 4px;"
        "}"
    ).arg(m_primaryColor, m_accentColor, m_backgroundColor);
    
    if (m_survivalConfigGroup) m_survivalConfigGroup->setStyleSheet(groupBoxStyle);
    if (m_bossConfigGroup) m_bossConfigGroup->setStyleSheet(groupBoxStyle);
    
    // 滚动区域样式
    if (m_configScrollArea) {
        m_configScrollArea->setStyleSheet(
            "QScrollArea {"
            "   border: 1px solid #BDC3C7;"
            "   border-radius: 8px;"
            "   background-color: white;"
            "}"
        );
    }
}

void GameLauncherWidget::onSingleDebugClicked()
{
    GameConfig config;
    config.mode = GameMode::SINGLE_DEBUG;
    config.teamCount = 1;
    config.playersPerTeam = 1;
    config.initialScoreMin = GoBiggerConfig::CELL_INIT_SCORE;
    config.initialScoreMax = GoBiggerConfig::CELL_INIT_SCORE;
    config.enableRLModel = false;
    
    emit gameStartRequested(config);
}

void GameLauncherWidget::onSurvivalClicked()
{
    m_currentMode = GameMode::SURVIVAL;
    showConfigPanel(GameMode::SURVIVAL);
    updateConfigPreview();
}

void GameLauncherWidget::onBossClicked()
{
    m_currentMode = GameMode::BOSS;
    showConfigPanel(GameMode::BOSS);
    updateConfigPreview();
}

void GameLauncherWidget::onStartGameClicked()
{
    GameConfig config = getCurrentConfig();
    emit gameStartRequested(config);
}

void GameLauncherWidget::onBackToMenuClicked()
{
    hideConfigPanel();
}

void GameLauncherWidget::showConfigPanel(GameMode mode)
{
    // 隐藏模式选择按钮
    m_modeButtonsWidget->setVisible(false);
    
    // 显示配置面板
    m_configScrollArea->setVisible(true);
    
    // 根据模式显示相应配置
    if (m_survivalConfigGroup) {
        m_survivalConfigGroup->setVisible(mode == GameMode::SURVIVAL);
    }
    if (m_bossConfigGroup) {
        m_bossConfigGroup->setVisible(mode == GameMode::BOSS);
    }
}

void GameLauncherWidget::hideConfigPanel()
{
    // 显示模式选择按钮
    m_modeButtonsWidget->setVisible(true);
    
    // 隐藏配置面板
    m_configScrollArea->setVisible(false);
}

void GameLauncherWidget::updateConfigPreview()
{
    GameConfig config = getCurrentConfig();
    QString summary = getConfigSummary(config);
    m_configPreviewLabel->setText(summary);
}

GameLauncherWidget::GameConfig GameLauncherWidget::getCurrentConfig() const
{
    GameConfig config;
    config.mode = m_currentMode;
    
    if (m_currentMode == GameMode::SURVIVAL) {
        config.teamCount = m_teamCountSpin->value();
        config.playersPerTeam = 2; // 固定每队2人
        config.initialScoreMin = m_initialScoreMinSpin->value();
        config.initialScoreMax = m_initialScoreMaxSpin->value();
        config.enableRLModel = m_enableRLCheckBox->isChecked();
        
        // 获取AI策略
        auto strategy1 = static_cast<GoBigger::AI::SimpleAIPlayer::AIStrategy>(
            m_aiStrategy1Combo->currentData().toInt());
        auto strategy2 = static_cast<GoBigger::AI::SimpleAIPlayer::AIStrategy>(
            m_aiStrategy2Combo->currentData().toInt());
        config.aiStrategies = {strategy1, strategy2};
        
    } else if (m_currentMode == GameMode::BOSS) {
        config.teamCount = m_bossCountSpin->value() + 1; // BOSS队伍 + 玩家队伍
        config.playersPerTeam = 1;
        config.initialScoreMin = m_playerScoreSpin->value();
        config.initialScoreMax = m_bossScoreSpin->value();
        config.enableRLModel = true; // BOSS模式默认启用RL
        
        auto bossStrategy = static_cast<GoBigger::AI::SimpleAIPlayer::AIStrategy>(
            m_bossStrategyCombo->currentData().toInt());
        config.aiStrategies = {bossStrategy};
    }
    
    return config;
}

QString GameLauncherWidget::getConfigSummary(const GameConfig& config) const
{
    QString summary = "<b>📋 Game Configuration Summary</b><br><br>";
    
    switch (config.mode) {
    case GameMode::SURVIVAL:
        summary += QString("🎮 <b>Mode:</b> Survival Game<br>");
        summary += QString("👥 <b>Teams:</b> %1 teams with %2 players each<br>")
                   .arg(config.teamCount).arg(config.playersPerTeam);
        summary += QString("💯 <b>Initial Score:</b> %1 - %2 points<br>")
                   .arg(config.initialScoreMin).arg(config.initialScoreMax);
        summary += QString("🤖 <b>RL Model:</b> %1<br>")
                   .arg(config.enableRLModel ? "Enabled" : "Disabled");
        break;
        
    case GameMode::BOSS:
        summary += QString("🎮 <b>Mode:</b> BOSS Challenge<br>");
        summary += QString("👑 <b>BOSSes:</b> %1 powerful AI opponents<br>")
                   .arg(config.teamCount - 1);
        summary += QString("💯 <b>Score Range:</b> Player(%1) vs BOSS(%2)<br>")
                   .arg(config.initialScoreMin).arg(config.initialScoreMax);
        summary += QString("🤖 <b>RL Model:</b> Enabled<br>");
        break;
        
    default:
        summary += QString("🎮 <b>Mode:</b> Single Debug<br>");
        summary += QString("🔧 <b>Purpose:</b> Development and testing<br>");
        break;
    }
    
    return summary;
}

void GameLauncherWidget::animateTitle()
{
    // 简单的标题颜色变换动画
    static int colorIndex = 0;
    QStringList colors = {"#2C3E50", "#8E44AD", "#2980B9", "#16A085", "#27AE60", "#F39C12", "#E74C3C"};
    
    QString newColor = colors[colorIndex % colors.size()];
    m_titleLabel->setStyleSheet(QString("color: %1; text-shadow: 2px 2px 4px rgba(0,0,0,0.3);").arg(newColor));
    
    colorIndex++;
}
