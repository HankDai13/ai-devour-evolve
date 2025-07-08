#include "GameStartScreen.h"
#include "GameView.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QGraphicsDropShadowEffect>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFontDatabase>
#include <QPixmap>
#include <QBitmap>
#include <QDebug>
#include <QFormLayout>
#include <QSplitter>

// 主题颜色常量
const QString GameStartScreen::THEME_COLOR_PRIMARY = "#1e3a8a";      // 深蓝色
const QString GameStartScreen::THEME_COLOR_SECONDARY = "#3b82f6";    // 蓝色
const QString GameStartScreen::THEME_COLOR_ACCENT = "#10b981";       // 绿色
const QString GameStartScreen::THEME_COLOR_BACKGROUND = "#0f172a";   // 深色背景
const QString GameStartScreen::THEME_COLOR_SURFACE = "#1e293b";      // 表面色
const QString GameStartScreen::THEME_COLOR_TEXT = "#f1f5f9";         // 文本色
const QString GameStartScreen::THEME_COLOR_TEXT_SECONDARY = "#94a3b8"; // 次要文本色

GameStartScreen::GameStartScreen(QWidget* parent)
    : QWidget(parent)
    , m_currentMode(GameMode::DEBUG_SINGLE_PLAYER)
    , m_isConfigurationValid(true)
    , m_fadeAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_animationTimer(nullptr)
{
    setupUI();
    setupAnimations();
    applyTheme();
    
    // 初始化默认配置
    resetConfigurationToDefaults();
    
    // 设置窗口属性
    setWindowTitle("🎮 智能吞噬进化 - 游戏启动器");
    setMinimumSize(1000, 700);
    resize(1200, 800);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        move(screenGeometry.center() - rect().center());
    }
    
    // 设置窗口标志
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    
    qDebug() << "GameStartScreen created successfully";
}

GameStartScreen::~GameStartScreen() {
    if (m_animationTimer) {
        m_animationTimer->stop();
    }
}

void GameStartScreen::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 创建标题
    QLabel* titleLabel = new QLabel("🎮 智能吞噬进化", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel { "
        "font-size: 32px; "
        "font-weight: bold; "
        "color: #3b82f6; "
        "margin: 10px; "
        "}"
    );
    
    QLabel* subtitleLabel = new QLabel("AI Devour Evolve - 选择您的游戏模式", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "QLabel { "
        "font-size: 14px; "
        "color: #94a3b8; "
        "margin-bottom: 20px; "
        "}"
    );
    
    m_mainLayout->addWidget(titleLabel);
    m_mainLayout->addWidget(subtitleLabel);
    
    // 创建内容布局
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(20);
    
    // 设置面板
    setupModeSelectionPanel();
    setupConfigurationPanel();
    
    m_contentLayout->addWidget(m_modeSelectionGroup, 1);
    m_contentLayout->addWidget(m_configurationGroup, 2);
    
    m_mainLayout->addLayout(m_contentLayout, 1);
    
    // 设置操作面板
    setupActionPanel();
    
    // 连接信号
    connect(m_teamsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &GameStartScreen::onConfigurationChanged);
    connect(m_scoreMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &GameStartScreen::onConfigurationChanged);
    connect(m_scoreMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &GameStartScreen::onConfigurationChanged);
}

void GameStartScreen::setupModeSelectionPanel() {
    m_modeSelectionGroup = new QGroupBox("🎯 游戏模式选择", this);
    m_modeLayout = new QVBoxLayout(m_modeSelectionGroup);
    m_modeLayout->setSpacing(15);
    
    // 创建模式按钮组
    m_modeButtonGroup = new QButtonGroup(this);
    
    // 单人调试模式
    m_debugModeBtn = new QPushButton("🔧 单人调试模式", this);
    m_debugModeBtn->setCheckable(true);
    m_debugModeBtn->setChecked(true);
    m_debugModeBtn->setMinimumHeight(60);
    m_debugModeBtn->setProperty("mode", static_cast<int>(GameMode::DEBUG_SINGLE_PLAYER));
    
    // 生存游戏模式
    m_survivalModeBtn = new QPushButton("⚔️ 生存游戏模式", this);
    m_survivalModeBtn->setCheckable(true);
    m_survivalModeBtn->setMinimumHeight(60);
    m_survivalModeBtn->setProperty("mode", static_cast<int>(GameMode::SURVIVAL_BATTLE));
    
    // BOSS挑战模式
    m_bossModeBtn = new QPushButton("👑 BOSS挑战模式", this);
    m_bossModeBtn->setCheckable(true);
    m_bossModeBtn->setMinimumHeight(60);
    m_bossModeBtn->setProperty("mode", static_cast<int>(GameMode::BOSS_CHALLENGE));
    
    // 添加到按钮组
    m_modeButtonGroup->addButton(m_debugModeBtn);
    m_modeButtonGroup->addButton(m_survivalModeBtn);
    m_modeButtonGroup->addButton(m_bossModeBtn);
    
    // 连接信号
    connect(m_modeButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &GameStartScreen::onModeButtonClicked);
    
    // 模式描述标签
    m_modeDescriptionLabel = new QLabel(this);
    m_modeDescriptionLabel->setWordWrap(true);
    m_modeDescriptionLabel->setMinimumHeight(120);
    m_modeDescriptionLabel->setAlignment(Qt::AlignTop);
    
    // 添加到布局
    m_modeLayout->addWidget(m_debugModeBtn);
    m_modeLayout->addWidget(m_survivalModeBtn);
    m_modeLayout->addWidget(m_bossModeBtn);
    m_modeLayout->addSpacing(10);
    m_modeLayout->addWidget(m_modeDescriptionLabel);
    m_modeLayout->addStretch();
    
    // 初始化模式描述
    updateModeDescription();
}

void GameStartScreen::setupConfigurationPanel() {
    m_configurationGroup = new QGroupBox("⚙️ 游戏配置", this);
    QVBoxLayout* configMainLayout = new QVBoxLayout(m_configurationGroup);
    
    // 创建滚动区域
    m_configScrollArea = new QScrollArea(this);
    m_configScrollArea->setWidgetResizable(true);
    m_configScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_configScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 创建配置标签页
    m_configTabs = new QTabWidget(this);
    
    // 基本配置标签页
    m_basicConfigTab = new QWidget();
    setupBasicConfigTab();
    m_configTabs->addTab(m_basicConfigTab, "🎮 基本设置");
    
    // AI配置标签页
    m_aiConfigTab = new QWidget();
    setupAIConfigTab();
    m_configTabs->addTab(m_aiConfigTab, "🤖 AI设置");
    
    // 世界配置标签页
    m_worldConfigTab = new QWidget();
    setupWorldConfigTab();
    m_configTabs->addTab(m_worldConfigTab, "🌍 世界设置");
    
    // 高级配置标签页
    m_advancedConfigTab = new QWidget();
    setupAdvancedConfigTab();
    m_configTabs->addTab(m_advancedConfigTab, "🔧 高级设置");
    
    m_configScrollArea->setWidget(m_configTabs);
    configMainLayout->addWidget(m_configScrollArea);
}

void GameStartScreen::setupBasicConfigTab() {
    m_basicConfigTab = new QWidget();
    auto* layout = new QFormLayout(m_basicConfigTab);
    
    // 队伍数量配置
    m_teamsLabel = new QLabel("队伍数量:");
    m_teamsSpinBox = new QSpinBox();
    m_teamsSpinBox->setRange(2, 16);
    m_teamsSpinBox->setValue(8);
    m_teamsSpinBox->setToolTip("游戏中的队伍数量");
    
    connect(m_teamsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    
    layout->addRow(m_teamsLabel, m_teamsSpinBox);
    
    // 分数范围配置
    m_scoreRangeLabel = new QLabel("初始分数范围:");
    
    auto* scoreWidget = new QWidget();
    auto* scoreLayout = new QHBoxLayout(scoreWidget);
    
    m_scoreMinSpinBox = new QSpinBox();
    m_scoreMinSpinBox->setRange(1000, 50000);
    m_scoreMinSpinBox->setValue(5000);
    m_scoreMinSpinBox->setSuffix(" 分");
    
    m_scoreMaxSpinBox = new QSpinBox();
    m_scoreMaxSpinBox->setRange(1000, 50000);
    m_scoreMaxSpinBox->setValue(10000);
    m_scoreMaxSpinBox->setSuffix(" 分");
    
    connect(m_scoreMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    connect(m_scoreMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    
    scoreLayout->addWidget(new QLabel("最小:"));
    scoreLayout->addWidget(m_scoreMinSpinBox);
    scoreLayout->addWidget(new QLabel("最大:"));
    scoreLayout->addWidget(m_scoreMaxSpinBox);
    
    layout->addRow(m_scoreRangeLabel, scoreWidget);
    
    // 注意：此标签页已在setupConfigurationPanel中添加
}

void GameStartScreen::setupAIConfigTab() {
    m_aiConfigTab = new QWidget();
    m_aiConfigLayout = new QVBoxLayout(m_aiConfigTab);
    
    auto* titleLabel = new QLabel("AI队伍配置");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 10px;");
    m_aiConfigLayout->addWidget(titleLabel);
    
    // 添加滚动区域
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; }");
    
    auto* scrollWidget = new QWidget();
    auto* scrollLayout = new QVBoxLayout(scrollWidget);
    
    // 这里会根据队伍数量动态创建AI配置界面
    updateAIConfigInterface();
    
    scrollArea->setWidget(scrollWidget);
    m_aiConfigLayout->addWidget(scrollArea);
    
    // 注意：此标签页已在setupConfigurationPanel中添加
}

void GameStartScreen::setupWorldConfigTab() {
    m_worldConfigTab = new QWidget();
    auto* layout = new QFormLayout(m_worldConfigTab);
    
    // 世界大小配置
    m_worldSizeLabel = new QLabel("世界大小:");
    m_worldSizeSpinBox = new QSpinBox();
    m_worldSizeSpinBox->setRange(1000, 5000);
    m_worldSizeSpinBox->setValue(2000);
    m_worldSizeSpinBox->setSuffix(" 像素");
    
    connect(m_worldSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    layout->addRow(m_worldSizeLabel, m_worldSizeSpinBox);
    
    // 食物密度配置
    m_foodDensityLabel = new QLabel("食物密度: 100%");
    m_foodDensitySlider = new QSlider(Qt::Horizontal);
    m_foodDensitySlider->setRange(10, 200);
    m_foodDensitySlider->setValue(100);
    
    connect(m_foodDensitySlider, &QSlider::valueChanged, [this](int value) {
        m_foodDensityLabel->setText(QString("食物密度: %1%").arg(value));
        onConfigurationChanged();
    });
    
    layout->addRow(m_foodDensityLabel, m_foodDensitySlider);
    
    // 荆棘密度配置
    m_thornDensityLabel = new QLabel("荆棘密度: 50%");
    m_thornDensitySlider = new QSlider(Qt::Horizontal);
    m_thornDensitySlider->setRange(0, 100);
    m_thornDensitySlider->setValue(50);
    
    connect(m_thornDensitySlider, &QSlider::valueChanged, [this](int value) {
        m_thornDensityLabel->setText(QString("荆棘密度: %1%").arg(value));
        onConfigurationChanged();
    });
    
    layout->addRow(m_thornDensityLabel, m_thornDensitySlider);
    
    // 注意：此标签页已在setupConfigurationPanel中添加
}

void GameStartScreen::setupAdvancedConfigTab() {
    m_advancedConfigTab = new QWidget();
    auto* layout = new QFormLayout(m_advancedConfigTab);
    
    // 团队模式配置
    m_teamModeCheckBox = new QCheckBox("启用团队模式");
    m_teamModeCheckBox->setChecked(true);
    
    m_friendlyFireCheckBox = new QCheckBox("启用友军伤害");
    m_friendlyFireCheckBox->setChecked(false);
    
    connect(m_teamModeCheckBox, &QCheckBox::toggled, this, &GameStartScreen::onConfigurationChanged);
    connect(m_friendlyFireCheckBox, &QCheckBox::toggled, this, &GameStartScreen::onConfigurationChanged);
    
    layout->addRow(m_teamModeCheckBox);
    layout->addRow(m_friendlyFireCheckBox);
    
    // 时间限制配置
    m_timeLimitLabel = new QLabel("游戏时间限制:");
    m_timeLimitSpinBox = new QSpinBox();
    m_timeLimitSpinBox->setRange(0, 3600);
    m_timeLimitSpinBox->setValue(0);
    m_timeLimitSpinBox->setSuffix(" 秒 (0=无限制)");
    
    connect(m_timeLimitSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    layout->addRow(m_timeLimitLabel, m_timeLimitSpinBox);
    
    // BOSS模式特殊配置
    m_bossConfigWidget = new QWidget(m_advancedConfigTab);
    setupBossConfigWidget();
    
    // 根据当前模式显示/隐藏BOSS配置
    if (m_currentMode == GameMode::BOSS_CHALLENGE) {
        m_bossConfigWidget->show();
    } else {
        m_bossConfigWidget->hide();
    }
    
    layout->addRow(m_bossConfigWidget);
    
    // 注意：此标签页已在setupConfigurationPanel中添加
}

void GameStartScreen::setupBossConfigWidget() {
    m_bossConfigWidget = new QWidget();
    auto* layout = new QFormLayout(m_bossConfigWidget);
    
    // BOSS初始分数
    m_bossScoreLabel = new QLabel("BOSS初始分数:");
    m_bossScoreSpinBox = new QSpinBox();
    m_bossScoreSpinBox->setRange(10000, 100000);
    m_bossScoreSpinBox->setValue(50000);
    m_bossScoreSpinBox->setSuffix(" 分");
    
    layout->addRow(m_bossScoreLabel, m_bossScoreSpinBox);
    
    // BOSS队伍选择
    m_bossTeamComboBox = new QComboBox();
    updateBossTeamComboBox();
    
    layout->addRow(new QLabel("BOSS队伍:"), m_bossTeamComboBox);
    
    // BOSS特殊能力
    m_bossAbilitiesCheckBox = new QCheckBox("启用BOSS特殊能力");
    m_bossAbilitiesCheckBox->setChecked(true);
    
    layout->addRow(m_bossAbilitiesCheckBox);
    
    connect(m_bossScoreSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
    connect(m_bossTeamComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameStartScreen::onConfigurationChanged);
    connect(m_bossAbilitiesCheckBox, &QCheckBox::toggled, this, &GameStartScreen::onConfigurationChanged);
}

void GameStartScreen::setupActionPanel() {
    m_actionLayout = new QHBoxLayout();
    m_actionLayout->setSpacing(15);
    
    // 配置管理按钮
    m_defaultsBtn = new QPushButton("🔄 恢复默认", this);
    m_saveConfigBtn = new QPushButton("💾 保存配置", this);
    m_loadConfigBtn = new QPushButton("📁 加载配置", this);
    
    // 主要操作按钮
    m_startGameBtn = new QPushButton("🚀 开始游戏", this);
    m_startGameBtn->setMinimumHeight(50);
    m_startGameBtn->setMinimumWidth(150);
    
    m_exitBtn = new QPushButton("❌ 退出", this);
    m_exitBtn->setMinimumHeight(50);
    
    // 设置按钮样式
    QString buttonStyle = 
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3b82f6, stop:1 #1e40af); "
        "border: none; "
        "border-radius: 8px; "
        "color: white; "
        "font-weight: bold; "
        "padding: 8px 16px; "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2563eb, stop:1 #1d4ed8); "
        "} "
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1d4ed8, stop:1 #1e3a8a); "
        "}";
    
    m_startGameBtn->setStyleSheet(buttonStyle);
    m_defaultsBtn->setStyleSheet(buttonStyle);
    m_saveConfigBtn->setStyleSheet(buttonStyle);
    m_loadConfigBtn->setStyleSheet(buttonStyle);
    
    QString exitButtonStyle = 
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ef4444, stop:1 #dc2626); "
        "border: none; "
        "border-radius: 8px; "
        "color: white; "
        "font-weight: bold; "
        "padding: 8px 16px; "
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #dc2626, stop:1 #b91c1c); "
        "} "
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b91c1c, stop:1 #991b1b); "
        "}";
    
    m_exitBtn->setStyleSheet(exitButtonStyle);
    
    // 连接信号
    connect(m_startGameBtn, &QPushButton::clicked, this, &GameStartScreen::onStartGameClicked);
    connect(m_exitBtn, &QPushButton::clicked, this, &GameStartScreen::onExitClicked);
    connect(m_defaultsBtn, &QPushButton::clicked, this, &GameStartScreen::onRestoreDefaultsClicked);
    connect(m_saveConfigBtn, &QPushButton::clicked, this, &GameStartScreen::onSaveConfigClicked);
    connect(m_loadConfigBtn, &QPushButton::clicked, this, &GameStartScreen::onLoadConfigClicked);
    
    // 添加到布局
    m_actionLayout->addWidget(m_defaultsBtn);
    m_actionLayout->addWidget(m_saveConfigBtn);
    m_actionLayout->addWidget(m_loadConfigBtn);
    m_actionLayout->addStretch();
    m_actionLayout->addWidget(m_startGameBtn);
    m_actionLayout->addWidget(m_exitBtn);
    
    m_mainLayout->addLayout(m_actionLayout);
    
    // 初始化按钮文本
    updateStartButtonText();
}

void GameStartScreen::onModeButtonClicked() {
    QAbstractButton* button = m_modeButtonGroup->checkedButton();
    if (!button) return;
    
    GameMode newMode = static_cast<GameMode>(button->property("mode").toInt());
    if (newMode != m_currentMode) {
        switchToMode(newMode);
    }
}

void GameStartScreen::onStartGameClicked() {
    // 验证配置
    if (!validateConfiguration()) {
        QMessageBox::warning(this, "配置错误", "请检查您的配置设置，确保所有参数都有效。");
        return;
    }
    
    // 更新游戏配置
    updateGameConfig();
    
    // 发射开始游戏信号
    emit startGameRequested(m_gameConfig);
    
    // 隐藏启动界面
    hide();
}

void GameStartScreen::onExitClicked() {
    emit exitRequested();
    close();
}

void GameStartScreen::onConfigurationChanged() {
    // 更新配置有效性
    m_isConfigurationValid = validateConfiguration();
    
    // 更新UI状态
    updateStartButtonText();
    updateConfigurationVisibility();
    
    // 如果是队伍数量改变，更新相关UI
    if (sender() == m_teamsSpinBox) {
        updateAIConfigInterface();
        updateBossTeamComboBox();
    }
    
    // 确保分数范围合理
    if (sender() == m_scoreMinSpinBox) {
        if (m_scoreMinSpinBox->value() >= m_scoreMaxSpinBox->value()) {
            m_scoreMaxSpinBox->setValue(m_scoreMinSpinBox->value() + 1000);
        }
    } else if (sender() == m_scoreMaxSpinBox) {
        if (m_scoreMaxSpinBox->value() <= m_scoreMinSpinBox->value()) {
            m_scoreMinSpinBox->setValue(m_scoreMaxSpinBox->value() - 1000);
        }
    }
}

void GameStartScreen::onRestoreDefaultsClicked() {
    int result = QMessageBox::question(this, "确认重置", 
                                     "是否确定要恢复所有设置到默认值？\n这将清除当前的所有配置.",
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        resetConfigurationToDefaults();
    }
}

void GameStartScreen::onSaveConfigClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                  "保存游戏配置",
                                                  QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/game_config.json",
                                                  "JSON 配置文件 (*.json)");
    
    if (!fileName.isEmpty()) {
        QJsonObject config = configToJson();
        QJsonDocument doc(config);
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            QMessageBox::information(this, "保存成功", "配置已成功保存到：\n" + fileName);
        } else {
            QMessageBox::critical(this, "保存失败", "无法保存配置文件。请检查文件权限。");
        }
    }
}

void GameStartScreen::onLoadConfigClicked() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                  "加载游戏配置",
                                                  QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                  "JSON 配置文件 (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            
            if (error.error == QJsonParseError::NoError) {
                configFromJson(doc.object());
                QMessageBox::information(this, "加载成功", "配置已成功从文件加载。");
            } else {
                QMessageBox::critical(this, "加载失败", "配置文件格式错误：\n" + error.errorString());
            }
        } else {
            QMessageBox::critical(this, "加载失败", "无法打开配置文件。");
        }
    }
}

void GameStartScreen::switchToMode(GameMode mode) {
    m_currentMode = mode;
    
    // 更新模式描述
    updateModeDescription();
    
    // 更新配置界面可见性
    updateConfigurationVisibility();
    
    // 更新开始按钮文本
    updateStartButtonText();
    
    // 重置配置到该模式的默认值
    resetConfigurationToDefaults();
    
    qDebug() << "Switched to mode:" << static_cast<int>(mode);
}

void GameStartScreen::updateModeDescription() {
    QString description;
    QString styleSheet = "QLabel { color: #94a3b8; padding: 10px; background-color: #1e293b; border-radius: 8px; }";
    
    switch (m_currentMode) {
    case GameMode::DEBUG_SINGLE_PLAYER:
        description = "<b>🔧 单人调试模式</b><br/><br/>"
                     "这是默认的开发调试模式，适合：<br/>"
                     "• 测试新功能和AI算法<br/>"
                     "• 单人游戏体验<br/>"
                     "• 学习游戏机制<br/><br/>"
                     "特点：<br/>"
                     "• 默认不添加AI玩家<br/>"
                     "• 可以手动添加AI进行测试<br/>"
                     "• 完全的自定义控制";
        break;
        
    case GameMode::SURVIVAL_BATTLE:
        description = "<b>⚔️ 生存游戏模式</b><br/><br/>"
                     "激烈的多队伍生存竞赛模式：<br/>"
                     "• 8支队伍同时开局竞争<br/>"
                     "• 每队配备智能AI玩家<br/>"
                     "• 高起始分数，快节奏对战<br/><br/>"
                     "AI策略组合：<br/>"
                     "• 食物猎人：稳健发展<br/>"
                     "• 攻击策略：主动进攻<br/>"
                     "• 支持RL模型AI";
        break;
        
    case GameMode::BOSS_CHALLENGE:
        description = "<b>👑 BOSS挑战模式</b><br/><br/>"
                     "终极挑战模式，对抗超强BOSS：<br/>"
                     "• 一支超强BOSS队伍<br/>"
                     "• 多支挑战者队伍<br/>"
                     "• BOSS拥有特殊能力<br/><br/>"
                     "挑战特色：<br/>"
                     "• BOSS初始分数极高<br/>"
                     "• 挑战者需要团队协作<br/>"
                     "• 激动人心的对抗体验";
        break;
    }
    
    m_modeDescriptionLabel->setText(description);
    m_modeDescriptionLabel->setStyleSheet(styleSheet);
}

void GameStartScreen::updateStartButtonText() {
    QString buttonText;
    
    switch (m_currentMode) {
    case GameMode::DEBUG_SINGLE_PLAYER:
        buttonText = "🔧 开始调试模式";
        break;
    case GameMode::SURVIVAL_BATTLE:
        buttonText = "⚔️ 开始生存战斗";
        break;
    case GameMode::BOSS_CHALLENGE:
        buttonText = "👑 挑战BOSS";
        break;
    }
    
    m_startGameBtn->setText(buttonText);
    m_startGameBtn->setEnabled(m_isConfigurationValid);
}

void GameStartScreen::updateConfigurationVisibility() {
    // 根据模式显示/隐藏相关配置
    bool showAIConfig = (m_currentMode != GameMode::DEBUG_SINGLE_PLAYER);
    bool showBossConfig = (m_currentMode == GameMode::BOSS_CHALLENGE);
    
    // AI配置标签页
    m_configTabs->setTabEnabled(1, showAIConfig);
    
    // BOSS配置
    if (m_bossConfigWidget) {
        m_bossConfigWidget->setVisible(showBossConfig);
    }
    
    // 根据模式调整默认值
    if (m_currentMode == GameMode::DEBUG_SINGLE_PLAYER) {
        m_teamsSpinBox->setValue(1);
        m_teamsSpinBox->setEnabled(false);
    } else {
        m_teamsSpinBox->setEnabled(true);
        if (m_currentMode == GameMode::SURVIVAL_BATTLE) {
            m_teamsSpinBox->setValue(8);
        } else if (m_currentMode == GameMode::BOSS_CHALLENGE) {
            m_teamsSpinBox->setValue(6); // 5支挑战者队伍 + 1支BOSS队伍
        }
    }
}

void GameStartScreen::resetConfigurationToDefaults() {
    // 阻止信号循环
    const bool oldState = blockSignals(true);
    
    switch (m_currentMode) {
    case GameMode::DEBUG_SINGLE_PLAYER:
        m_teamsSpinBox->setValue(1);
        m_scoreMinSpinBox->setValue(3000);
        m_scoreMaxSpinBox->setValue(5000);
        break;
        
    case GameMode::SURVIVAL_BATTLE:
        m_teamsSpinBox->setValue(8);
        m_scoreMinSpinBox->setValue(5000);
        m_scoreMaxSpinBox->setValue(10000);
        break;
        
    case GameMode::BOSS_CHALLENGE:
        m_teamsSpinBox->setValue(6);
        m_scoreMinSpinBox->setValue(3000);
        m_scoreMaxSpinBox->setValue(6000);
        if (m_bossScoreSpinBox) {
            m_bossScoreSpinBox->setValue(50000);
        }
        break;
    }
    
    // 世界设置
    m_worldSizeSpinBox->setValue(2000);
    m_foodDensitySlider->setValue(100);
    m_thornDensitySlider->setValue(50);
    
    // 游戏规则
    m_teamModeCheckBox->setChecked(true);
    m_friendlyFireCheckBox->setChecked(false);
    m_timeLimitSpinBox->setValue(0);
    
    // 恢复信号
    blockSignals(oldState);
    
    // 更新配置界面
    updateConfigurationVisibility();
    updateAIConfigInterface();
    updateBossTeamComboBox();
    
    // 手动触发配置更新
    onConfigurationChanged();
}

bool GameStartScreen::validateConfiguration() {
    // 检查基本配置
    if (m_scoreMinSpinBox->value() >= m_scoreMaxSpinBox->value()) {
        return false;
    }
    
    if (m_teamsSpinBox->value() < 1) {
        return false;
    }
    
    // 检查世界配置
    if (m_worldSizeSpinBox->value() < 1000) {
        return false;
    }
    
    // 检查BOSS模式特殊配置
    if (m_currentMode == GameMode::BOSS_CHALLENGE) {
        if (m_bossScoreSpinBox && m_bossScoreSpinBox->value() < m_scoreMaxSpinBox->value()) {
            return false; // BOSS分数应该比普通玩家高
        }
    }
    
    return true;
}

void GameStartScreen::updateGameConfig() {
    m_gameConfig.mode = m_currentMode;
    m_gameConfig.totalTeams = m_teamsSpinBox->value();
    m_gameConfig.initialScoreMin = m_scoreMinSpinBox->value();
    m_gameConfig.initialScoreMax = m_scoreMaxSpinBox->value();
    
    // 世界配置
    m_gameConfig.worldSize = m_worldSizeSpinBox->value();
    m_gameConfig.foodDensity = m_foodDensitySlider->value();
    m_gameConfig.thornDensity = m_thornDensitySlider->value();
    
    // 游戏规则
    m_gameConfig.enableTeamMode = m_teamModeCheckBox->isChecked();
    m_gameConfig.enableFriendlyFire = m_friendlyFireCheckBox->isChecked();
    m_gameConfig.gameTimeLimit = m_timeLimitSpinBox->value();
    
    // BOSS模式配置
    if (m_currentMode == GameMode::BOSS_CHALLENGE) {
        m_gameConfig.bossInitialScore = m_bossScoreSpinBox->value();
        m_gameConfig.bossTeamId = m_bossTeamComboBox->currentIndex();
        m_gameConfig.enableBossSpecialAbilities = m_bossAbilitiesCheckBox->isChecked();
    }
    
    // AI配置
    m_gameConfig.teamAIConfigs.clear();
    
    switch (m_currentMode) {
    case GameMode::DEBUG_SINGLE_PLAYER:
        // 调试模式不预设AI
        break;
        
    case GameMode::SURVIVAL_BATTLE:
        // 为每支队伍配置默认AI
        for (int teamId = 0; teamId < m_gameConfig.totalTeams; ++teamId) {
            AIConfig aiConfig;
            aiConfig.name = QString("队伍 %1").arg(teamId + 1);
            aiConfig.description = "生存模式AI配置";
            aiConfig.foodHunterCount = 1;
            aiConfig.aggressiveCount = 1;
            aiConfig.modelBasedCount = 0; // 如果有RL模型可以设置为1
            aiConfig.randomCount = 0;
            
            m_gameConfig.teamAIConfigs[teamId] = aiConfig;
        }
        break;
        
    case GameMode::BOSS_CHALLENGE:
        // BOSS队伍配置
        {
            AIConfig bossConfig;
            bossConfig.name = "BOSS队伍";
            bossConfig.description = "终极BOSS";
            bossConfig.aggressiveCount = 2;
            bossConfig.modelBasedCount = 1;
            
            m_gameConfig.teamAIConfigs[m_gameConfig.bossTeamId] = bossConfig;
        }
        
        // 挑战者队伍配置
        for (int teamId = 0; teamId < m_gameConfig.totalTeams; ++teamId) {
            if (teamId != m_gameConfig.bossTeamId) {
                AIConfig challengerConfig;
                challengerConfig.name = QString("挑战者队伍 %1").arg(teamId + 1);
                challengerConfig.description = "BOSS挑战者";
                challengerConfig.foodHunterCount = 1;
                challengerConfig.aggressiveCount = 1;
                
                m_gameConfig.teamAIConfigs[teamId] = challengerConfig;
            }
        }
        break;
    }
    
    qDebug() << "Game configuration updated for mode:" << static_cast<int>(m_currentMode);
}

void GameStartScreen::updateAIConfigInterface() {
    // 清除现有的AI配置界面
    for (auto* widget : m_teamConfigWidgets.values()) {
        widget->deleteLater();
    }
    m_teamConfigWidgets.clear();
    
    // 根据队伍数量创建AI配置界面
    int teamCount = m_teamsSpinBox ? m_teamsSpinBox->value() : 8;
    
    for (int i = 0; i < teamCount; ++i) {
        auto* teamWidget = new QGroupBox(QString("队伍 %1").arg(i + 1));
        auto* teamLayout = new QFormLayout(teamWidget);
        
        // 创建各种AI类型的配置
        auto* foodHunterSpinBox = new QSpinBox();
        foodHunterSpinBox->setRange(0, 10);
        foodHunterSpinBox->setValue(2);
        teamLayout->addRow("食物猎手AI:", foodHunterSpinBox);
        
        auto* aggressiveSpinBox = new QSpinBox();
        aggressiveSpinBox->setRange(0, 10);
        aggressiveSpinBox->setValue(1);
        teamLayout->addRow("攻击性AI:", aggressiveSpinBox);
        
        auto* modelBasedSpinBox = new QSpinBox();
        modelBasedSpinBox->setRange(0, 10);
        modelBasedSpinBox->setValue(0);
        teamLayout->addRow("模型AI:", modelBasedSpinBox);
        
        auto* randomSpinBox = new QSpinBox();
        randomSpinBox->setRange(0, 10);
        randomSpinBox->setValue(0);
        teamLayout->addRow("随机AI:", randomSpinBox);
        
        // 连接信号
        connect(foodHunterSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
        connect(aggressiveSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
        connect(modelBasedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
        connect(randomSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GameStartScreen::onConfigurationChanged);
        
        m_teamConfigWidgets[i] = teamWidget;
        
        if (m_aiConfigLayout) {
            m_aiConfigLayout->addWidget(teamWidget);
        }
    }
}

void GameStartScreen::updateBossTeamComboBox() {
    if (!m_bossTeamComboBox) return;
    
    m_bossTeamComboBox->clear();
    int teamCount = m_teamsSpinBox ? m_teamsSpinBox->value() : 8;
    
    for (int i = 0; i < teamCount; ++i) {
        m_bossTeamComboBox->addItem(QString("队伍 %1").arg(i + 1), i);
    }
}

void GameStartScreen::setupAnimations() {
    // 创建透明度效果
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    
    // 创建淡入动画
    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(1000);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    
    // 创建动画定时器
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &GameStartScreen::updateAnimations);
    m_animationTimer->start(50); // 20 FPS
    
    // 开始淡入动画
    m_fadeAnimation->start();
}

void GameStartScreen::updateAnimations() {
    // 这里可以添加持续的动画效果，比如浮动粒子等
    // 目前保持简单
}

void GameStartScreen::applyTheme() {
    setStyleSheet(
        "GameStartScreen { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #0f172a, stop:0.5 #1e293b, stop:1 #0f172a); "
        "} "
        
        "QGroupBox { "
        "font-weight: bold; "
        "border: 2px solid #334155; "
        "border-radius: 12px; "
        "margin: 10px; "
        "padding-top: 15px; "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #1e293b, stop:1 #0f172a); "
        "color: #f1f5f9; "
        "} "
        
        "QGroupBox::title { "
        "subcontrol-origin: margin; "
        "left: 15px; "
        "padding: 0 8px 0 8px; "
        "color: #3b82f6; "
        "font-size: 14px; "
        "} "
        
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #374151, stop:1 #1f2937); "
        "border: 2px solid #4b5563; "
        "border-radius: 8px; "
        "color: #f9fafb; "
        "font-weight: bold; "
        "padding: 12px 20px; "
        "font-size: 14px; "
        "} "
        
        "QPushButton:hover { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #4b5563, stop:1 #374151); "
        "border-color: #6b7280; "
        "} "
        
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #1f2937, stop:1 #111827); "
        "} "
        
        "QPushButton:checked { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #3b82f6, stop:1 #1e40af); "
        "border-color: #2563eb; "
        "color: white; "
        "} "
        
        "QSpinBox, QDoubleSpinBox, QComboBox { "
        "background-color: #1e293b; "
        "border: 1px solid #475569; "
        "border-radius: 6px; "
        "padding: 6px 10px; "
        "color: #f1f5f9; "
        "font-size: 13px; "
        "} "
        
        "QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { "
        "border-color: #3b82f6; "
        "} "
        
        "QSlider::groove:horizontal { "
        "border: 1px solid #475569; "
        "height: 8px; "
        "background: #1e293b; "
        "border-radius: 4px; "
        "} "
        
        "QSlider::handle:horizontal { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #3b82f6, stop:1 #1e40af); "
        "border: 1px solid #1e40af; "
        "width: 18px; "
        "margin: -2px 0; "
        "border-radius: 9px; "
        "} "
        
        "QCheckBox { "
        "color: #f1f5f9; "
        "font-size: 13px; "
        "} "
        
        "QCheckBox::indicator { "
        "width: 18px; "
        "height: 18px; "
        "} "
        
        "QCheckBox::indicator:unchecked { "
        "background-color: #1e293b; "
        "border: 1px solid #475569; "
        "border-radius: 3px; "
        "} "
        
        "QCheckBox::indicator:checked { "
        "background-color: #3b82f6; "
        "border: 1px solid #1e40af; "
        "border-radius: 3px; "
        "} "
        
        "QTabWidget::pane { "
        "border: 1px solid #475569; "
        "border-radius: 8px; "
        "background-color: #1e293b; "
        "} "
        
        "QTabBar::tab { "
        "background: #374151; "
        "color: #9ca3af; "
        "padding: 8px 16px; "
        "margin-right: 2px; "
        "border-top-left-radius: 6px; "
        "border-top-right-radius: 6px; "
        "} "
        
        "QTabBar::tab:selected { "
        "background: #3b82f6; "
        "color: white; "
        "} "
        
        "QLabel { "
        "color: #f1f5f9; "
        "font-size: 13px; "
        "} "
        
        "QTextEdit { "
        "background-color: #1e293b; "
        "border: 1px solid #475569; "
        "border-radius: 6px; "
        "color: #f1f5f9; "
        "font-size: 12px; "
        "padding: 8px; "
        "} "
        
        "QScrollArea { "
        "background-color: transparent; "
        "border: none; "
        "} "
        
        "QScrollBar:vertical { "
        "background-color: #374151; "
        "width: 12px; "
        "border-radius: 6px; "
        "} "
        
        "QScrollBar::handle:vertical { "
        "background-color: #6b7280; "
        "border-radius: 6px; "
        "min-height: 20px; "
        "} "
        
        "QScrollBar::handle:vertical:hover { "
        "background-color: #9ca3af; "
        "}"
    );
}

QJsonObject GameStartScreen::configToJson() const {
    QJsonObject config;
    
    config["mode"] = static_cast<int>(m_currentMode);
    config["totalTeams"] = m_teamsSpinBox->value();
    config["scoreMin"] = m_scoreMinSpinBox->value();
    config["scoreMax"] = m_scoreMaxSpinBox->value();
    config["worldSize"] = m_worldSizeSpinBox->value();
    config["foodDensity"] = m_foodDensitySlider->value();
    config["thornDensity"] = m_thornDensitySlider->value();
    config["teamMode"] = m_teamModeCheckBox->isChecked();
    config["friendlyFire"] = m_friendlyFireCheckBox->isChecked();
    config["timeLimit"] = m_timeLimitSpinBox->value();
    
    if (m_currentMode == GameMode::BOSS_CHALLENGE) {
        config["bossScore"] = m_bossScoreSpinBox->value();
        config["bossTeam"] = m_bossTeamComboBox->currentIndex();
        config["bossAbilities"] = m_bossAbilitiesCheckBox->isChecked();
    }
    
    return config;
}

void GameStartScreen::configFromJson(const QJsonObject& json) {
    const bool oldState = blockSignals(true);
    
    if (json.contains("mode")) {
        GameMode mode = static_cast<GameMode>(json["mode"].toInt());
        
        // 设置模式按钮
        switch (mode) {
        case GameMode::DEBUG_SINGLE_PLAYER:
            m_debugModeBtn->setChecked(true);
            break;
        case GameMode::SURVIVAL_BATTLE:
            m_survivalModeBtn->setChecked(true);
            break;
        case GameMode::BOSS_CHALLENGE:
            m_bossModeBtn->setChecked(true);
            break;
        }
        
        switchToMode(mode);
    }
    
    if (json.contains("totalTeams")) {
        m_teamsSpinBox->setValue(json["totalTeams"].toInt());
    }
    if (json.contains("scoreMin")) {
        m_scoreMinSpinBox->setValue(json["scoreMin"].toInt());
    }
    if (json.contains("scoreMax")) {
        m_scoreMaxSpinBox->setValue(json["scoreMax"].toInt());
    }
    if (json.contains("worldSize")) {
        m_worldSizeSpinBox->setValue(json["worldSize"].toInt());
    }
    if (json.contains("foodDensity")) {
        m_foodDensitySlider->setValue(json["foodDensity"].toInt());
    }
    if (json.contains("thornDensity")) {
        m_thornDensitySlider->setValue(json["thornDensity"].toInt());
    }
    if (json.contains("teamMode")) {
        m_teamModeCheckBox->setChecked(json["teamMode"].toBool());
    }
    if (json.contains("friendlyFire")) {
        m_friendlyFireCheckBox->setChecked(json["friendlyFire"].toBool());
    }
    if (json.contains("timeLimit")) {
        m_timeLimitSpinBox->setValue(json["timeLimit"].toInt());
    }
    
    if (m_currentMode == GameMode::BOSS_CHALLENGE) {
        if (json.contains("bossScore")) {
            m_bossScoreSpinBox->setValue(json["bossScore"].toInt());
        }
        if (json.contains("bossTeam")) {
            m_bossTeamComboBox->setCurrentIndex(json["bossTeam"].toInt());
        }
        if (json.contains("bossAbilities")) {
            m_bossAbilitiesCheckBox->setChecked(json["bossAbilities"].toBool());
        }
    }
    
    blockSignals(oldState);
    onConfigurationChanged();
}
